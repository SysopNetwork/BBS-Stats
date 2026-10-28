/* 26.10.21.2 10:47AM */
/*****************************************************************************
 *   SBBBS.H                                  BBS Stats - The BBS stats page *
 *                                                                           *
 *   The BBS-wide counters this module keeps itself (logons, new users and  *
 *   the most users online at once, per day / week / month), and the list   *
 *   of items the BBS stats page can show. The sysop hides or shows each    *
 *   item from the sysop configuration screens.                             *
 *                                                                           *
 *   Copyright (C) 2026 Sysop Network.  All Rights Reserved.                 *
 *****************************************************************************/

#ifndef SBBBS_H
#define SBBBS_H

#include "gcomm.h"

VOID bbs_open(VOID);                 /* load the counters (or start them)    */
VOID bbs_logon(VOID);                /* a caller finished logging on         */
VOID bbs_newacct(VOID);              /* a new account was created            */
VOID bbs_online(INT n);              /* n users online right now (each tick) */
VOID bbs_scan(VOID);                 /* count ANSI users a batch at a time   */

/* The page items, numbered from 0. An item's number is stored in the       */
/* sysop's hide/show settings, so numbers never change: new items are only  */
/* ever added at the end.                                                   */
INT         bbs_count(VOID);
const CHAR *bbs_label(INT item);
CHAR       *bbs_value(INT item, CHAR *buf, INT len);

#endif /* SBBBS_H */
