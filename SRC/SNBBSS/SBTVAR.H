/* 26.10.20.1 12:15AM */
/*****************************************************************************
 *   SBTVAR.H                                   BBS Stats - Text variables   *
 *                                                                           *
 *   Every changing value on the module's screens reaches the message text  *
 *   through a NAMED text variable, never a positional argument. The sysop  *
 *   may therefore reword, reorder, repeat or drop any value just by        *
 *   editing SNBBSS.MSG. Names are global to the whole BBS, so all of them  *
 *   start with SBS_ and stay within the BBS's 15-character limit. Once a   *
 *   name has shipped it is part of the sysop's interface: never rename it.  *
 *                                                                           *
 *   Copyright (C) 2026 Sysop Network.  All Rights Reserved.                 *
 *****************************************************************************/

#ifndef SBTVAR_H
#define SBTVAR_H

#include "gcomm.h"

/* Value slots, filled just before the message that shows them.             */
#define TV_LABEL    0                /* SBS_LABEL   a stat or setting name   */
#define TV_VALUE    1                /* SBS_VALUE   its value                */
#define TV_WINDOW   2                /* SBS_WINDOW  "Showing today"...       */
#define TV_MHEAD    3                /* SBS_MHEAD   module page column titles*/
#define TV_MROW     4                /* SBS_MROW    one module page row      */
#define TV_NUM      5                /* SBS_NUM     a row number             */
#define TV_RANGE    6                /* SBS_RANGE   what a setting accepts   */
#define TV_MARK     7                /* SBS_MARK    shown / hidden marker    */
#define TV_TITLE    8                /* SBS_TITLE   a screen's title         */
#define TV_COUNT    9                /* SBS_COUNT   how many rows            */
#define TV_RANKBY   10               /* SBS_RANKBY  what the ad ranks by     */
#define TV_PERIOD   11               /* SBS_PERIOD  the ad's time window     */
#define TV_SLOTS    12

/* SBS_VERSION, SBS_BBSCMD and SBS_MODCMD need no slot: they read the       */
/* build and the live settings directly, so they are right anywhere.        */

VOID tvar_init(VOID);                /* register every SBS_ name             */
VOID tvar_set(INT slot, const CHAR *value);   /* escapes any % itself  */
VOID tvar_setn(INT slot, LONG value);

#endif /* SBTVAR_H */
