/* 26.10.15.1 12:00PM */
/*****************************************************************************
 *   SYSTEST.C                                BBS Stats - Server health test *
 *                                                                           *
 *   Console test for SRC\SNBBSS\SBSYS.C against the machine it runs on.     *
 *   It cannot know the right figures, so it checks they are sane: CPU load *
 *   is reported only after two samples and stays within 0-100, a second of *
 *   busy work shows up as load, and memory and disk report plausible,      *
 *   self-consistent sizes. Prints PASS and exits 0 when every check holds. *
 *                                                                           *
 *   Copyright (C) 2026 Sysop Network.  All Rights Reserved.                 *
 *****************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#include "SBSYS.H"

static int fails = 0;

static void
check(int ok, const char *what)
{
    if (!ok) {
        fails++;
        printf("FAIL: %s\n", what);
    }
}

int
main(void)
{
    int m = -1, s = -1, pct = -1, i;
    unsigned __int64 avail = 0, total = 0;
    volatile unsigned long spin = 0;
    DWORD until;

    check(!sys_cpu(&m, &s), "CPU load reported before any sample");
    sys_sample();
    check(!sys_cpu(&m, &s), "CPU load reported after only one sample");

    until = GetTickCount() + 1100;   /* a second of work on this thread      */
    while (GetTickCount() < until)
        spin++;
    sys_sample();
    check(sys_cpu(&m, &s), "no CPU load after two samples");
    check(m >= 0 && m <= 100 && s >= 0 && s <= 100, "CPU load outside 0-100");
    check(s > 0, "a second of busy work did not register for this process");
    check(s <= m + 1, "this process used more than the whole machine");

    for (i = 0; i < 8; i++)          /* wrap the ring more than once         */
        sys_sample();
    check(sys_cpu(&m, &s) && m >= 0 && m <= 100, "CPU load after the ring wrapped");

    check(sys_mem(&avail, &total, &pct), "memory not reported");
    check(total >= ((unsigned __int64)256 << 20), "less than 256 MB installed");
    check(avail <= total, "more memory free than installed");
    check(pct >= 0 && pct <= 100, "memory in use outside 0-100");

    check(sys_disk(&avail, &total), "disk not reported");
    check(total > 0 && avail <= total, "disk sizes inconsistent");

    if (fails) {
        printf("%d check(s) FAILED\n", fails);
        return 1;
    }
    printf("PASS\n");
    return 0;
}
