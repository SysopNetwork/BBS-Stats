/* 26.10.15.1 12:00PM */
/*****************************************************************************
 *   SBSYS.C                                    BBS Stats - Server health    *
 *                                                                           *
 *   See SBSYS.H.                                                            *
 *                                                                           *
 *   CPU load: Windows reports, for the whole machine, how long all its     *
 *   processors have spent idle and busy since boot (GetSystemTimes), and   *
 *   for this process how long it has spent running (GetProcessTimes).      *
 *   Comparing two readings a few seconds apart gives the load over that    *
 *   span. The timer keeps the last SPAN+1 readings in a ring.              *
 *                                                                           *
 *   Copyright (C) 2026 Sysop Network.  All Rights Reserved.                 *
 *****************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "SBSYS.H"

#define SPAN 5                       /* seconds of history behind the figure */

struct cpusamp {
    unsigned __int64 idle;           /* machine: all processors idle         */
    unsigned __int64 busy;           /* machine: kernel + user (idle incl.)  */
    unsigned __int64 proc;           /* this process: kernel + user          */
};

static struct cpusamp ring[SPAN + 1];
static int nsamp;                    /* samples taken (stops counting at max)*/
static int head;                     /* where the next sample goes           */

static unsigned __int64
ft(const FILETIME *f)
{
    return ((unsigned __int64)f->dwHighDateTime << 32) | f->dwLowDateTime;
}

void
sys_sample(void)
{
    FILETIME idle, kern, user, pcre, pexit, pkern, puser;
    struct cpusamp *s = &ring[head];

    if (!GetSystemTimes(&idle, &kern, &user))
        return;
    if (!GetProcessTimes(GetCurrentProcess(), &pcre, &pexit, &pkern, &puser))
        return;
    s->idle = ft(&idle);
    s->busy = ft(&kern) + ft(&user); /* kernel time includes idle time       */
    s->proc = ft(&pkern) + ft(&puser);
    head = (head + 1) % (SPAN + 1);
    if (nsamp < SPAN + 1)
        nsamp++;
}

/* Whole percentage of part in whole, clamped to 0-100.                     */
static int
pct(unsigned __int64 part, unsigned __int64 whole)
{
    unsigned __int64 p;

    if (whole == 0)
        return 0;
    p = (part * 100 + whole / 2) / whole;
    return p > 100 ? 100 : (int)p;
}

int
sys_cpu(int *machine, int *server)
{
    const struct cpusamp *old, *now;
    unsigned __int64 dtot, didle, dproc;

    if (nsamp < 2)
        return 0;
    now = &ring[(head + SPAN) % (SPAN + 1)];          /* newest             */
    old = &ring[nsamp <= SPAN ? 0 : head];            /* oldest held        */
    dtot  = now->busy - old->busy;
    didle = now->idle - old->idle;
    dproc = now->proc - old->proc;
    if (didle > dtot)
        didle = dtot;
    *machine = pct(dtot - didle, dtot);
    *server = pct(dproc, dtot);
    return 1;
}

int
sys_mem(unsigned __int64 *avail, unsigned __int64 *total, int *usedpct)
{
    MEMORYSTATUSEX ms;

    ms.dwLength = sizeof(ms);
    if (!GlobalMemoryStatusEx(&ms))
        return 0;
    *avail = ms.ullAvailPhys;
    *total = ms.ullTotalPhys;
    *usedpct = (int)ms.dwMemoryLoad;
    return 1;
}

int
sys_disk(unsigned __int64 *avail, unsigned __int64 *total)
{
    ULARGE_INTEGER freecaller, all, freeall;

    /* NULL = the drive of the current directory, which is the BBS's own   */
    /* directory while the server runs.                                     */
    if (!GetDiskFreeSpaceExA(NULL, &freecaller, &all, &freeall))
        return 0;
    *avail = freecaller.QuadPart;
    *total = all.QuadPart;
    return 1;
}
