/* 26.10.21.1 10:28AM */
/*****************************************************************************
 *   SNBBSS.C                          The Major BBS v10 Module - BBS Stats  *
 *                                                                           *
 *   Copyright (C) 2026 Sysop Network.  All Rights Reserved.                 *
 *                                                                           *
 *   Description:   BBS Stats gathers statistics about the BBS itself and   *
 *                  about how callers use it and its modules, and shows     *
 *                  them on two pages:                                       *
 *                                                                           *
 *                  - the BBS stats page: system-wide figures such as       *
 *                    callers online, logons, new users and peaks;          *
 *                  - the module stats page: for each module, how many      *
 *                    different users used it, how often, the total time   *
 *                    spent there and the average time per user - for      *
 *                    today, this week, this month or all-time.            *
 *                                                                           *
 *                  The sysop chooses, online and without a restart, which *
 *                  items and modules appear and who may view each page.    *
 *                  Both pages can also be called up from anywhere on the  *
 *                  BBS with global commands, named by the sysop.           *
 *                                                                           *
 *                  This file is the module entry point and the per-user   *
 *                  menus. Measuring happens in SBTRACK.C, the BBS-wide     *
 *                  counters in SBBBS.C, settings in SBCFG.C and all data  *
 *                  files in SBSTORE.C.                                     *
 *                                                                           *
 *   Created by Jacque Steyn (The Adept BBS) and Mark Laudenbach.            *
 *   See LICENSE file in the project root for terms and conditions.          *
 *                                                                           *
 *****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gcomm.h"
#include "majorbbs.h"

#include "SNBBSS.H"
#include "SBDATE.H"
#include "SBFMT.H"
#include "SBSTORE.H"
#include "SBCFG.H"
#include "SBTRACK.H"
#include "SBBBS.H"
#include "SBTVAR.H"

/* ---- forward declarations -------------------------------------------- */
GBOOL sb_logon(VOID);
GBOOL sb_input(VOID);
INT   sb_global(VOID);
VOID  sb_delacct(CHAR *userid);
VOID  bbs_newacct_hook(const struct usracc *acct);
VOID  sb_shutdown(VOID);
VOID  end_snbbss(VOID);

/* ---- module interface block ------------------------------------------ */
struct module SNBBSS = {
     "",                             /* description (set from the MDF)       */
     sb_logon,                       /* logon: count it, show the ad         */
     sb_input,                       /* input while in the module            */
     NULL,                           /* status input                         */
     NULL,                           /* injected messages                    */
     NULL,                           /* logoff (tracking needs no hook)      */
     NULL,                           /* hangup (tracking needs no hook)      */
     NULL,                           /* nightly cleanup (windows roll alone) */
     sb_delacct,                     /* account deleted                      */
     sb_shutdown                     /* clean shutdown                       */
};

HMCVFILE sbmb;                       /* message file handle                  */
INT sbstt;                           /* this module's number                 */

/* ---- where a user is in the module (usrptr->substt) ------------------- */
#define ST_MENU     1                /* main menu                            */
#define ST_MODS     2                /* module page: pick a window           */
#define ST_CFG      3                /* sysop configuration menu             */
#define ST_SET      4                /* general settings: pick a row         */
#define ST_SETVAL   5                /* general settings: enter a value      */
#define ST_TOG      6                /* a show/hide list                     */
#define ST_DELOK    7                /* modules shown: confirm a delete      */

/* The show/hide lists on the sysop configuration menu.                     */
#define TL_ITEMS    2                /* BBS page items                       */
#define TL_COLS     3                /* module page columns                  */
#define TL_MODS     4                /* modules shown                        */
#define CM_AD       5                /* config menu: the logon advertisement */

/* Per-channel progress through the screens, reset on every module entry.  */
static INT pick[MAXNTERM];           /* setting being edited                 */
static INT togl[MAXNTERM];           /* show/hide list being edited          */
static INT sgrp[MAXNTERM];           /* settings screen being edited (CFG_*) */
static CHAR delnm[MAXNTERM][MNMSIZ]; /* module whose delete awaits a yes     */

/* Scratch space for building the module page and the module list. Pages   */
/* are built and sent in one go on the single main thread, so one shared   */
/* copy is enough.                                                          */
#define MAXROWS 512
struct mrow {
    CHAR  name[MNMSIZ];
    UCHAR hidden;
    UCHAR notrack;
    ULONG users, secs, visits;
};
static struct mrow rows[MAXROWS];
static INT nrows;

static const CHAR *winname[SBP_COUNT] = {
    "Showing today", "Showing this week", "Showing this month", "Showing all-time"
};
static const CHAR *colname[SBC_COUNT] = {
    "Users - different callers who used the module",
    "Visits - times the module was entered",
    "Total time - time spent in the module",
    "Avg/user - total time divided by users",
};

/* ---------------------------------------------------------------------- *
 * init__snbbss                                                           *
 *                                                                        *
 * The BBS calls this once at startup; its name must match the DLL name.  *
 * ---------------------------------------------------------------------- */
VOID EXPORT
init__snbbss(VOID)
{
    stzcpy(SNBBSS.descrp, gmdnam("SNBBSS.MDF"), MNMSIZ);
    sbstt = register_module(&SNBBSS);
    sbmb = opnmsg("SNBBSS.MCV");

    st_open();                       /* data files (created on first run)    */
    cfg_open();                      /* live settings (defaults on first run)*/
    bbs_open();                      /* BBS-wide counters                    */
    tvar_init();                     /* SBS_ text variables                  */
    hook_createacct(bbs_newacct_hook);
    globalcmd(sb_global);            /* the stats pages from anywhere        */
    trk_init();                      /* start measuring module use           */

    /* The running build names itself on the audit trail.                   */
    shocst("BBS Stats loaded", "%s", "BBS Stats v" SB_VERSION " is gathering statistics");
}

/* ---------------------------------------------------------------------- *
 * bbs_newacct_hook                                                       *
 *                                                                        *
 * Called by the BBS whenever a new account is created.                   *
 * ---------------------------------------------------------------------- */
VOID
bbs_newacct_hook(const struct usracc *acct)
{
    (VOID)acct;
    bbs_newacct();
}

static VOID show_ad(GBOOL preview);
static GBOOL may_view(const CHAR *key);
static GBOOL sees_ad(VOID);

/* ---------------------------------------------------------------------- *
 * sb_logon                                                               *
 *                                                                        *
 * The BBS runs every module's logon routine as each caller logs on       *
 * (MAJORBBS.C cyclon). This one counts the logon and, if the sysop has   *
 * not switched it off, shows the advertisement to callers it is meant   *
 * for (sees_ad). Returning 0 lets the logon carry on. Client/server      *
 * sessions are skipped as the engine's own logon routines do (AAEML.C    *
 * emllon).                                                               *
 * ---------------------------------------------------------------------- */
GBOOL EXPORT
sb_logon(VOID)
{
    bbs_logon();
    if (cfg_adon() && !(usrptr->flags & WSGCSU) && sees_ad()) {
        setmbk(sbmb);
        show_ad(FALSE);
        outprf(usrnum);
        rstmbk();
    }
    return 0;
}

/* ---- small helpers --------------------------------------------------- */

/* prfmsg() writes into the BBS's output staging buffer with no bounds     */
/* check (gcommlib PRF.C), and that buffer holds only 8K on some boards    */
/* (MAJORBBS.C, the OUTBSZ option). Long lists send what they have built   */
/* so far before it can fill.                                               */
static VOID
flushlong(VOID)
{
    if (prfptr - prfbuf > 4096)
        outprf(usrnum);
}

static GBOOL
is_sysop(VOID)
{
    return haskey(syskey) == 1 || haskey("MASTER") == 1;
}

/* A page with no key set is open to all; sysops can always see it. A key  */
/* that does not exist on the BBS returns -1, so only a real "yes" (1)     */
/* opens the page.                                                          */
static GBOOL
may_view(const CHAR *key)
{
    CHAR k[KEYSIZ];

    if (key[0] == '\0' || is_sysop())
        return TRUE;
    stzcpy(k, key, sizeof(k));       /* key checks need writable memory      */
    return haskey(k) == 1;
}

/* Does this caller get the ad at logon? The sysop can require a key to  */
/* see it and name a key that hides it; the hiding key wins. A user with  */
/* the MASTER flag passes any key check (LOCKNKEY.C low_haskey),         */
/* so sysops skip the ad whenever a hiding key is set.                    */
static GBOOL
sees_ad(VOID)
{
    CHAR k[KEYSIZ];

    if (cfg_adnokey()[0] != '\0') {
        stzcpy(k, cfg_adnokey(), sizeof(k)); /* key checks need writable memory */
        if (haskey(k) == 1)
            return FALSE;
    }
    return may_view(cfg_adkey());
}

static LONG
parse_num(const CHAR *s)
{
    LONG v = 0;

    if (*s == '\0')
        return -1;
    for (; *s != '\0'; s++) {
        if (*s < '0' || *s > '9' || v > 100000L)
            return -1;
        v = v * 10 + (*s - '0');
    }
    return v;
}

/* Write a line to the audit trail naming the sysop who changed something. */
/* Audit lines must stay under 128 characters.                             */
static VOID
audit(const CHAR *what, const CHAR *value)
{
    CHAR det[120];

    _snprintf(det, sizeof(det) - 1, "%.29s: %.48s = %.30s", usaptr->userid, what, value);
    det[sizeof(det) - 1] = '\0';
    shocst("BBS Stats setting changed", "%s", det);
}

static VOID
show_menu(VOID)
{
    prfmsg(MAINMNU);
    if (is_sysop())
        prfmsg(SYSOPT);
    prfmsg(MAINPRM);
}

/* ---------------------------------------------------------------------- *
 * show_bbs                                                               *
 *                                                                        *
 * The BBS stats page: every item the sysop has left switched on.         *
 * ---------------------------------------------------------------------- */
static VOID
show_bbs(VOID)
{
    CHAR val[80];
    INT i;

    prfmsg(BBSHDR);
    for (i = 0; i < bbs_count(); i++) {
        if (!cfg_itemshown(i))
            continue;
        tvar_set(TV_LABEL, bbs_label(i));
        tvar_set(TV_VALUE, bbs_value(i, val, sizeof(val)));
        prfmsg(BBSROW);
    }
    prfmsg(BBSFTR);
}

/* ---------------------------------------------------------------------- *
 * collect_mods (internal)                                                *
 *                                                                        *
 * Load every module summary into rows[] with its counts for one page     *
 * window.                                                                *
 * ---------------------------------------------------------------------- */
static VOID
collect_mods(INT win)
{
    struct sbmod m;
    GBOOL more;

    nrows = 0;
    for (more = st_mod_walk(&m, TRUE); more && nrows < MAXROWS;
         more = st_mod_walk(&m, FALSE)) {
        stzcpy(rows[nrows].name, m.name, MNMSIZ);
        rows[nrows].hidden = m.hidden;
        rows[nrows].notrack = m.notrack;
        trk_counts(&m, win, &rows[nrows].users, &rows[nrows].secs,
                   &rows[nrows].visits);
        nrows++;
    }
}

static int
by_name(const void *a, const void *b)
{
    return stricmp(((const struct mrow *)a)->name, ((const struct mrow *)b)->name);
}

static int
by_time(const void *a, const void *b)
{
    const struct mrow *x = (const struct mrow *)a, *y = (const struct mrow *)b;

    if (x->secs != y->secs)
        return x->secs < y->secs ? 1 : -1;
    return by_name(a, b);
}

static int
by_users(const void *a, const void *b)
{
    const struct mrow *x = (const struct mrow *)a, *y = (const struct mrow *)b;

    if (x->users != y->users)
        return x->users < y->users ? 1 : -1;
    return by_time(a, b);
}

static int
by_visits(const void *a, const void *b)
{
    const struct mrow *x = (const struct mrow *)a, *y = (const struct mrow *)b;

    if (x->visits != y->visits)
        return x->visits < y->visits ? 1 : -1;
    return by_time(a, b);
}

/* ---------------------------------------------------------------------- *
 * show_ad                                                                *
 *                                                                        *
 * The logon advertisement: the top modules by the sysop's chosen         *
 * measure and window, in the chosen layout. Modules hidden from the      *
 * module page or not tracked, and modules with nothing to show, are     *
 * left out. With nothing at all to show, a logon shows nothing; the      *
 * sysop's preview says why instead.                                      *
 *                                                                        *
 * The table layout sends one message per module, so the sysop can       *
 * restyle each part in SNBBSS.MSG. The two other layouts are laid out    *
 * here and sent a line at a time through SBS_MROW.                       *
 * ---------------------------------------------------------------------- */
static VOID
show_ad(GBOOL preview)
{
    static const CHAR *ranknm[] = { "", "time played", "users", "visits" };
    static const CHAR *periodnm[SBP_COUNT] = {
        "today", "this week", "this month", "all-time"
    };
    INT top[SBAD_MAX];
    CHAR fig[SBAD_MAX][16], line[96], cell[48];
    INT i, j, n = 0, half, len, rank = cfg_adrank();
    GBOOL figs = cfg_adfigs();
    ULONG v;

    collect_mods(cfg_adwin());
    switch (rank) {
    case SBR_USERS:  qsort(rows, nrows, sizeof(rows[0]), by_users);  break;
    case SBR_VISITS: qsort(rows, nrows, sizeof(rows[0]), by_visits); break;
    default:         qsort(rows, nrows, sizeof(rows[0]), by_time);   break;
    }
    for (i = 0; i < nrows && n < cfg_adcount(); i++) {
        v = rank == SBR_USERS ? rows[i].users
          : rank == SBR_VISITS ? rows[i].visits : rows[i].secs;
        if (rows[i].hidden || rows[i].notrack || rows[i].users == 0 || v == 0)
            continue;
        if (rank == SBR_TIME)
            sbf_dur(fig[n], sizeof(fig[n]), v);
        else
            sbf_num(fig[n], sizeof(fig[n]), v);
        top[n++] = i;
    }
    if (n == 0) {
        if (preview)
            prfmsg(ADNONE);
        return;
    }

    tvar_set(TV_TITLE, cfg_adtitle());
    tvar_setn(TV_COUNT, (LONG)n);
    tvar_set(TV_RANKBY, ranknm[rank]);
    tvar_set(TV_PERIOD, periodnm[cfg_adwin()]);
    prfmsg(ADHDR);

    switch (cfg_adstyle()) {
    case SBL_TWOCOL:                 /* down the left column, then the right */
        half = (n + 1) / 2;
        for (i = 0; i < half; i++) {
            line[0] = '\0';
            for (j = i; j < n; j += half) {
                if (figs)
                    sprintf(cell, "%2d. %-19.19s %9.9s", j + 1, rows[top[j]].name, fig[j]);
                else
                    sprintf(cell, "%2d. %-29.29s", j + 1, rows[top[j]].name);
                if (line[0] != '\0')
                    strcat(line, "   ");
                strcat(line, cell);
            }
            tvar_set(TV_MROW, line);
            prfmsg(ADLROW);
        }
        break;

    case SBL_COMPACT:                /* one running list, wrapped at 70      */
        line[0] = '\0';
        for (i = 0; i < n; i++) {
            if (figs)
                sprintf(cell, "%d. %.24s (%s)", i + 1, rows[top[i]].name, fig[i]);
            else
                sprintf(cell, "%d. %.24s", i + 1, rows[top[i]].name);
            len = (INT)strlen(line);
            if (len > 0 && len + 3 + (INT)strlen(cell) > 70) {
                tvar_set(TV_MROW, line);
                prfmsg(ADLROW);
                line[0] = '\0';
                len = 0;
            }
            if (len > 0)
                strcat(line, "   ");
            strcat(line, cell);
        }
        tvar_set(TV_MROW, line);
        prfmsg(ADLROW);
        break;

    default:                         /* SBL_TABLE                            */
        for (i = 0; i < n; i++) {
            tvar_setn(TV_NUM, (LONG)(i + 1));
            tvar_set(TV_LABEL, rows[top[i]].name);
            tvar_set(TV_VALUE, figs ? fig[i] : "");
            prfmsg(ADTROW);
        }
        break;
    }
    prfmsg(ADFTR);
}

/* Append one right-aligned column to a module page line.                  */
static VOID
addcol(CHAR *line, INT col, INT width, const CHAR *text)
{
    CHAR cell[24];

    if (!cfg_colshown(col))
        return;
    sprintf(cell, " %*.*s", width, width, text);
    strcat(line, cell);
}

/* ---------------------------------------------------------------------- *
 * show_mods                                                              *
 *                                                                        *
 * The module stats page for one window. Hidden and untracked modules,    *
 * and modules nobody used in the window, are left out. The columns are laid out here *
 * (the sysop can hide any of them), then handed to the message text      *
 * through SBS_MHEAD and SBS_MROW. The caller sends whatever follows the  *
 * page: the window prompt in the module, a footer for /MODSTATS.         *
 * ---------------------------------------------------------------------- */
static VOID
show_mods(INT win)
{
    CHAR line[96], n[24];
    INT i, shown = 0;

    collect_mods(win);
    switch (cfg_sortby()) {
    case SBS_USERS: qsort(rows, nrows, sizeof(rows[0]), by_users); break;
    case SBS_NAME:  qsort(rows, nrows, sizeof(rows[0]), by_name);  break;
    default:        qsort(rows, nrows, sizeof(rows[0]), by_time);  break;
    }

    sprintf(line, "%-24s", "Module");
    addcol(line, SBC_USERS, 7, "Users");
    addcol(line, SBC_VISITS, 8, "Visits");
    addcol(line, SBC_TIME, 11, "Total time");
    addcol(line, SBC_AVG, 11, "Avg/user");
    tvar_set(TV_MHEAD, line);
    memset(line, '-', strlen(line));  /* the underline, same width          */
    tvar_set(TV_MROW, line);
    tvar_set(TV_WINDOW, winname[win]);
    prfmsg(MODHDR);

    for (i = 0; i < nrows; i++) {
        if (rows[i].hidden || rows[i].notrack || rows[i].users == 0)
            continue;
        sprintf(line, "%-24.24s", rows[i].name);
        addcol(line, SBC_USERS, 7, sbf_num(n, sizeof(n), rows[i].users));
        addcol(line, SBC_VISITS, 8, sbf_num(n, sizeof(n), rows[i].visits));
        addcol(line, SBC_TIME, 11, sbf_dur(n, sizeof(n), rows[i].secs));
        addcol(line, SBC_AVG, 11, sbf_dur(n, sizeof(n), rows[i].secs / rows[i].users));
        tvar_set(TV_MROW, line);
        prfmsg(MODROW);
        flushlong();
        shown++;
    }
    if (shown == 0)
        prfmsg(MODNONE);
}

/* ---------------------------------------------------------------------- *
 * sb_global                                                              *
 *                                                                        *
 * The global command handler: the BBS offers it every line a user types, *
 * wherever they are (MAJORBBS.C hdlcri). The sysop names the commands   *
 * (default /BBSSTATS and /MODSTATS) or switches either off. The first    *
 * shows the BBS stats page, the second the module stats page, optionally *
 * for a period given as T, W, M or A. Page keys apply as in the module. *
 *                                                                        *
 * Returns 1 when the line was ours; the BBS then re-prompts whatever the *
 * user was doing (injacr). Anything else, including the commands with    *
 * arguments we don't take, returns 0 so other handlers can have it.      *
 * ---------------------------------------------------------------------- */
INT EXPORT
sb_global(VOID)
{
    static const CHAR *wins[SBP_COUNT] = { "T", "W", "M", "A" };
    INT win;

    if (usrptr->usrcls != ACTUSR || margc < 1)
        return 0;

    if (margc == 1 && cfg_bbscmd()[0] != '\0' && sameas(margv[0], (CHAR *)cfg_bbscmd())) {
        setmbk(sbmb);
        if (may_view(cfg_bbskey()))
            show_bbs();
        else
            prfmsg(NOACC);
    }
    else if (margc <= 2 && cfg_modcmd()[0] != '\0'
          && sameas(margv[0], (CHAR *)cfg_modcmd())) {
        win = cfg_defwin();
        if (margc == 2) {
            for (win = 0; win < SBP_COUNT && !sameas(margv[1], (CHAR *)wins[win]); win++)
                ;
        }
        setmbk(sbmb);
        if (win == SBP_COUNT)
            prfmsg(GMODUSE);
        else if (may_view(cfg_modkey())) {
            show_mods(win);
            prfmsg(GMODFTR);
        }
        else
            prfmsg(NOACC);
    }
    else
        return 0;

    outprf(usrnum);
    rstmbk();
    return 1;
}

/* ---------------------------------------------------------------------- *
 * The sysop's settings screens: general settings and the logon ad. Both  *
 * are numbered lists of rows from SBCFG.C; the ad's prompt also offers   *
 * P to preview the ad as it stands.                                      *
 * ---------------------------------------------------------------------- */
static VOID
set_prompt(INT grp)
{
    tvar_setn(TV_COUNT, (LONG)cfg_count(grp));
    prfmsg(grp == CFG_AD ? ADSPRM : SETPRM);
}

static VOID
show_settings(INT grp)
{
    CHAR val[SBAD_TITLE];
    INT i;

    prfmsg(grp == CFG_AD ? ADSHDR : SETHDR);
    for (i = 1; i <= cfg_count(grp); i++) {
        tvar_setn(TV_NUM, (LONG)i);
        tvar_set(TV_LABEL, cfg_label(grp, i));
        tvar_set(TV_VALUE, cfg_valtext(grp, i, val, sizeof(val)));
        prfmsg(SETROW);
    }
    set_prompt(grp);
}

/* ---------------------------------------------------------------------- *
 * The show/hide lists                                                    *
 *                                                                        *
 * Three lists share one screen: BBS page items, module page columns and  *
 * the modules on the module page. Each is numbered from 1 on screen.     *
 * ---------------------------------------------------------------------- */
static INT
tog_count(INT list)
{
    switch (list) {
    case TL_ITEMS: return bbs_count();
    case TL_COLS:  return SBC_COUNT;
    case TL_MODS:
        collect_mods(SBP_ALL);       /* alphabetical, so numbers are stable  */
        qsort(rows, nrows, sizeof(rows[0]), by_name);
        return nrows;
    }
    return 0;
}

/* Label and state of entry i (0-based). For TL_MODS, tog_count() must     */
/* have just filled rows[].                                                 */
static const CHAR *
tog_label(INT list, INT i)
{
    switch (list) {
    case TL_ITEMS: return bbs_label(i);
    case TL_COLS:  return colname[i];
    case TL_MODS:  return rows[i].name;
    }
    return "";
}

static GBOOL
tog_shown(INT list, INT i)
{
    switch (list) {
    case TL_ITEMS: return cfg_itemshown(i);
    case TL_COLS:  return cfg_colshown(i);
    case TL_MODS:  return !rows[i].hidden;
    }
    return FALSE;
}

static VOID
tog_set(INT list, INT i, GBOOL shown)
{
    switch (list) {
    case TL_ITEMS: cfg_setitem(i, shown); break;
    case TL_COLS:  cfg_setcol(i, shown);  break;
    case TL_MODS:
        trk_sethidden(rows[i].name, !shown);
        rows[i].hidden = (UCHAR)(shown ? 0 : 1);
        break;
    }
}

/* The modules list also takes T# (tracking) and D# (delete), so it has   */
/* its own prompt.                                                          */
static VOID
tog_prompt(INT list, INT n)
{
    tvar_setn(TV_COUNT, (LONG)n);
    prfmsg(list == TL_MODS ? MODTPRM : TOGPRM);
}

static VOID
show_toggles(INT list)
{
    static const CHAR *titles[] = {
        "", "", "BBS PAGE ITEMS", "MODULE PAGE COLUMNS", "MODULES SHOWN"
    };
    CHAR lbl[MNMSIZ + 16];
    INT i, n = tog_count(list);

    tvar_set(TV_TITLE, titles[list]);
    prfmsg(TOGHDR);
    for (i = 0; i < n; i++) {
        tvar_setn(TV_NUM, (LONG)(i + 1));
        tvar_set(TV_MARK, tog_shown(list, i) ? "*" : " ");
        if (list == TL_MODS && rows[i].notrack) {
            sprintf(lbl, "%s (not tracked)", rows[i].name);
            tvar_set(TV_LABEL, lbl);
        }
        else
            tvar_set(TV_LABEL, tog_label(list, i));
        prfmsg(TOGROW);
        flushlong();
    }
    tog_prompt(list, n);
}

/* The number in a "<letter><n>" or "<letter> <n>" command, such as D5 or  */
/* D 5; -1 if the line is not that command.                                 */
static LONG
lettered(CHAR letter)
{
    if (margc < 1 || toupper((UCHAR)margv[0][0]) != letter)
        return -1;
    if (margc == 1 && margv[0][1] != '\0')
        return parse_num(margv[0] + 1);
    if (margc == 2 && margv[0][1] == '\0')
        return parse_num(margv[1]);
    return -1;
}

/* ---------------------------------------------------------------------- *
 * do_toggle (internal)                                                   *
 *                                                                        *
 * Act on what the sysop typed on a show/hide list: a number switches one *
 * entry, A shows all, N hides all. On the modules list, T# stops or      *
 * restarts tracking a module and D# asks to delete its statistics.       *
 * ---------------------------------------------------------------------- */
static VOID
do_toggle(INT list, const CHAR *in)
{
    INT i, n = tog_count(list);
    LONG v;
    GBOOL on;

    if (list == TL_MODS && (lettered('T') >= 0 || lettered('D') >= 0)) {
        v = lettered('T') >= 0 ? lettered('T') : lettered('D');
        if (v < 1 || v > n) {
            prfmsg(TOGBAD);
            tog_prompt(list, n);
            return;
        }
        i = (INT)v - 1;
        if (lettered('D') >= 0) {
            stzcpy(delnm[usrnum], rows[i].name, MNMSIZ);
            tvar_set(TV_LABEL, delnm[usrnum]);
            prfmsg(DELASK);
            usrptr->substt = ST_DELOK;
            return;
        }
        on = rows[i].notrack != 0;   /* untracked now, so track it again     */
        trk_settrack(rows[i].name, on);
        audit(rows[i].name, on ? "tracked" : "not tracked");
    }
    else if (sameas((CHAR *)in, "A") || sameas((CHAR *)in, "N")) {
        on = sameas((CHAR *)in, "A");
        for (i = 0; i < n; i++)
            tog_set(list, i, on);
        audit(list == TL_ITEMS ? "BBS page items" : list == TL_COLS
              ? "Module page columns" : "Modules shown", on ? "all shown" : "all hidden");
    }
    else {
        v = parse_num(in);
        if (v < 1 || v > n) {
            prfmsg(TOGBAD);
            tog_prompt(list, n);
            return;
        }
        i = (INT)v - 1;
        on = !tog_shown(list, i);
        tog_set(list, i, on);
        audit(tog_label(list, i), on ? "shown" : "hidden");
    }
    show_toggles(list);
}

/* ---------------------------------------------------------------------- *
 * reprompt (internal)                                                    *
 *                                                                        *
 * Ask again for whatever the current screen was waiting for, without     *
 * changing anything.                                                     *
 * ---------------------------------------------------------------------- */
static VOID
reprompt(VOID)
{
    CHAR val[SBAD_TITLE];
    INT g = sgrp[usrnum];

    switch (usrptr->substt) {
    case ST_MENU: prfmsg(MAINPRM); break;
    case ST_MODS: prfmsg(MODPRM);  break;
    case ST_CFG:  prfmsg(CFGPRM);  break;
    case ST_SET:  set_prompt(g);   break;
    case ST_SETVAL:
        tvar_set(TV_LABEL, cfg_label(g, pick[usrnum]));
        tvar_set(TV_VALUE, cfg_valtext(g, pick[usrnum], val, sizeof(val)));
        tvar_set(TV_RANGE, cfg_range(g, pick[usrnum]));
        prfmsg(SETASK);
        break;
    case ST_TOG:
        tog_prompt(togl[usrnum], tog_count(togl[usrnum]));
        break;
    case ST_DELOK:
        tvar_set(TV_LABEL, delnm[usrnum]);
        prfmsg(DELASK);
        break;
    }
}

/* ---------------------------------------------------------------------- *
 * sb_input                                                               *
 *                                                                        *
 * Everything a caller types while in the module. Returns 1 to stay in    *
 * the module, 0 to go back to the BBS.                                   *
 * ---------------------------------------------------------------------- */
GBOOL EXPORT
sb_input(VOID)
{
    const CHAR *in;
    CHAR val[SBAD_TITLE];
    INT g;
    LONG n;

    setmbk(sbmb);
    in = (margc > 0) ? margv[0] : "";

    /* The sysop screens re-check the key on every line, in case it was    */
    /* taken away mid-session.                                             */
    if (usrptr->substt >= ST_CFG && !is_sysop())
        usrptr->substt = 0;

    /* After a global command (ours or another module's, e.g. /P or        */
    /* /STATS) the BBS sends an empty line to re-prompt (MAJORBBS.C        */
    /* injacr, which sets INJOIP). An empty line means "go back" on most   */
    /* screens here, so ask again instead of acting on it.                 */
    if ((usrptr->flags & INJOIP) && usrptr->substt != 0) {
        reprompt();
        outprf(usrnum);
        rstmbk();
        return 1;
    }

    switch (usrptr->substt) {
    case 0:                          /* just entered the module              */
        pick[usrnum] = 0;
        togl[usrnum] = 0;
        sgrp[usrnum] = CFG_GEN;
        show_menu();
        usrptr->substt = ST_MENU;
        break;

    case ST_MENU:
        if (sameas((CHAR *)in, "B")) {
            if (may_view(cfg_bbskey()))
                show_bbs();
            else
                prfmsg(NOACC);
            prfmsg(MAINPRM);
        }
        else if (sameas((CHAR *)in, "M")) {
            if (may_view(cfg_modkey())) {
                show_mods(cfg_defwin());
                prfmsg(MODPRM);
                usrptr->substt = ST_MODS;
            }
            else {
                prfmsg(NOACC);
                prfmsg(MAINPRM);
            }
        }
        else if (sameas((CHAR *)in, "S") && is_sysop()) {
            prfmsg(CFGMNU);
            prfmsg(CFGPRM);
            usrptr->substt = ST_CFG;
        }
        else if (sameas((CHAR *)in, "X")) {
            prfmsg(EXITMSG);
            outprf(usrnum);
            rstmbk();
            return 0;
        }
        else if (sameas((CHAR *)in, "?") || *in == '\0') {
            show_menu();
        }
        else {
            prfmsg(BADSEL);
            prfmsg(MAINPRM);
        }
        break;

    case ST_MODS:
        if      (sameas((CHAR *)in, "T")) { show_mods(SBP_DAY);   prfmsg(MODPRM); }
        else if (sameas((CHAR *)in, "W")) { show_mods(SBP_WEEK);  prfmsg(MODPRM); }
        else if (sameas((CHAR *)in, "M")) { show_mods(SBP_MONTH); prfmsg(MODPRM); }
        else if (sameas((CHAR *)in, "A")) { show_mods(SBP_ALL);   prfmsg(MODPRM); }
        else if (sameas((CHAR *)in, "X") || *in == '\0') {
            show_menu();
            usrptr->substt = ST_MENU;
        }
        else {
            prfmsg(BADSEL);
            prfmsg(MODPRM);
        }
        break;

    case ST_CFG:
        n = parse_num(in);
        if (n == 1 || n == CM_AD) {
            sgrp[usrnum] = (n == 1) ? CFG_GEN : CFG_AD;
            show_settings(sgrp[usrnum]);
            usrptr->substt = ST_SET;
        }
        else if (n >= TL_ITEMS && n <= TL_MODS) {
            togl[usrnum] = (INT)n;
            show_toggles((INT)n);
            usrptr->substt = ST_TOG;
        }
        else if (sameas((CHAR *)in, "X") || *in == '\0') {
            show_menu();
            usrptr->substt = ST_MENU;
        }
        else {
            prfmsg(BADSEL);
            prfmsg(CFGPRM);
        }
        break;

    case ST_SET:
        g = sgrp[usrnum];
        if (sameas((CHAR *)in, "X") || *in == '\0') {
            prfmsg(CFGMNU);
            prfmsg(CFGPRM);
            usrptr->substt = ST_CFG;
            break;
        }
        if (g == CFG_AD && sameas((CHAR *)in, "P")) {
            show_ad(TRUE);           /* exactly what callers will see        */
            set_prompt(g);
            break;
        }
        n = parse_num(in);
        if (n < 1 || n > cfg_count(g)) {
            prfmsg(BADSEL);
            set_prompt(g);
            break;
        }
        pick[usrnum] = (INT)n;
        tvar_set(TV_LABEL, cfg_label(g, (INT)n));
        tvar_set(TV_VALUE, cfg_valtext(g, (INT)n, val, sizeof(val)));
        tvar_set(TV_RANGE, cfg_range(g, (INT)n));
        prfmsg(SETASK);
        usrptr->substt = ST_SETVAL;
        break;

    case ST_SETVAL:
        g = sgrp[usrnum];
        if (*in == '\0' || sameas((CHAR *)in, "X")) {
            show_settings(g);        /* changed their mind - nothing changes */
            usrptr->substt = ST_SET;
            break;
        }
        if (cfg_istext(g, pick[usrnum])) {
            rstrin();                /* the whole line, spaces and all       */
            in = margv[0];           /*   (MAJORBBS.C rstrin)                */
        }
        if (!cfg_set(g, pick[usrnum], in)) {
            tvar_set(TV_RANGE, cfg_range(g, pick[usrnum]));
            prfmsg(SETBAD);
            break;
        }
        tvar_set(TV_LABEL, cfg_label(g, pick[usrnum]));
        tvar_set(TV_VALUE, cfg_valtext(g, pick[usrnum], val, sizeof(val)));
        audit(cfg_label(g, pick[usrnum]), val);
        prfmsg(SETOK);
        show_settings(g);
        usrptr->substt = ST_SET;
        break;

    case ST_TOG:
        if (sameas((CHAR *)in, "X") || *in == '\0') {
            prfmsg(CFGMNU);
            prfmsg(CFGPRM);
            usrptr->substt = ST_CFG;
            break;
        }
        do_toggle(togl[usrnum], in);
        break;

    case ST_DELOK:
        if (sameas((CHAR *)in, "Y") || sameas((CHAR *)in, "YES")) {
            tvar_set(TV_LABEL, delnm[usrnum]);
            switch (trk_delete(delnm[usrnum])) {
            case SBDEL_BUSY:
                prfmsg(DELBUSY);
                break;
            case SBDEL_RESET:
                audit(delnm[usrnum], "statistics deleted");
                prfmsg(DELRST);
                break;
            default:
                audit(delnm[usrnum], "statistics deleted");
                prfmsg(DELGONE);
                break;
            }
        }
        else
            prfmsg(DELNO);
        show_toggles(TL_MODS);
        usrptr->substt = ST_TOG;
        break;
    }

    outprf(usrnum);
    rstmbk();
    return 1;
}

/* ---------------------------------------------------------------------- *
 * sb_delacct                                                             *
 *                                                                        *
 * An account was deleted: remove that user's per-module records. Totals  *
 * already counted stay, since they record what happened at the time.     *
 * ---------------------------------------------------------------------- */
VOID EXPORT
sb_delacct(CHAR *userid)
{
    st_use_delete_user(userid);
}

/* ---------------------------------------------------------------------- *
 * sb_shutdown                                                            *
 *                                                                        *
 * Clean shutdown (including the nightly restart): save any time still    *
 * being measured, then close everything.                                 *
 * ---------------------------------------------------------------------- */
VOID EXPORT
sb_shutdown(VOID)
{
    trk_stop();
    st_close();
    clsmsg(sbmb);
}

VOID EXPORT
end_snbbss(VOID)                     /* marks the end of the code, to help   */
{                                    /* read crash reports                   */
}
