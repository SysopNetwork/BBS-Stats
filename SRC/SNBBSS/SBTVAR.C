/* 26.10.20.1 12:15AM */
/*****************************************************************************
 *   SBTVAR.C                                   BBS Stats - Text variables   *
 *                                                                           *
 *   A text variable sits in message text as the bytes                      *
 *   0x01 <justify> <width+32> NAME 0x01. When the message is shown, the    *
 *   BBS swaps that for the string the variable's routine returns. The      *
 *   routines here hand back small buffers the module fills (tvar_set)      *
 *   right before each message.                                              *
 *                                                                           *
 *   Copyright (C) 2026 Sysop Network.  All Rights Reserved.                 *
 *****************************************************************************/

#include <stdio.h>

#include "gcomm.h"
#include "majorbbs.h"

#include "SBVER.H"
#include "SBFMT.H"
#include "SBTVAR.H"
#include "SBCFG.H"

#define TVBW 96                      /* fits the widest module page row      */

static CHAR tvb[TV_SLOTS][TVBW];

/* The BBS wants a distinct routine for each registered name.               */
static CHAR *tvLABEL(VOID)  { return tvb[TV_LABEL];  }
static CHAR *tvVALUE(VOID)  { return tvb[TV_VALUE];  }
static CHAR *tvWINDOW(VOID) { return tvb[TV_WINDOW]; }
static CHAR *tvMHEAD(VOID)  { return tvb[TV_MHEAD];  }
static CHAR *tvMROW(VOID)   { return tvb[TV_MROW];   }
static CHAR *tvNUM(VOID)    { return tvb[TV_NUM];    }
static CHAR *tvRANGE(VOID)  { return tvb[TV_RANGE];  }
static CHAR *tvMARK(VOID)   { return tvb[TV_MARK];   }
static CHAR *tvTITLE(VOID)  { return tvb[TV_TITLE];  }
static CHAR *tvCOUNT(VOID)  { return tvb[TV_COUNT];  }
static CHAR *tvRANKBY(VOID) { return tvb[TV_RANKBY]; }
static CHAR *tvPERIOD(VOID) { return tvb[TV_PERIOD]; }

/* The running build's own version, for any screen that shows it. A        */
/* version typed into the message text would go stale with the first        */
/* DLL-only update; this one cannot.                                        */
static CHAR tvver[] = SB_VERSION;  /* writable, like every other value     */
static CHAR *tvVER(VOID)    { return tvver;          }

/* The global command names, as the sysop set them ("" when switched off). */
/* They hold letters, digits and the slash only, so no % to escape.         */
static CHAR *tvBBSCMD(VOID) { return (CHAR *)cfg_bbscmd(); }
static CHAR *tvMODCMD(VOID) { return (CHAR *)cfg_modcmd(); }

VOID
tvar_init(VOID)
{
    register_textvar("SBS_LABEL",   tvLABEL);
    register_textvar("SBS_VALUE",   tvVALUE);
    register_textvar("SBS_WINDOW",  tvWINDOW);
    register_textvar("SBS_MHEAD",   tvMHEAD);
    register_textvar("SBS_MROW",    tvMROW);
    register_textvar("SBS_NUM",     tvNUM);
    register_textvar("SBS_RANGE",   tvRANGE);
    register_textvar("SBS_MARK",    tvMARK);
    register_textvar("SBS_TITLE",   tvTITLE);
    register_textvar("SBS_COUNT",   tvCOUNT);
    register_textvar("SBS_VERSION", tvVER);
    register_textvar("SBS_RANKBY",  tvRANKBY);
    register_textvar("SBS_PERIOD",  tvPERIOD);
    register_textvar("SBS_BBSCMD",  tvBBSCMD);
    register_textvar("SBS_MODCMD",  tvMODCMD);
}

/* The BBS expands text variables before the message goes through        */
/* vsprintf (see SBFMT.H), so every '%' in a value is doubled here.        */
VOID
tvar_set(INT slot, const CHAR *value)
{
    if (slot >= 0 && slot < TV_SLOTS)
        sbf_pct(tvb[slot], TVBW, value);
}

VOID
tvar_setn(INT slot, LONG value)
{
    if (slot >= 0 && slot < TV_SLOTS)
        sprintf(tvb[slot], "%ld", value);
}
