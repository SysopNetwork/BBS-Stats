/* 26.10.20.1 12:15AM */
/*****************************************************************************
 *   SBCFG.H                                    BBS Stats - Live settings    *
 *                                                                           *
 *   Every setting is changed ONLINE, from the sysop configuration screens  *
 *   inside the module, and takes effect immediately - no restart and no    *
 *   CNF options. The values live in the settings record of SNBBSS.DAT; the *
 *   defaults below seed a new install. Callers read settings through these *
 *   getters at the moment they need them.                                   *
 *                                                                           *
 *   Copyright (C) 2026 Sysop Network.  All Rights Reserved.                 *
 *****************************************************************************/

#ifndef SBCFG_H
#define SBCFG_H

#include "gcomm.h"

/* Windows a stats page can show. The first three match SBDATE.H; the       */
/* fourth is all-time.                                                      */
#define SBP_DAY     0
#define SBP_WEEK    1
#define SBP_MONTH   2
#define SBP_ALL     3
#define SBP_COUNT   4

/* Module page sort orders.                                                 */
#define SBS_TIME    1
#define SBS_USERS   2
#define SBS_NAME    3

/* Module page columns, each of which the sysop can hide.                   */
#define SBC_USERS   0
#define SBC_VISITS  1
#define SBC_TIME    2
#define SBC_AVG     3
#define SBC_COUNT   4

/* What the logon advertisement ranks modules by.                          */
#define SBR_TIME    1
#define SBR_USERS   2
#define SBR_VISITS  3

/* Logon advertisement layouts.                                             */
#define SBL_TABLE   1                /* one ranked module per line           */
#define SBL_TWOCOL  2                /* ranked, in two columns               */
#define SBL_COMPACT 3                /* a running list, wrapped              */

/* The settings screens, each a numbered list of rows.                      */
#define CFG_GEN     1                /* general settings                     */
#define CFG_AD      2                /* logon advertisement                  */

VOID cfg_open(VOID);                 /* load, or seed the defaults           */

/* ---- live getters ---------------------------------------------------- */
GBOOL       cfg_cntsys(VOID);        /* count time used by sysops?           */
INT         cfg_defwin(VOID);        /* default page window (SBP_*)          */
INT         cfg_sortby(VOID);        /* module page order (SBS_*)            */
GBOOL       cfg_weekmon(VOID);       /* weeks start on Monday?               */
GBOOL       cfg_colshown(INT col);   /* module page column shown? (SBC_*)    */
GBOOL       cfg_itemshown(INT item); /* BBS page item shown? (SBBBS.H)       */
const CHAR *cfg_bbskey(VOID);        /* key needed for the BBS page, or ""   */
const CHAR *cfg_modkey(VOID);        /* key needed for the module page, or ""*/
GBOOL       cfg_adon(VOID);          /* show the ad at logon?                */
INT         cfg_adrank(VOID);        /* what the ad ranks by (SBR_*)         */
INT         cfg_adwin(VOID);         /* the ad's window (SBP_*)              */
INT         cfg_adcount(VOID);       /* how many modules the ad lists        */
INT         cfg_adstyle(VOID);       /* the ad's layout (SBL_*)              */
GBOOL       cfg_adfigs(VOID);        /* show the figures next to each name?  */
const CHAR *cfg_adtitle(VOID);       /* the ad's headline                    */
const CHAR *cfg_adkey(VOID);         /* key needed to see the ad, or ""      */
const CHAR *cfg_adnokey(VOID);       /* key that hides the ad, or ""         */
const CHAR *cfg_bbscmd(VOID);        /* BBS page command, or "" if off       */
const CHAR *cfg_modcmd(VOID);        /* module page command, or "" if off    */

/* ---- the settings editor: screen grp (CFG_*), 1-based rows ------------ */
INT         cfg_count(INT grp);
const CHAR *cfg_label(INT grp, INT idx);
CHAR       *cfg_valtext(INT grp, INT idx, CHAR *buf, INT len); /* value now  */
const CHAR *cfg_range(INT grp, INT idx);                       /* accepts    */
GBOOL       cfg_istext(INT grp, INT idx);  /* takes a whole line of text?    */

/* Apply what the sysop typed for a row. Persists and takes effect at once. */
/* Returns FALSE (changing nothing) if the input is not acceptable.         */
GBOOL cfg_set(INT grp, INT idx, const CHAR *input);

/* ---- toggles for the column and item screens -------------------------- */
VOID cfg_setcol(INT col, GBOOL shown);
VOID cfg_setitem(INT item, GBOOL shown);

#endif /* SBCFG_H */
