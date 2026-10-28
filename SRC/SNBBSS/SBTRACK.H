/* 26.10.19.2 12:30PM */
/*****************************************************************************
 *   SBTRACK.H                                  BBS Stats - Usage tracking   *
 *                                                                           *
 *   Measures who uses which module and for how long, by looking at every   *
 *   channel once a second - the same way the BBS itself measures module    *
 *   hours (it looks every 15 seconds). See SBTRACK.C for the details.      *
 *                                                                           *
 *   Copyright (C) 2026 Sysop Network.  All Rights Reserved.                 *
 *****************************************************************************/

#ifndef SBTRACK_H
#define SBTRACK_H

#include "gcomm.h"
#include "SBSTORE.H"

VOID trk_init(VOID);                 /* reset channels and start sampling    */
VOID trk_stop(VOID);                 /* save everything pending; stop        */

/* The current stamp of a window (SBW_DAY/WEEK/MONTH), honouring the        */
/* sysop's week-start setting.                                              */
USHORT trk_stamp(INT window);

/* A module summary's counts for a page window (SBP_*): all-time totals, or */
/* the window's counts if they belong to the current window, else zero.    */
VOID trk_counts(const struct sbmod *m, INT pagewin,
                ULONG *users, ULONG *secs, ULONG *visits);

/* Show or hide a module on the module stats page.                          */
VOID trk_sethidden(const CHAR *name, GBOOL hidden);

/* Track a module's use or not. Its figures so far are kept either way.     */
VOID trk_settrack(const CHAR *name, GBOOL track);

/* Delete a module's statistics. Its summary goes at once; its per-user     */
/* records are removed a batch a second, and its use is not counted until  */
/* that is done. Only one module can be in progress at a time.             */
#define SBDEL_GONE  0                /* not installed: removed altogether    */
#define SBDEL_RESET 1                /* installed: back to zero, flags kept  */
#define SBDEL_BUSY  2                /* an earlier delete is still running   */
INT  trk_delete(const CHAR *name);

/* Users logged on right now.                                               */
INT  trk_online(VOID);

#endif /* SBTRACK_H */
