/* 26.10.27.1 09:44PM */
/*****************************************************************************
 *   SBBBS.C                                  BBS Stats - The BBS stats page *
 *                                                                           *
 *   See SBBBS.H. Some items come straight from the BBS's own system         *
 *   statistics (accounts, calls, messages, files); the rest are counters    *
 *   this module keeps in SNBBSS.DAT, each window starting over by itself   *
 *   when its day, week or month ends.                                       *
 *                                                                           *
 *   ANSI users are counted here by reading the user accounts, not taken    *
 *   from the BBS's own figure: that one is a running tally the BBS adjusts *
 *   as accounts change, and some changes (ANSI switched by auto-sensing or *
 *   an account edit) never reach it, so it drifts and can go below zero.   *
 *   The count reads ACCBATCH accounts a tick, so even a large user base    *
 *   never stalls the board, and starts over every RECOUNT seconds.         *
 *                                                                           *
 *   Copyright (C) 2026 Sysop Network.  All Rights Reserved.                 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "gcomm.h"
#include "majorbbs.h"

#include "SBDATE.H"
#include "SBFMT.H"
#include "SBSTORE.H"
#include "SBCFG.H"
#include "SBTRACK.H"
#include "SBBBS.H"
#include "SBSYS.H"

#define ACCBATCH    200              /* user accounts read per tick          */
#define RECOUNT     300              /* seconds between ANSI-user counts     */

static struct sbctr ctr;             /* the counters, as stored              */
static time_t upsince;               /* when this module (the server) started*/

static struct usracc ansacc;         /* the account being looked at          */
static CHAR ansuid[UIDSIZ];          /* last account counted ("" = start)    */
static ULONG ansrun;                 /* ANSI users so far in this count      */
static ULONG ansusers;               /* the last finished count              */
static GBOOL ansok;                  /* a count has finished                 */
static GBOOL anscnt;                 /* a count is in progress               */
static time_t ansnext;               /* when the next count starts           */

/* ---------------------------------------------------------------------- *
 * The page items. ORDER IS PERMANENT: an item's position is its number   *
 * in the sysop's hide/show settings. Add new items only at the end.      *
 * ---------------------------------------------------------------------- */
enum {
    IT_NAME, IT_UP, IT_LINES, IT_ONLINE,
    IT_PKDAY, IT_PKWEEK, IT_PKMONTH, IT_PKEVER,
    IT_LGDAY, IT_LGWEEK, IT_LGMONTH, IT_LGALL, IT_CALLS,
    IT_NUDAY, IT_NUWEEK, IT_NUMONTH, IT_ACCTS,
    IT_MSGS, IT_UPLDS, IT_DNLDS, IT_MODS,
    IT_TMDAY, IT_TMWEEK, IT_TMMONTH, IT_TMALL,
    IT_SINCE,
    IT_CALLSMO, IT_ANSI,
    IT_CPU, IT_MEM, IT_DISK,         /* server health                       */
    IT_COUNT
};

static const CHAR *labels[IT_COUNT] = {
    "BBS name",
    "Server up since",
    "Lines",
    "Users online now",
    "Most online today",
    "Most online this week",
    "Most online this month",
    "Most online ever",
    "Logons today",
    "Logons this week",
    "Logons this month",
    "Logons counted",
    "Calls to date",
    "New users today",
    "New users this week",
    "New users this month",
    "User accounts",
    "Messages to date",
    "Uploads to date",
    "Downloads to date",
    "Modules installed",
    "Time online today",
    "Time online this week",
    "Time online this month",
    "Time online all-time",
    "Stats kept since",
    "Calls this month",
    "ANSI users",
    "Server CPU load",
    "Server memory",
    "Server disk space",
};

/* ---------------------------------------------------------------------- *
 * roll (internal)                                                        *
 *                                                                        *
 * Start any window whose day/week/month has ended over from zero.        *
 * Returns TRUE if anything changed (so the caller saves).                *
 * ---------------------------------------------------------------------- */
static GBOOL
roll(VOID)
{
    GBOOL changed = FALSE;
    USHORT cur;
    INT w;

    for (w = 0; w < SBW_COUNT; w++) {
        cur = trk_stamp(w);
        if (ctr.win[w].stamp != cur) {
            setmem((CHAR *)&ctr.win[w], sizeof(ctr.win[w]), 0);
            ctr.win[w].stamp = cur;
            changed = TRUE;
        }
    }
    return changed;
}

VOID
bbs_open(VOID)
{
    upsince = time(NULL);
    if (!st_ctr_load(&ctr)) {
        setmem((CHAR *)&ctr, sizeof(ctr), 0);
        ctr.id = SBREC_CTR;
        ctr.ver = SBCTR_VER;
        ctr.since = (USHORT)today();
    }
    roll();
    st_ctr_save(&ctr);
}

VOID
bbs_logon(VOID)
{
    INT w;

    roll();
    for (w = 0; w < SBW_COUNT; w++)
        ctr.win[w].logons++;
    ctr.logons++;
    st_ctr_save(&ctr);
}

VOID
bbs_newacct(VOID)
{
    INT w;

    roll();
    for (w = 0; w < SBW_COUNT; w++)
        ctr.win[w].newusr++;
    ctr.newusr++;
    st_ctr_save(&ctr);
}

VOID
bbs_online(INT n)
{
    GBOOL changed = roll();
    INT w;

    for (w = 0; w < SBW_COUNT; w++) {
        if ((USHORT)n > ctr.win[w].peak) {
            ctr.win[w].peak = (USHORT)n;
            changed = TRUE;
        }
    }
    if ((USHORT)n > ctr.peak) {
        ctr.peak = (USHORT)n;
        ctr.peakdt = (USHORT)today();
        changed = TRUE;
    }
    if (changed)                     /* only when a record or window changed */
        st_ctr_save(&ctr);
}

VOID
bbs_scan(VOID)
{
    GBOOL got;
    INT n;

    if (!anscnt) {
        if (ansok && time(NULL) < ansnext)
            return;
        ansuid[0] = '\0';
        ansrun = 0;
        anscnt = TRUE;
    }
    /* Pick up after the last account counted; ansuid is static, so the    */
    /* key lookup has the writable memory it needs. Query, then fetch the   */
    /* record found: dfaAcqNX would stop at the first different user-id.   */
    dfaSetBlk(accbb);
    got = (ansuid[0] == '\0') ? dfaQueryLO(0) : dfaQueryGT(ansuid, 0);
    for (n = 0; got && n < ACCBATCH; n++) {
        dfaAbsRec(&ansacc, 0);
        if ((ansacc.ansifl & ANSON) && !(ansacc.flags & DELTAG))
            ansrun++;
        stzcpy(ansuid, ansacc.userid, UIDSIZ);
        got = (n + 1 < ACCBATCH) ? dfaQueryNX() : TRUE;
    }
    dfaRstBlk();
    if (!got) {                      /* past the last account: count done    */
        ansusers = ansrun;
        ansok = TRUE;
        anscnt = FALSE;
        ansnext = time(NULL) + RECOUNT;
    }
}

/* One of the BBS's own running totals. They are kept as signed numbers, so */
/* one that has drifted below zero shows as zero, not as about 4 billion.   */
static ULONG
engval(LONG v)
{
    return (v > 0) ? (ULONG)v : 0;
}

INT
bbs_count(VOID)
{
    return IT_COUNT;
}

const CHAR *
bbs_label(INT item)
{
    return (item >= 0 && item < IT_COUNT) ? labels[item] : "";
}

/* Total time callers spent online in a page window: the sum over every    */
/* module, the menus included.                                              */
static ULONG
timeonline(INT pagewin)
{
    struct sbmod m;
    ULONG users, secs, visits, total = 0;
    GBOOL more;

    for (more = st_mod_walk(&m, TRUE); more; more = st_mod_walk(&m, FALSE)) {
        trk_counts(&m, pagewin, &users, &secs, &visits);
        total += secs;
    }
    return total;
}

/* The BBS's own count of calls this month, which it keeps per baud rate. */
static ULONG
callsmonth(VOID)
{
    ULONG total = 0;
    INT i;

    for (i = 0; i < (INT)(sizeof(sv.calls) / sizeof(sv.calls[0])); i++)
        total += (ULONG)sv.calls[i];
    return total;
}

/* A counter window's value, or zero if its window has ended since.        */
static ULONG
winval(INT w, INT which)
{
    if (ctr.win[w].stamp != trk_stamp(w))
        return 0;
    switch (which) {
    case 0:  return ctr.win[w].peak;
    case 1:  return ctr.win[w].logons;
    default: return ctr.win[w].newusr;
    }
}

CHAR *
bbs_value(INT item, CHAR *buf, INT len)
{
    CHAR n[24], d[24], tmp[80];
    unsigned __int64 avail, total;
    INT a, b;
    time_t up;

    tmp[0] = '\0';
    switch (item) {
    case IT_NAME:
        stzcpy(tmp, bbsttl, sizeof(tmp));
        break;
    case IT_UP:
        strftime(d, sizeof(d), "%m/%d/%y %I:%M%p", localtime(&upsince));
        up = time(NULL) - upsince;
        sprintf(tmp, "%s (%s)", d, sbf_dur(n, sizeof(n), (ULONG)(up > 0 ? up : 0)));
        break;
    case IT_LINES:     sbf_num(tmp, sizeof(tmp), (ULONG)nterms);             break;
    case IT_ONLINE:    sbf_num(tmp, sizeof(tmp), (ULONG)trk_online());       break;
    case IT_PKDAY:     sbf_num(tmp, sizeof(tmp), winval(SBW_DAY, 0));        break;
    case IT_PKWEEK:    sbf_num(tmp, sizeof(tmp), winval(SBW_WEEK, 0));       break;
    case IT_PKMONTH:   sbf_num(tmp, sizeof(tmp), winval(SBW_MONTH, 0));      break;
    case IT_PKEVER:
        stzcpy(d, ncdate(ctr.peakdt), sizeof(d));
        sprintf(tmp, "%s (%s)", sbf_num(n, sizeof(n), (ULONG)ctr.peak), d);
        break;
    case IT_LGDAY:     sbf_num(tmp, sizeof(tmp), winval(SBW_DAY, 1));        break;
    case IT_LGWEEK:    sbf_num(tmp, sizeof(tmp), winval(SBW_WEEK, 1));       break;
    case IT_LGMONTH:   sbf_num(tmp, sizeof(tmp), winval(SBW_MONTH, 1));      break;
    case IT_LGALL:
        stzcpy(d, ncdate(ctr.since), sizeof(d));
        sprintf(tmp, "%s since %s", sbf_num(n, sizeof(n), ctr.logons), d);
        break;
    case IT_CALLS:     sbf_num(tmp, sizeof(tmp), engval((LONG)sv2.totcalls));    break;
    case IT_NUDAY:     sbf_num(tmp, sizeof(tmp), winval(SBW_DAY, 2));        break;
    case IT_NUWEEK:    sbf_num(tmp, sizeof(tmp), winval(SBW_WEEK, 2));       break;
    case IT_NUMONTH:   sbf_num(tmp, sizeof(tmp), winval(SBW_MONTH, 2));      break;
    case IT_ACCTS:     sbf_num(tmp, sizeof(tmp), engval((LONG)sv2.numact));      break;
    case IT_MSGS:      sbf_num(tmp, sizeof(tmp), engval(sv.msgtot));             break;
    case IT_UPLDS:     sbf_num(tmp, sizeof(tmp), engval(sv.uplds));              break;
    case IT_DNLDS:     sbf_num(tmp, sizeof(tmp), engval(sv.dwnlds));             break;
    case IT_MODS:      sbf_num(tmp, sizeof(tmp), (ULONG)(nmods - 1));        break;
    case IT_TMDAY:     sbf_dur(tmp, sizeof(tmp), timeonline(SBP_DAY));       break;
    case IT_TMWEEK:    sbf_dur(tmp, sizeof(tmp), timeonline(SBP_WEEK));      break;
    case IT_TMMONTH:   sbf_dur(tmp, sizeof(tmp), timeonline(SBP_MONTH));     break;
    case IT_TMALL:     sbf_dur(tmp, sizeof(tmp), timeonline(SBP_ALL));       break;
    case IT_SINCE:     stzcpy(tmp, ncdate(ctr.since), sizeof(tmp));          break;
    case IT_CALLSMO:   sbf_num(tmp, sizeof(tmp), callsmonth());              break;
    case IT_ANSI:
        if (ansok)
            sbf_num(tmp, sizeof(tmp), ansusers);
        else
            stzcpy(tmp, "counting...", sizeof(tmp));
        break;
    case IT_CPU:
        if (sys_cpu(&a, &b))
            sprintf(tmp, "%d%% (BBS server %d%%)", a, b);
        else
            stzcpy(tmp, "measuring...", sizeof(tmp));
        break;
    case IT_MEM:
        if (sys_mem(&avail, &total, &a)) {
            sbf_bytes(n, sizeof(n), avail);
            sbf_bytes(d, sizeof(d), total);
            sprintf(tmp, "%s free of %s (%d%% in use)", n, d, a);
        }
        else
            stzcpy(tmp, "unknown", sizeof(tmp));
        break;
    case IT_DISK:
        if (sys_disk(&avail, &total)) {
            sbf_bytes(n, sizeof(n), avail);
            sbf_bytes(d, sizeof(d), total);
            sprintf(tmp, "%s free of %s", n, d);
        }
        else
            stzcpy(tmp, "unknown", sizeof(tmp));
        break;
    }
    stzcpy(buf, tmp, len);
    return buf;
}
