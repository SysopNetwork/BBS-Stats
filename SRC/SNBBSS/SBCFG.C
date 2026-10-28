/* 26.10.20.1 12:15AM */
/*****************************************************************************
 *   SBCFG.C                                    BBS Stats - Live settings    *
 *                                                                           *
 *   See SBCFG.H. The record is held in memory and written through to       *
 *   SNBBSS.DAT on every change, so the in-memory copy is always the stored *
 *   one - every reader sees a change the moment the sysop makes it.        *
 *                                                                           *
 *   Copyright (C) 2026 Sysop Network.  All Rights Reserved.                 *
 *****************************************************************************/

#include <ctype.h>
#include <stdio.h>

#include "gcomm.h"
#include "majorbbs.h"

#include "SBSTORE.H"
#include "SBCFG.H"

static struct sbcfg cfg;             /* the settings, as stored              */

/* Every editable setting, grouped by the screen it appears on and in      */
/* on-screen order within it. Rows with lo > hi take a key name; rows with */
/* lo == hi == 0 take a line of free text.                                  */
#define R_CNTSYS    1
#define R_DEFWIN    2
#define R_SORTBY    3
#define R_WEEKMON   4
#define R_BBSKEY    5
#define R_MODKEY    6
#define R_ADON      7
#define R_ADRANK    8
#define R_ADWIN     9
#define R_ADCOUNT   10
#define R_ADSTYLE   11
#define R_ADFIGS    12
#define R_ADTITLE   13
#define R_ADKEY     14
#define R_ADNOKEY   15
#define R_BBSCMD    16
#define R_MODCMD    17

struct cfgrow {
    INT grp;                         /* screen it is on (CFG_*)              */
    INT id;                          /* which setting (R_*)                  */
    const CHAR *label;
    LONG lo, hi;
    const CHAR *range;               /* shown to the sysop when editing      */
};
static const struct cfgrow rows[] = {
    { CFG_GEN, R_CNTSYS,  "Count time used by sysops",       0, 1, "0 = no, 1 = yes" },
    { CFG_GEN, R_DEFWIN,  "Default time window",             1, 4, "1 = today, 2 = week, 3 = month, 4 = all-time" },
    { CFG_GEN, R_SORTBY,  "Module stats page order",         1, 3, "1 = most time, 2 = most users, 3 = by name" },
    { CFG_GEN, R_WEEKMON, "Weeks start on",                  0, 1, "0 = Sunday, 1 = Monday" },
    { CFG_GEN, R_BBSKEY,  "Key needed to view BBS stats",    1, 0, "a key name, or NONE for everyone" },
    { CFG_GEN, R_MODKEY,  "Key needed to view module stats", 1, 0, "a key name, or NONE for everyone" },
    { CFG_GEN, R_BBSCMD,  "Command for BBS stats",           1, 0, "a command such as /BBSSTATS, or NONE for off" },
    { CFG_GEN, R_MODCMD,  "Command for module stats",        1, 0, "a command such as /MODSTATS, or NONE for off" },
    { CFG_AD,  R_ADON,    "Show the ad at logon",            0, 1, "0 = no, 1 = yes" },
    { CFG_AD,  R_ADKEY,   "Key needed to see the ad",        1, 0, "a key name, or NONE for everyone" },
    { CFG_AD,  R_ADNOKEY, "Key that hides the ad",           1, 0, "a key name, or NONE for no one" },
    { CFG_AD,  R_ADRANK,  "Rank modules by",                 1, 3, "1 = time played, 2 = users, 3 = visits" },
    { CFG_AD,  R_ADWIN,   "Time window",                     1, 4, "1 = today, 2 = week, 3 = month, 4 = all-time" },
    { CFG_AD,  R_ADCOUNT, "How many modules",                1, SBAD_MAX, "1 to 20" },
    { CFG_AD,  R_ADSTYLE, "Layout",                          1, 3, "1 = table, 2 = two columns, 3 = compact list" },
    { CFG_AD,  R_ADFIGS,  "Show the figures",                0, 1, "0 = names only, 1 = names and figures" },
    { CFG_AD,  R_ADTITLE, "Headline",                        0, 0, "up to 60 characters, or DEFAULT" },
};
#define NROWS ((INT)(sizeof(rows) / sizeof(rows[0])))

#define DEF_ADTITLE "The most popular modules on this BBS"

/* The global commands' starting names. /STATS is not one of them: ML      */
/* already has a /STATS command, and whichever module loads first would   */
/* take it from the other.                                                  */
#define DEF_BBSCMD  "/BBSSTATS"
#define DEF_MODCMD  "/MODSTATS"

/* ---------------------------------------------------------------------- *
 * cfg_defaults (internal)                                                *
 *                                                                        *
 * The settings a brand-new install starts with. This table and the       *
 * range checks in cfg_open() are the only places defaults live.          *
 * ---------------------------------------------------------------------- */
static VOID
cfg_defaults(VOID)
{
    setmem((CHAR *)&cfg, sizeof(cfg), 0);
    cfg.id       = SBREC_CFG;
    cfg.ver      = SBCFG_VER;
    cfg.cntsys   = 0;                /* match the engine's own module hours  */
    cfg.defwin   = 4;                /* all-time                             */
    cfg.sortby   = SBS_TIME;
    cfg.weekmon  = 0;                /* weeks start on Sunday                */
    cfg.colhide  = 0;                /* every column shown                   */
    cfg.itemhide = 0;                /* every BBS page item shown            */
    cfg.adon     = 1;                /* the ad shows until switched off      */
    stzcpy(cfg.bbscmd, DEF_BBSCMD, SBCMDSZ);
    stzcpy(cfg.modcmd, DEF_MODCMD, SBCMDSZ);
}

VOID
cfg_open(VOID)
{
    if (!st_cfg_load(&cfg)) {
        cfg_defaults();
        st_cfg_save(&cfg);
    }
    /* Before version 3 the ad was off until the sysop switched it on;     */
    /* from version 3 it is on by default, so an older record turns it on. */
    if (cfg.ver < 3)
        cfg.adon = 1;
    /* Before version 4 there were no command names to store.              */
    if (cfg.ver < 4) {
        stzcpy(cfg.bbscmd, DEF_BBSCMD, SBCMDSZ);
        stzcpy(cfg.modcmd, DEF_MODCMD, SBCMDSZ);
    }
    /* Later layout versions only ever add fields carved from spare bytes,  */
    /* where zero means the old behaviour, so any stored version is usable  */
    /* as-is. Never discard a sysop's settings on upgrade.                  */
    if (cfg.defwin < 1 || cfg.defwin > 4)
        cfg.defwin = 4;
    if (cfg.sortby < SBS_TIME || cfg.sortby > SBS_NAME)
        cfg.sortby = SBS_TIME;
    cfg.bbskey[KEYSIZ - 1] = '\0';
    cfg.modkey[KEYSIZ - 1] = '\0';

    /* The ad's fields are zero in a version 1 record; zero (or anything    */
    /* out of range) means the default.                                     */
    if (cfg.adrank < SBR_TIME || cfg.adrank > SBR_VISITS)
        cfg.adrank = SBR_TIME;
    if (cfg.adwin < 1 || cfg.adwin > 4)
        cfg.adwin = 2;               /* this week                            */
    if (cfg.adcount < 1 || cfg.adcount > SBAD_MAX)
        cfg.adcount = 10;
    if (cfg.adstyle < SBL_TABLE || cfg.adstyle > SBL_COMPACT)
        cfg.adstyle = SBL_TABLE;
    cfg.adtitle[SBAD_TITLE - 1] = '\0';
    cfg.adkey[KEYSIZ - 1] = '\0';
    cfg.adnokey[KEYSIZ - 1] = '\0';
    cfg.bbscmd[SBCMDSZ - 1] = '\0';
    cfg.modcmd[SBCMDSZ - 1] = '\0';
    if (cfg.adtitle[0] == '\0')
        stzcpy(cfg.adtitle, DEF_ADTITLE, SBAD_TITLE);
    if (cfg.ver < SBCFG_VER) {       /* an upgrade: store it once            */
        cfg.ver = SBCFG_VER;
        st_cfg_save(&cfg);
    }
}

/* ---- live getters ---------------------------------------------------- */
GBOOL       cfg_cntsys(VOID)  { return cfg.cntsys != 0;  }
INT         cfg_defwin(VOID)  { return cfg.defwin - 1;   }
INT         cfg_sortby(VOID)  { return cfg.sortby;       }
GBOOL       cfg_weekmon(VOID) { return cfg.weekmon != 0; }
const CHAR *cfg_bbskey(VOID)  { return cfg.bbskey;       }
const CHAR *cfg_modkey(VOID)  { return cfg.modkey;       }
GBOOL       cfg_adon(VOID)    { return cfg.adon != 0;    }
INT         cfg_adrank(VOID)  { return cfg.adrank;       }
INT         cfg_adwin(VOID)   { return cfg.adwin - 1;    }
INT         cfg_adcount(VOID) { return cfg.adcount;      }
INT         cfg_adstyle(VOID) { return cfg.adstyle;      }
GBOOL       cfg_adfigs(VOID)  { return cfg.adnofig == 0; }
const CHAR *cfg_adtitle(VOID) { return cfg.adtitle;      }
const CHAR *cfg_adkey(VOID)   { return cfg.adkey;        }
const CHAR *cfg_adnokey(VOID) { return cfg.adnokey;      }
const CHAR *cfg_bbscmd(VOID)  { return cfg.bbscmd;       }
const CHAR *cfg_modcmd(VOID)  { return cfg.modcmd;       }

GBOOL
cfg_colshown(INT col)
{
    return col >= 0 && col < SBC_COUNT && !(cfg.colhide & (1 << col));
}

GBOOL
cfg_itemshown(INT item)
{
    return item >= 0 && item < 32 && !(cfg.itemhide & (1UL << item));
}

/* ---- the settings editor --------------------------------------------- */

/* Row idx (1-based) of screen grp, or NULL if there is no such row.       */
static const struct cfgrow *
findrow(INT grp, INT idx)
{
    INT i, n = 0;

    for (i = 0; i < NROWS; i++)
        if (rows[i].grp == grp && ++n == idx)
            return &rows[i];
    return NULL;
}

INT
cfg_count(INT grp)
{
    INT i, n = 0;

    for (i = 0; i < NROWS; i++)
        if (rows[i].grp == grp)
            n++;
    return n;
}

const CHAR *
cfg_label(INT grp, INT idx)
{
    const struct cfgrow *r = findrow(grp, idx);

    return r != NULL ? r->label : "";
}

const CHAR *
cfg_range(INT grp, INT idx)
{
    const struct cfgrow *r = findrow(grp, idx);

    return r != NULL ? r->range : "";
}

GBOOL
cfg_istext(INT grp, INT idx)
{
    const struct cfgrow *r = findrow(grp, idx);

    return r != NULL && r->lo == 0 && r->hi == 0;
}

CHAR *
cfg_valtext(INT grp, INT idx, CHAR *buf, INT len)
{
    static const CHAR *winnm[] = { "", "today", "this week", "this month", "all-time" };
    static const CHAR *sortnm[] = { "", "most time", "most users", "by name" };
    static const CHAR *ranknm[] = { "", "time played", "users", "visits" };
    static const CHAR *stylnm[] = { "", "table", "two columns", "compact list" };
    const struct cfgrow *r = findrow(grp, idx);
    CHAR num[8];
    const CHAR *v = "";

    switch (r != NULL ? r->id : 0) {
    case R_CNTSYS:  v = cfg.cntsys ? "yes" : "no";              break;
    case R_DEFWIN:  v = winnm[cfg.defwin];                      break;
    case R_SORTBY:  v = sortnm[cfg.sortby];                     break;
    case R_WEEKMON: v = cfg.weekmon ? "Monday" : "Sunday";      break;
    case R_BBSKEY:  v = cfg.bbskey[0] ? cfg.bbskey : "NONE (everyone)"; break;
    case R_MODKEY:  v = cfg.modkey[0] ? cfg.modkey : "NONE (everyone)"; break;
    case R_ADON:    v = cfg.adon ? "yes" : "no";                break;
    case R_ADRANK:  v = ranknm[cfg.adrank];                     break;
    case R_ADWIN:   v = winnm[cfg.adwin];                       break;
    case R_ADCOUNT: sprintf(num, "%d", (INT)cfg.adcount); v = num; break;
    case R_ADSTYLE: v = stylnm[cfg.adstyle];                    break;
    case R_ADFIGS:  v = cfg.adnofig ? "no" : "yes";             break;
    case R_ADTITLE: v = cfg.adtitle;                            break;
    case R_ADKEY:   v = cfg.adkey[0] ? cfg.adkey : "NONE (everyone)";  break;
    case R_ADNOKEY: v = cfg.adnokey[0] ? cfg.adnokey : "NONE (no one)"; break;
    case R_BBSCMD:  v = cfg.bbscmd[0] ? cfg.bbscmd : "NONE (off)";     break;
    case R_MODCMD:  v = cfg.modcmd[0] ? cfg.modcmd : "NONE (off)";     break;
    }
    stzcpy(buf, v, len);
    return buf;
}

/* ---------------------------------------------------------------------- *
 * setkey (internal)                                                      *
 *                                                                        *
 * Validate and store a key name: letters and digits only, shorter than   *
 * the BBS's key size, stored in upper case. NONE clears it.              *
 * ---------------------------------------------------------------------- */
static GBOOL
setkey(CHAR *dst, const CHAR *input)
{
    CHAR tmp[KEYSIZ];
    INT i;

    if (sameas((CHAR *)input, "NONE")) {
        dst[0] = '\0';
        return TRUE;
    }
    for (i = 0; input[i] != '\0'; i++) {
        if (i >= KEYSIZ - 1 || !isalnum(input[i]))
            return FALSE;
        tmp[i] = (CHAR)toupper(input[i]);
    }
    if (i == 0)
        return FALSE;
    tmp[i] = '\0';
    stzcpy(dst, tmp, KEYSIZ);
    return TRUE;
}

/* ---------------------------------------------------------------------- *
 * setcmd (internal)                                                      *
 *                                                                        *
 * Validate and store a global command name: a slash (added if left off)  *
 * and letters or digits, stored in upper case. It may not be the other  *
 * command's name, nor one of the BBS's own global commands, which are    *
 * offered each line before any module's (MAJORBBS.C glorys, PAGE.C       *
 * gloPage, MENUING.C globalgo). NONE switches the command off.           *
 * ---------------------------------------------------------------------- */
static GBOOL
setcmd(CHAR *dst, const CHAR *other, const CHAR *input)
{
    static const CHAR *engine[] = { "/GO", "/P", "/RECENT", "/L", "/INVIS", NULL };
    CHAR tmp[SBCMDSZ];
    INT i, n = 1;

    if (sameas((CHAR *)input, "NONE")) {
        dst[0] = '\0';
        return TRUE;
    }
    tmp[0] = '/';
    if (*input == '/')
        input++;
    for (; *input != '\0'; input++) {
        if (n >= SBCMDSZ - 1 || !isalnum((UCHAR)*input))
            return FALSE;
        tmp[n++] = (CHAR)toupper((UCHAR)*input);
    }
    if (n == 1)
        return FALSE;
    tmp[n] = '\0';
    if (other[0] != '\0' && sameas(tmp, (CHAR *)other))
        return FALSE;
    for (i = 0; engine[i] != NULL; i++)
        if (sameas(tmp, (CHAR *)engine[i]))
            return FALSE;
    stzcpy(dst, tmp, SBCMDSZ);
    return TRUE;
}

/* ---------------------------------------------------------------------- *
 * settitle (internal)                                                    *
 *                                                                        *
 * Validate and store the ad's headline: printable characters only (no    *
 * control codes, so no escape sequences), outer blanks dropped, at most  *
 * SBAD_TITLE-2 characters. DEFAULT restores the stock headline.          *
 * ---------------------------------------------------------------------- */
static GBOOL
settitle(const CHAR *input)
{
    CHAR tmp[SBAD_TITLE];
    INT n;

    while (*input == ' ')
        input++;
    if (sameas((CHAR *)input, "DEFAULT")) {
        stzcpy(cfg.adtitle, DEF_ADTITLE, SBAD_TITLE);
        return TRUE;
    }
    for (n = 0; input[n] != '\0'; n++) {
        if (n >= SBAD_TITLE - 2 || (UCHAR)input[n] < 32 || (UCHAR)input[n] > 126)
            return FALSE;
        tmp[n] = input[n];
    }
    while (n > 0 && tmp[n - 1] == ' ')
        n--;
    if (n == 0)
        return FALSE;
    tmp[n] = '\0';
    stzcpy(cfg.adtitle, tmp, SBAD_TITLE);
    return TRUE;
}

GBOOL
cfg_set(INT grp, INT idx, const CHAR *input)
{
    const struct cfgrow *r = findrow(grp, idx);
    LONG v = 0;
    INT i;

    if (r == NULL)
        return FALSE;

    if (r->id == R_BBSKEY || r->id == R_MODKEY) {
        if (!setkey(r->id == R_BBSKEY ? cfg.bbskey : cfg.modkey, input))
            return FALSE;
    }
    else if (r->id == R_ADKEY || r->id == R_ADNOKEY) {
        if (!setkey(r->id == R_ADKEY ? cfg.adkey : cfg.adnokey, input))
            return FALSE;
    }
    else if (r->id == R_BBSCMD) {
        if (!setcmd(cfg.bbscmd, cfg.modcmd, input))
            return FALSE;
    }
    else if (r->id == R_MODCMD) {
        if (!setcmd(cfg.modcmd, cfg.bbscmd, input))
            return FALSE;
    }
    else if (r->id == R_ADTITLE) {
        if (!settitle(input))
            return FALSE;
    }
    else {
        /* digits only, within the row's range                              */
        if (input[0] == '\0')
            return FALSE;
        for (i = 0; input[i] != '\0'; i++) {
            if (!isdigit(input[i]) || i > 6)
                return FALSE;
            v = v * 10 + (input[i] - '0');
        }
        if (v < r->lo || v > r->hi)
            return FALSE;
        switch (r->id) {
        case R_CNTSYS:  cfg.cntsys  = (UCHAR)v;           break;
        case R_DEFWIN:  cfg.defwin  = (UCHAR)v;           break;
        case R_SORTBY:  cfg.sortby  = (UCHAR)v;           break;
        case R_WEEKMON: cfg.weekmon = (UCHAR)v;           break;
        case R_ADON:    cfg.adon    = (UCHAR)v;           break;
        case R_ADRANK:  cfg.adrank  = (UCHAR)v;           break;
        case R_ADWIN:   cfg.adwin   = (UCHAR)v;           break;
        case R_ADCOUNT: cfg.adcount = (UCHAR)v;           break;
        case R_ADSTYLE: cfg.adstyle = (UCHAR)v;           break;
        case R_ADFIGS:  cfg.adnofig = (UCHAR)(v ? 0 : 1); break;
        }
    }
    st_cfg_save(&cfg);               /* persisted and live at once           */
    return TRUE;
}

/* ---- toggles --------------------------------------------------------- */
VOID
cfg_setcol(INT col, GBOOL shown)
{
    if (col < 0 || col >= SBC_COUNT)
        return;
    if (shown)
        cfg.colhide &= (UCHAR)~(1 << col);
    else
        cfg.colhide |= (UCHAR)(1 << col);
    st_cfg_save(&cfg);
}

VOID
cfg_setitem(INT item, GBOOL shown)
{
    if (item < 0 || item >= 32)
        return;
    if (shown)
        cfg.itemhide &= ~(1UL << item);
    else
        cfg.itemhide |= (1UL << item);
    st_cfg_save(&cfg);
}
