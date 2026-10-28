/* 26.10.15.1 12:00PM */
/*****************************************************************************
 *   FMTTEST.C                              BBS Stats - Number display tests *
 *                                                                           *
 *   Console test for SRC\SNBBSS\SBFMT.C. Prints PASS and exits 0 when       *
 *   every check holds. Build and run from the project root:                 *
 *                                                                           *
 *     cl /nologo /J /W3 /ISRC\SNBBSS TESTS\FMTTEST.C SRC\SNBBSS\SBFMT.C     *
 *     FMTTEST.EXE                                                           *
 *                                                                           *
 *   Copyright (C) 2026 Sysop Network.  All Rights Reserved.                 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>

#include "SBFMT.H"

static int fails = 0;

static void
expect(const char *got, const char *want)
{
    if (strcmp(got, want) != 0) {
        fails++;
        printf("FAIL: got \"%s\", want \"%s\"\n", got, want);
    }
}

int
main(void)
{
    char b[32];

    expect(sbf_num(b, sizeof(b), 0UL), "0");
    expect(sbf_num(b, sizeof(b), 999UL), "999");
    expect(sbf_num(b, sizeof(b), 1000UL), "1,000");
    expect(sbf_num(b, sizeof(b), 1234567UL), "1,234,567");
    expect(sbf_num(b, sizeof(b), 4294967295UL), "4,294,967,295");
    expect(sbf_num(b, 4, 1234567UL), "1,2");     /* truncates, terminated  */

    expect(sbf_dur(b, sizeof(b), 0UL), "0s");
    expect(sbf_dur(b, sizeof(b), 45UL), "45s");
    expect(sbf_dur(b, sizeof(b), 60UL), "1m 00s");
    expect(sbf_dur(b, sizeof(b), 125UL), "2m 05s");
    expect(sbf_dur(b, sizeof(b), 3600UL), "1h 00m");
    expect(sbf_dur(b, sizeof(b), 7380UL), "2h 03m");
    expect(sbf_dur(b, sizeof(b), 86400UL), "1d 00h");
    expect(sbf_dur(b, sizeof(b), 190800UL), "2d 05h");
    expect(sbf_dur(b, sizeof(b), 4294967295UL), "49710d 06h");

    expect(sbf_bytes(b, sizeof(b), 0), "0 bytes");
    expect(sbf_bytes(b, sizeof(b), 1023), "1023 bytes");
    expect(sbf_bytes(b, sizeof(b), 1024), "1.0 KB");
    expect(sbf_bytes(b, sizeof(b), 1536), "1.5 KB");
    expect(sbf_bytes(b, sizeof(b), 1048575), "1023.9 KB");
    expect(sbf_bytes(b, sizeof(b), 3221225472ULL), "3.0 GB");
    expect(sbf_bytes(b, sizeof(b), 2199023255552ULL), "2.0 TB");

    expect(sbf_pct(b, sizeof(b), "Plain"), "Plain");
    expect(sbf_pct(b, sizeof(b), "100% Fun"), "100%% Fun");
    expect(sbf_pct(b, sizeof(b), "%s%d"), "%%s%%d");
    expect(sbf_pct(b, 4, "ab%c"), "ab");          /* pair never split       */
    expect(sbf_pct(b, 5, "ab%c"), "ab%%");
    expect(sbf_pct(b, 1, "abc"), "");

    if (fails) {
        printf("%d check(s) FAILED\n", fails);
        return 1;
    }
    printf("PASS\n");
    return 0;
}
