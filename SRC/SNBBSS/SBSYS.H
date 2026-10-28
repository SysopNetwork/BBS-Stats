/* 26.10.15.1 12:00PM */
/*****************************************************************************
 *   SBSYS.H                                    BBS Stats - Server health    *
 *                                                                           *
 *   CPU load, memory and disk space of the machine the BBS runs on, for    *
 *   the BBS stats page. Kept apart from the BBS headers (it talks only to  *
 *   Windows), which also lets it be tested outside the BBS.                *
 *                                                                           *
 *   Every call here returns at once: nothing waits or polls. CPU load is   *
 *   worked out from samples the module's once-a-second timer takes, so     *
 *   showing it never means measuring over a delay.                         *
 *                                                                           *
 *   Copyright (C) 2026 Sysop Network.  All Rights Reserved.                 *
 *****************************************************************************/

#ifndef SBSYS_H
#define SBSYS_H

/* Take one CPU sample. Call about once a second.                           */
void sys_sample(void);

/* CPU load over the last few samples, as whole percentages: the whole      */
/* machine, and the share of it used by the BBS server itself. Returns 0   */
/* until two samples exist.                                                 */
int sys_cpu(int *machine, int *server);

/* Physical memory: bytes free, bytes installed, percent in use.            */
/* Returns 0 if Windows could not say.                                      */
int sys_mem(unsigned __int64 *avail, unsigned __int64 *total, int *usedpct);

/* Disk holding the BBS's own directory: bytes free, bytes in all.          */
/* Returns 0 if Windows could not say.                                      */
int sys_disk(unsigned __int64 *avail, unsigned __int64 *total);

#endif /* SBSYS_H */
