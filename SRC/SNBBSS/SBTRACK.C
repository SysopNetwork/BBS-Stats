/* 26.10.21.2 10:47AM */
/*****************************************************************************
 *   SBTRACK.C                                  BBS Stats - Usage tracking   *
 *                                                                           *
 *   How the measuring works                                                 *
 *   -----------------------                                                 *
 *   Every BBS channel has a "state": the number of the module the caller   *
 *   is in right now. Once a second a timer looks at every channel:         *
 *                                                                           *
 *   - A logged-on caller in the same module as last second, under the same *
 *     user-id, has spent that second there. The time piles up in memory.   *
 *   - When the caller leaves the module, logs off or hangs up, the piled-  *
 *     up time is written to that module's records ("flushed"), and a new   *
 *     visit starts with whatever module they are in now.                   *
 *   - A long stay is flushed every minute too, so the pages stay current   *
 *     and a crash can lose at most a minute.                               *
 *                                                                           *
 *   Nothing here depends on logon, logoff or hangup notifications: every   *
 *   second the channel is compared against what was seen the second       *
 *   before, so a recycled channel or a dropped call can never carry one    *
 *   caller's time into the next caller's record. Time is accurate to       *
 *   about a second per visit.                                               *
 *                                                                           *
 *   Only fully logged-on callers count (not callers still signing up or    *
 *   part way through logon or logoff - the BBS walks those through every   *
 *   module's logon routine, which is not real use). Callers holding the    *
 *   sysop key are skipped unless the sysop has chosen to count them - the  *
 *   BBS's own module hours leave them out the same way.                    *
 *                                                                           *
 *   The timer runs on the BBS's single main thread, so each tick does only *
 *   cheap in-memory checks; the data files are touched only on a flush.    *
 *   Deleting a module's statistics (trk_delete) removes its per-user       *
 *   records a batch per tick for the same reason.                          *
 *                                                                           *
 *   A module the sysop has stopped tracking (notrack) keeps its figures,   *
 *   but time spent in it is thrown away at the flush.                      *
 *                                                                           *
 *   Copyright (C) 2026 Sysop Network.  All Rights Reserved.                 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "gcomm.h"
#include "majorbbs.h"

#include "SBDATE.H"
#include "SBSTORE.H"
#include "SBCFG.H"
#include "SBSYS.H"
#include "SBBBS.H"
#include "SBTRACK.H"

#define TICKSECS    1                /* seconds between looks                */
#define FLUSHSECS   60               /* flush a long stay this often         */
#define MAXGAP      60               /* longest gap credited to one tick     */
#define DELBATCH    200              /* per-user records deleted per tick    */

struct chtrk {                       /* what we last saw on one channel      */
    INT    mod;                      /*   module being timed, -1 = none      */
    GBOOL  ignore;                   /*   a sysop visit we are not counting  */
    GBOOL  newvis;                   /*   visit not yet counted              */
    ULONG  pend;                     /*   seconds not yet written            */
    CHAR   uid[UIDSIZ];              /*   who it is                          */
};

static struct chtrk trk[MAXNTERM];
static time_t lastick;               /* when the previous tick ran           */
static GBOOL running;                /* FALSE after shutdown: ignore kicks   */
static GBOOL seeded;                 /* every module has a summary record    */

/* In-memory copies of the module summaries, by module number. Loaded on   */
/* first use and written through to SNBBSSM.DAT on every change.           */
static struct sbmod *mc;
static UCHAR *mcok;                  /* 1 = mc[i] is loaded                  */
static INT mcn;                      /* entries allocated                    */

/* The module whose per-user records are being deleted ("" = none).        */
static CHAR delname[MNMSIZ];

/* ---------------------------------------------------------------------- *
 * modsum (internal)                                                      *
 *                                                                        *
 * The summary for module number idx, loading it - or creating it, for a  *
 * module never seen before - on first use. A new module is shown on the  *
 * module stats page, except the menu system (module 0), which is simply  *
 * time spent moving between menus.                                       *
 * ---------------------------------------------------------------------- */
static struct sbmod *
modsum(INT idx)
{
    if (idx >= mcn) {
        INT n = nmods + 8;
        struct sbmod *nm = (struct sbmod *)realloc(mc, n * sizeof(struct sbmod));
        UCHAR *nk = (UCHAR *)realloc(mcok, n);

        if (nm == NULL || nk == NULL) {
            if (nm != NULL) mc = nm;
            if (nk != NULL) mcok = nk;
            return NULL;
        }
        mc = nm;
        mcok = nk;
        memset(mcok + mcn, 0, n - mcn);
        mcn = n;
    }
    if (!mcok[idx]) {
        if (!st_mod_get(module[idx]->descrp, &mc[idx])) {
            setmem((CHAR *)&mc[idx], sizeof(struct sbmod), 0);
            stzcpy(mc[idx].name, module[idx]->descrp, MNMSIZ);
            mc[idx].hidden = (UCHAR)(idx == 0 ? 1 : 0);
            mc[idx].first = (USHORT)today();
            st_mod_put(&mc[idx]);
        }
        mcok[idx] = 1;
    }
    return &mc[idx];
}

USHORT
trk_stamp(INT window)
{
    return sbd_stamp(window, (USHORT)today(), cfg_weekmon());
}

/* Start a window over if the stored counts belong to an earlier one.      */
static VOID
roll(struct sbwin *w, USHORT cur)
{
    if (w->stamp != cur) {
        setmem((CHAR *)w, sizeof(*w), 0);
        w->stamp = cur;
    }
}

/* ---------------------------------------------------------------------- *
 * flush (internal)                                                       *
 *                                                                        *
 * Write one channel's piled-up time to the user's record for the module *
 * and to the module's summary. The first time a user's record is touched *
 * in a new day, week or month, the module gains one distinct user for    *
 * that window; a brand-new record also adds an all-time user.            *
 * ---------------------------------------------------------------------- */
static VOID
flush(struct chtrk *t)
{
    struct sbmod *m;
    struct sbuse u;
    USHORT d = (USHORT)today();
    USHORT cur;
    INT w;

    if (t->ignore || t->mod < 0 || (t->pend == 0 && !t->newvis))
        return;
    /* Not counted while its old records are being deleted (a user record  */
    /* not yet reached would make a returning user look like a repeat), or */
    /* when the sysop has stopped tracking it.                              */
    if (delname[0] != '\0' && strcmp(module[t->mod]->descrp, delname) == 0) {
        t->pend = 0;
        t->newvis = FALSE;
        return;
    }
    if ((m = modsum(t->mod)) == NULL)
        return;
    if (m->notrack) {
        t->pend = 0;
        t->newvis = FALSE;
        return;
    }

    if (!st_use_get(m->name, t->uid, &u)) {
        setmem((CHAR *)&u, sizeof(u), 0);
        stzcpy(u.name, m->name, MNMSIZ);
        stzcpy(u.userid, t->uid, UIDSIZ);
        u.first = d;
        m->users++;
    }
    for (w = 0; w < SBW_COUNT; w++) {
        cur = trk_stamp(w);
        roll(&m->win[w], cur);
        if (u.win[w].stamp != cur) {
            u.win[w].stamp = cur;
            u.win[w].secs = 0;
            u.win[w].visits = 0;
            m->win[w].users++;
        }
        u.win[w].secs   += t->pend;
        m->win[w].secs  += t->pend;
        if (t->newvis) {
            u.win[w].visits++;
            m->win[w].visits++;
        }
    }
    u.secs += t->pend;
    m->secs += t->pend;
    if (t->newvis) {
        u.visits++;
        m->visits++;
    }
    u.last = d;
    m->last = d;
    st_use_put(&u);
    st_mod_put(m);

    t->pend = 0;
    t->newvis = FALSE;
}

/* ---------------------------------------------------------------------- *
 * tick (internal)                                                        *
 *                                                                        *
 * The once-a-second look at every channel (see the top of this file).    *
 * Runs from the BBS's real-time kick table, with no current user, so it  *
 * works purely by channel number.                                        *
 * ---------------------------------------------------------------------- */
static VOID
tick(VOID)
{
    struct user *u;
    struct chtrk *t;
    time_t nowt;
    ULONG dt;
    INT ch, online = 0;
    GBOOL active;

    if (!running)
        return;

    /* By the first tick every module has registered. Give each one a       */
    /* summary record now, so the sysop can choose which modules to show    */
    /* before anyone has used them.                                         */
    if (!seeded) {
        for (ch = 0; ch < nmods; ch++)
            modsum(ch);
        seeded = TRUE;
    }

    if (delname[0] != '\0' && !st_use_delete_mod(delname, DELBATCH)) {
        shocst("BBS Stats records removed", "%.40s: all user records deleted", delname);
        delname[0] = '\0';
    }

    nowt = time(NULL);
    dt = (nowt > lastick) ? (ULONG)(nowt - lastick) : 0;
    if (dt > MAXGAP)                 /* clock jump or a stalled board        */
        dt = MAXGAP;
    lastick = nowt;

    for (ch = 0; ch < nterms; ch++) {
        u = usroff(ch);
        t = &trk[ch];
        active = (u->usrcls == ACTUSR);
        if (active)
            online++;

        if (t->mod >= 0) {
            /* the second just gone belongs to the module seen last tick    */
            if (!t->ignore)
                t->pend += dt;
            if (!active || u->state != t->mod
             || strcmp(uacoff(ch)->userid, t->uid) != 0) {
                flush(t);            /* the visit is over                    */
                t->mod = -1;
            }
            else if (t->pend >= FLUSHSECS) {
                flush(t);            /* still there - save progress          */
            }
        }

        if (active && t->mod < 0 && u->state >= 0 && u->state < nmods) {
            t->mod = u->state;       /* a new visit starts                   */
            t->pend = 0;
            t->newvis = TRUE;
            stzcpy(t->uid, uacoff(ch)->userid, UIDSIZ);
            /* syskey is the BBS's sysop key name (writable memory, which   */
            /* the key check requires for compound locks)                   */
            t->ignore = !cfg_cntsys() && gen_haskey(syskey, ch, u) == 1;
        }
    }

    bbs_online(online);              /* peak-users counters                  */
    sys_sample();                    /* CPU load for the BBS page            */
    bbs_scan();                      /* ANSI users, a batch of accounts      */
    rtkick(TICKSECS, tick);
}

VOID
trk_init(VOID)
{
    INT ch;

    for (ch = 0; ch < MAXNTERM; ch++) {
        setmem((CHAR *)&trk[ch], sizeof(trk[ch]), 0);
        trk[ch].mod = -1;
    }
    lastick = time(NULL);
    running = TRUE;
    rtkick(TICKSECS, tick);
}

VOID
trk_stop(VOID)
{
    INT ch;

    if (!running)
        return;
    for (ch = 0; ch < nterms && ch < MAXNTERM; ch++)
        flush(&trk[ch]);
    /* Finish a delete still in progress: on the next start its leftover   */
    /* records would read as users the module has already counted.         */
    while (delname[0] != '\0' && st_use_delete_mod(delname, DELBATCH))
        ;
    delname[0] = '\0';
    running = FALSE;                 /* a kick still queued becomes a no-op  */
    free(mc);
    free(mcok);
    mc = NULL;
    mcok = NULL;
    mcn = 0;
}

VOID
trk_counts(const struct sbmod *m, INT pagewin, ULONG *users, ULONG *secs,
           ULONG *visits)
{
    if (pagewin == SBP_ALL) {
        *users = m->users;
        *secs = m->secs;
        *visits = m->visits;
    }
    else if (m->win[pagewin].stamp == trk_stamp(pagewin)) {
        *users = m->win[pagewin].users;
        *secs = m->win[pagewin].secs;
        *visits = m->win[pagewin].visits;
    }
    else {
        *users = *secs = *visits = 0;
    }
}

/* A module's summary by name: the loaded copy if there is one (so it     */
/* stays in step), else the stored record read into *tmp. NULL if none.   */
static struct sbmod *
modbyname(const CHAR *name, struct sbmod *tmp)
{
    INT i;

    for (i = 0; i < mcn; i++)
        if (mcok[i] && strcmp(mc[i].name, name) == 0)
            return &mc[i];
    return st_mod_get(name, tmp) ? tmp : NULL;
}

VOID
trk_sethidden(const CHAR *name, GBOOL hidden)
{
    struct sbmod tmp, *m;

    if ((m = modbyname(name, &tmp)) != NULL) {
        m->hidden = (UCHAR)(hidden ? 1 : 0);
        st_mod_put(m);
    }
}

VOID
trk_settrack(const CHAR *name, GBOOL track)
{
    struct sbmod tmp, *m;

    if ((m = modbyname(name, &tmp)) != NULL) {
        m->notrack = (UCHAR)(track ? 0 : 1);
        st_mod_put(m);
    }
}

INT
trk_delete(const CHAR *name)
{
    struct sbmod *m;
    UCHAR hidden, notrack;
    INT i, ch;

    if (delname[0] != '\0')
        return SBDEL_BUSY;
    stzcpy(delname, name, MNMSIZ);   /* flush() drops its time from now on  */

    for (i = 0; i < nmods; i++)
        if (strcmp(module[i]->descrp, name) == 0)
            break;
    for (ch = 0; ch < MAXNTERM; ch++) {
        if (trk[ch].mod == i) {      /* time already piled up goes too       */
            trk[ch].pend = 0;
            trk[ch].newvis = FALSE;
        }
    }
    if (i == nmods || (m = modsum(i)) == NULL) {
        st_mod_delete(name);
        return SBDEL_GONE;
    }

    /* Still installed: keep a zeroed summary, so it stays on the sysop's  */
    /* lists with its show/hide and tracking choices.                      */
    hidden = m->hidden;
    notrack = m->notrack;
    setmem((CHAR *)m, sizeof(*m), 0);
    stzcpy(m->name, name, MNMSIZ);
    m->hidden = hidden;
    m->notrack = notrack;
    m->first = (USHORT)today();
    st_mod_put(m);
    return SBDEL_RESET;
}

INT
trk_online(VOID)
{
    INT ch, n = 0;

    for (ch = 0; ch < nterms; ch++)
        if (usroff(ch)->usrcls == ACTUSR)
            n++;
    return n;
}
