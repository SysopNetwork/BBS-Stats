/* 26.10.19.2 12:30PM */
/*****************************************************************************
 *   SBSTORE.C                                  BBS Stats - Data files       *
 *                                                                           *
 *   All Btrieve access lives here (see SBSTORE.H for the file layouts).     *
 *                                                                           *
 *   Rules this file follows, each learned the hard way on other modules:   *
 *   - Lookup keys are always copied into a local (writable) buffer. The    *
 *     data-file layer silently searches an EMPTY key when handed a string  *
 *     literal or other read-only memory.                                   *
 *   - Key buffers are the full field width, zero-filled, exactly as the    *
 *     key is stored.                                                       *
 *   - An update or delete immediately follows the acquire that found the   *
 *     record; after a step or query it would be silently discarded.        *
 *   - Inserts and updates use the "Dup" forms, which report an error       *
 *     instead of halting the whole board.                                  *
 *                                                                           *
 *   Copyright (C) 2026 Sysop Network.  All Rights Reserved.                 *
 *****************************************************************************/

#include "gcomm.h"
#include "majorbbs.h"

#include "SBSTORE.H"

#define SYSFIL  "SNBBSS.DAT"
#define MODFIL  "SNBBSSM.DAT"
#define USEFIL  "SNBBSSU.DAT"
#define PAGESZ  1024                 /* Btrieve page size (matches the MDF)  */

static DFAFILE *sysdat;              /* settings + counters                  */
static DFAFILE *moddat;              /* module summaries                     */
static DFAFILE *usedat;              /* per-user module records              */

/* A (module, userid) key exactly as it sits at the front of struct sbuse.  */
struct usekey {
    CHAR name[MNMSIZ];
    CHAR userid[UIDSIZ];
};

/* ---------------------------------------------------------------------- *
 * mkfile (internal)                                                      *
 *                                                                        *
 * Create a data file from a key layout, unless it already exists (the    *
 * existence check avoids a harmless but alarming create error at every   *
 * boot).                                                                 *
 * ---------------------------------------------------------------------- */
static VOID
mkfile(const CHAR *fname, size_t reclen, INT nkeys, struct dfaKeySpec *keys)
{
    if (!isfile(fname))
        dfaCreateSpec(fname, FALSE, reclen, PAGESZ, 0, 0, nkeys, keys, NULL);
}

static VOID
seg(struct dfaSegSpec *s, size_t pos, size_t len, INT type)
{
    setmem((CHAR *)s, sizeof(*s), 0);
    s->position = pos;
    s->length   = len;
    s->type     = type;
}

VOID
st_open(VOID)
{
    struct dfaSegSpec s[2];
    struct dfaKeySpec k[2];

    /* SNBBSS.DAT - key 0: the record id                                    */
    seg(&s[0], 0, sizeof(USHORT), DFAST_INTEGER);
    k[0].flags = 0; k[0].nSegments = 1; k[0].segs = &s[0];
    mkfile(SYSFIL, SBSYSLEN, 1, k);
    sysdat = dfaOpen(SYSFIL, SBSYSLEN, NULL);

    /* SNBBSSM.DAT - key 0: module name                                     */
    seg(&s[0], 0, MNMSIZ, DFAST_STRING);
    k[0].flags = 0; k[0].nSegments = 1; k[0].segs = &s[0];
    mkfile(MODFIL, sizeof(struct sbmod), 1, k);
    moddat = dfaOpen(MODFIL, sizeof(struct sbmod), NULL);

    /* SNBBSSU.DAT - key 0: module name + userid (unique);                  */
    /*               key 1: userid (duplicates), for account deletion       */
    seg(&s[0], 0, MNMSIZ + UIDSIZ, DFAST_STRING);
    k[0].flags = 0; k[0].nSegments = 1; k[0].segs = &s[0];
    seg(&s[1], MNMSIZ, UIDSIZ, DFAST_STRING);
    k[1].flags = DFAKF_DUPLICATE; k[1].nSegments = 1; k[1].segs = &s[1];
    mkfile(USEFIL, sizeof(struct sbuse), 2, k);
    usedat = dfaOpen(USEFIL, sizeof(struct sbuse), NULL);
}

VOID
st_close(VOID)
{
    dfaClose(usedat);
    dfaClose(moddat);
    dfaClose(sysdat);
}

/* ---------------------------------------------------------------------- *
 * Singletons in SNBBSS.DAT                                               *
 * ---------------------------------------------------------------------- */
static GBOOL
sys_load(USHORT id, VOID *rec)
{
    USHORT key = id;                 /* local, writable key                  */
    GBOOL found;

    dfaSetBlk(sysdat);
    found = dfaAcqEQ(rec, &key, 0);
    if (found)
        dfaUnlockCur();
    dfaRstBlk();
    return found;
}

static VOID
sys_save(USHORT id, VOID *rec)
{
    CHAR tmp[SBSYSLEN];
    USHORT key = id;
    GBOOL ok;

    dfaSetBlk(sysdat);
    if (dfaAcqEQ(tmp, &key, 0))
        ok = dfaUpdateDup(rec);
    else
        ok = dfaInsertDup(rec);
    dfaRstBlk();
    if (!ok)
        shocst("BBS Stats data error", "%s", "Could not save SNBBSS.DAT record");
}

GBOOL st_cfg_load(struct sbcfg *cfg) { return sys_load(SBREC_CFG, cfg); }
VOID  st_cfg_save(struct sbcfg *cfg) { cfg->id = SBREC_CFG; sys_save(SBREC_CFG, cfg); }
GBOOL st_ctr_load(struct sbctr *ctr) { return sys_load(SBREC_CTR, ctr); }
VOID  st_ctr_save(struct sbctr *ctr) { ctr->id = SBREC_CTR; sys_save(SBREC_CTR, ctr); }

/* ---------------------------------------------------------------------- *
 * Module summaries in SNBBSSM.DAT                                        *
 * ---------------------------------------------------------------------- */
GBOOL
st_mod_get(const CHAR *name, struct sbmod *mod)
{
    CHAR key[MNMSIZ];
    GBOOL found;

    setmem(key, sizeof(key), 0);
    stzcpy(key, name, sizeof(key));
    dfaSetBlk(moddat);
    found = dfaAcqEQ(mod, key, 0);
    if (found)
        dfaUnlockCur();
    dfaRstBlk();
    return found;
}

VOID
st_mod_put(struct sbmod *mod)
{
    struct sbmod tmp;
    CHAR key[MNMSIZ];
    GBOOL ok;

    setmem(key, sizeof(key), 0);
    stzcpy(key, mod->name, sizeof(key));
    dfaSetBlk(moddat);
    if (dfaAcqEQ(&tmp, key, 0))
        ok = dfaUpdateDup(mod);
    else
        ok = dfaInsertDup(mod);
    dfaRstBlk();
    if (!ok)
        shocst("BBS Stats data error", "%s", "Could not save SNBBSSM.DAT record");
}

VOID
st_mod_delete(const CHAR *name)
{
    struct sbmod tmp;
    CHAR key[MNMSIZ];

    setmem(key, sizeof(key), 0);
    stzcpy(key, name, sizeof(key));
    dfaSetBlk(moddat);
    if (dfaAcqEQ(&tmp, key, 0))
        dfaDelete();
    dfaRstBlk();
}

GBOOL
st_mod_walk(struct sbmod *mod, GBOOL first)
{
    GBOOL got;

    /* Steps take no locks and need no key - fine for a read-only pass.     */
    dfaSetBlk(moddat);
    got = first ? dfaStepLO(mod) : dfaStepNX(mod);
    dfaRstBlk();
    return got;
}

/* ---------------------------------------------------------------------- *
 * Per-user module records in SNBBSSU.DAT                                 *
 * ---------------------------------------------------------------------- */
static VOID
mkusekey(struct usekey *k, const CHAR *name, const CHAR *userid)
{
    setmem((CHAR *)k, sizeof(*k), 0);
    stzcpy(k->name, name, MNMSIZ);
    stzcpy(k->userid, userid, UIDSIZ);
}

GBOOL
st_use_get(const CHAR *name, const CHAR *userid, struct sbuse *use)
{
    struct usekey key;
    GBOOL found;

    mkusekey(&key, name, userid);
    dfaSetBlk(usedat);
    found = dfaAcqEQ(use, &key, 0);
    if (found)
        dfaUnlockCur();
    dfaRstBlk();
    return found;
}

VOID
st_use_put(struct sbuse *use)
{
    struct sbuse tmp;
    struct usekey key;
    GBOOL ok;

    mkusekey(&key, use->name, use->userid);
    dfaSetBlk(usedat);
    if (dfaAcqEQ(&tmp, &key, 0))
        ok = dfaUpdateDup(use);
    else
        ok = dfaInsertDup(use);
    dfaRstBlk();
    if (!ok)
        shocst("BBS Stats data error", "%s", "Could not save SNBBSSU.DAT record");
}

VOID
st_use_delete_user(const CHAR *userid)
{
    struct sbuse tmp;
    CHAR key[UIDSIZ];

    setmem(key, sizeof(key), 0);
    stzcpy(key, userid, sizeof(key));
    dfaSetBlk(usedat);
    /* Each acquire finds the next remaining record for this user and      */
    /* establishes the position the delete needs.                          */
    while (dfaAcqEQ(&tmp, key, 1))
        dfaDelete();
    dfaRstBlk();
}

GBOOL
st_use_delete_mod(const CHAR *name, INT max)
{
    struct sbuse tmp;
    struct usekey key;
    GBOOL got = FALSE;
    INT n;

    dfaSetBlk(usedat);
    /* A blank userid sorts first, so each acquire finds the lowest record  */
    /* left for this module and establishes the position the delete needs.  */
    for (n = 0; n <= max; n++) {
        mkusekey(&key, name, "");
        got = dfaAcqGE(&tmp, &key, 0);
        if (got && strncmp(tmp.name, key.name, MNMSIZ) != 0) {
            dfaUnlockCur();          /* the next module's record: leave it   */
            got = FALSE;
        }
        if (!got)
            break;
        if (n == max) {
            dfaUnlockCur();          /* batch done; this one is next time's  */
            break;
        }
        dfaDelete();
    }
    dfaRstBlk();
    return got;
}
