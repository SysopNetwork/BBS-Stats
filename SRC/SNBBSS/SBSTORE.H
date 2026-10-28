/* 26.10.27.1 09:44PM */
/*****************************************************************************
 *   SBSTORE.H                                  BBS Stats - Data files       *
 *                                                                           *
 *   The module keeps three Btrieve files, created on first run:             *
 *                                                                           *
 *   SNBBSS.DAT   Two singleton records: the sysop's settings (id 1) and     *
 *                the BBS-wide counters (id 2) - logons, new users, peaks.   *
 *   SNBBSSM.DAT  One summary record per module, keyed by module name:      *
 *                distinct users, time and visits - all-time and for the    *
 *                current day, week and month - plus the sysop's "show this *
 *                module on the stats page" choice.                          *
 *   SNBBSSU.DAT  One record per (module, user) pair. It is what makes the  *
 *                "distinct users" counts possible: the first time a user's *
 *                record is touched in a new window, that window's user     *
 *                count in the module summary goes up by one.                *
 *                                                                           *
 *   Records are laid out with 1-byte packing so their on-disk size never   *
 *   depends on compiler padding. Every record carries spare bytes; new     *
 *   fields are carved out of them so existing files keep working, and a    *
 *   zero in a new field always means "the old behaviour".                  *
 *                                                                           *
 *   Copyright (C) 2026 Sysop Network.  All Rights Reserved.                 *
 *****************************************************************************/

#ifndef SBSTORE_H
#define SBSTORE_H

#include "gcomm.h"
#include "majorbbs.h"
#include "SBDATE.H"

#pragma pack(push, 1)

/* ---- SNBBSS.DAT -------------------------------------------------------- */

#define SBREC_CFG   1                /* id of the settings record            */
#define SBREC_CTR   2                /* id of the BBS counters record        */
#define SBSYSLEN    256              /* fixed length of both record kinds    */
#define SBAD_TITLE  62               /* ad headline, NUL included            */
#define SBAD_MAX    20               /* most modules the ad can list         */
#define SBCMDSZ     16               /* a global command name, NUL included  */

struct sbcfg {                       /* sysop settings (id SBREC_CFG)        */
    USHORT id;                       /*   key 0: SBREC_CFG                   */
    USHORT ver;                      /*   layout version (SBCFG_VER)         */
    UCHAR  cntsys;                   /*   count time of SYSOP-key users?     */
    UCHAR  defwin;                   /*   default window 1=day 2=wk 3=mo 4=all*/
    UCHAR  sortby;                   /*   module page order 1=time 2=users 3=name */
    UCHAR  weekmon;                  /*   weeks start Monday (else Sunday)   */
    UCHAR  colhide;                  /*   module page columns hidden (bits)  */
    UCHAR  pad[3];                   /*   stays zero                         */
    ULONG  itemhide;                 /*   BBS page items hidden (bits)       */
    CHAR   bbskey[KEYSIZ];           /*   key to view the BBS page ("" = all) */
    CHAR   modkey[KEYSIZ];           /*   key to view the module page        */
    /* -- layout version 2: the logon advertisement. A zero in any of     */
    /*    these means "the default", so a version 1 record reads as the    */
    /*    ad switched off with every other choice at its default.          */
    UCHAR  adon;                     /*   show the ad at logon?              */
    UCHAR  adrank;                   /*   rank by 1=time 2=users 3=visits    */
    UCHAR  adwin;                    /*   window 1=day 2=wk 3=mo 4=all       */
    UCHAR  adcount;                  /*   how many modules (1-SBAD_MAX)      */
    UCHAR  adstyle;                  /*   layout 1=table 2=two col 3=compact */
    UCHAR  adnofig;                  /*   1 = leave the figures off          */
    CHAR   adtitle[SBAD_TITLE];      /*   headline ("" = the default)        */
    /* -- layout version 3: who sees the ad. A record older than version  */
    /*    3 is switched on when loaded, since the ad now defaults to on.   */
    CHAR   adkey[KEYSIZ];            /*   key needed to see the ad ("" = all)*/
    CHAR   adnokey[KEYSIZ];          /*   key that hides the ad ("" = none)  */
    /* -- layout version 4: the global command names. A record older than */
    /*    version 4 gets the default names when loaded; after that "" is   */
    /*    a command the sysop switched off.                                 */
    CHAR   bbscmd[SBCMDSZ];          /*   BBS stats page, e.g. "/BBSSTATS"   */
    CHAR   modcmd[SBCMDSZ];          /*   module stats page, e.g. "/MODSTATS"*/
    UCHAR  spare[SBSYSLEN - 48 - 6 - SBAD_TITLE - 2 * KEYSIZ - 2 * SBCMDSZ]; /* future */
};
#define SBCFG_VER   4

struct sbctrw {                      /* one window of the BBS counters       */
    USHORT stamp;                    /*   window these counts belong to      */
    USHORT peak;                     /*   most users online at once          */
    ULONG  logons;                   /*   logons counted                     */
    ULONG  newusr;                   /*   new accounts created               */
};

struct sbctr {                       /* BBS counters (id SBREC_CTR)          */
    USHORT id;                       /*   key 0: SBREC_CTR                   */
    USHORT ver;                      /*   layout version (SBCTR_VER)         */
    USHORT since;                    /*   packed date counting started       */
    USHORT peakdt;                   /*   packed date of the all-time peak   */
    USHORT peak;                     /*   all-time most users online at once */
    USHORT pad;                      /*   stays zero                         */
    ULONG  logons;                   /*   logons since counting started      */
    ULONG  newusr;                   /*   new accounts since counting started*/
    struct sbctrw win[SBW_COUNT];    /*   today / this week / this month     */
    UCHAR  spare[SBSYSLEN - 20 - 12 * SBW_COUNT];
};
#define SBCTR_VER   1

/* ---- SNBBSSM.DAT ------------------------------------------------------- */

struct sbwin {                       /* one window of a module summary       */
    USHORT stamp;                    /*   window these counts belong to      */
    USHORT pad;                      /*   stays zero                         */
    ULONG  users;                    /*   distinct users                     */
    ULONG  secs;                     /*   seconds spent                      */
    ULONG  visits;                   /*   times the module was entered       */
};

struct sbmod {                       /* per-module summary                   */
    CHAR   name[MNMSIZ];             /*   key 0: module name (NUL padded)    */
    UCHAR  hidden;                   /*   1 = left off the module stats page */
    USHORT first;                    /*   packed date first seen             */
    USHORT last;                     /*   packed date last used              */
    ULONG  users;                    /*   all-time distinct users            */
    ULONG  secs;                     /*   all-time seconds                   */
    ULONG  visits;                   /*   all-time visits                    */
    struct sbwin win[SBW_COUNT];     /*   today / this week / this month     */
    UCHAR  notrack;                  /*   1 = not tracked                    */
    UCHAR  spare[37];                /*   room for future fields (zero)      */
};

/* ---- SNBBSSU.DAT ------------------------------------------------------- */

struct sbuwin {                      /* one window of a user's module usage  */
    USHORT stamp;                    /*   window these counts belong to      */
    ULONG  secs;                     /*   seconds spent                      */
    ULONG  visits;                   /*   times entered                      */
};

struct sbuse {                       /* one user's use of one module         */
    CHAR   name[MNMSIZ];             /*   key 0 (with userid): module name   */
    CHAR   userid[UIDSIZ];           /*   key 1 (duplicates): the user       */
    USHORT first;                    /*   packed date first used             */
    USHORT last;                     /*   packed date last used              */
    ULONG  secs;                     /*   all-time seconds                   */
    ULONG  visits;                   /*   all-time visits                    */
    struct sbuwin win[SBW_COUNT];    /*   today / this week / this month     */
    UCHAR  spare[16];                /*   room for future fields (zero)      */
};

#pragma pack(pop)

/* Catch accidental layout changes at compile time: these sizes are what    */
/* is on disk at every installed board.                                     */
typedef char sb_cfg_size_check[(sizeof(struct sbcfg) == SBSYSLEN) ? 1 : -1];
typedef char sb_ctr_size_check[(sizeof(struct sbctr) == SBSYSLEN) ? 1 : -1];
typedef char sb_mod_size_check[(sizeof(struct sbmod) == 128) ? 1 : -1];
typedef char sb_use_size_check[(sizeof(struct sbuse) == 113) ? 1 : -1];

/* Open the three files, creating any that do not exist yet.                */
VOID st_open(VOID);
VOID st_close(VOID);

/* Settings and counters singletons. Load returns FALSE if not stored yet.  */
GBOOL st_cfg_load(struct sbcfg *cfg);
VOID  st_cfg_save(struct sbcfg *cfg);
GBOOL st_ctr_load(struct sbctr *ctr);
VOID  st_ctr_save(struct sbctr *ctr);

/* Module summaries. Get returns FALSE if the module has no summary yet.    */
GBOOL st_mod_get(const CHAR *name, struct sbmod *mod);
VOID  st_mod_put(struct sbmod *mod);
VOID  st_mod_delete(const CHAR *name);

/* Walk every module summary (file order): start with first=TRUE, then     */
/* FALSE, until it returns FALSE. Read-only; do not write while walking.    */
GBOOL st_mod_walk(struct sbmod *mod, GBOOL first);

/* A user's record for one module. Get returns FALSE if none exists yet.    */
GBOOL st_use_get(const CHAR *name, const CHAR *userid, struct sbuse *use);
VOID  st_use_put(struct sbuse *use);

/* Remove every module record belonging to a deleted account.               */
VOID  st_use_delete_user(const CHAR *userid);

/* Remove up to max of one module's per-user records. Returns TRUE while    */
/* some remain, so a big module can be cleared a batch at a time.           */
GBOOL st_use_delete_mod(const CHAR *name, INT max);

#endif /* SBSTORE_H */
