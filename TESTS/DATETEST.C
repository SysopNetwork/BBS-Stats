/* 26.10.11.1 11:56PM */
/*****************************************************************************
 *   DATETEST.C                         BBS Stats - Calendar window tests    *
 *                                                                           *
 *   Console test for SRC\SNBBSS\SBDATE.C. Prints PASS and exits 0 when      *
 *   every check holds. Build and run from the project root:                 *
 *                                                                           *
 *     cl /nologo /J /W3 /ISRC\SNBBSS TESTS\DATETEST.C SRC\SNBBSS\SBDATE.C   *
 *     DATETEST.EXE                                                          *
 *                                                                           *
 *   Copyright (C) 2026 Sysop Network.  All Rights Reserved.                 *
 *****************************************************************************/

#include <stdio.h>

#include "SBDATE.H"

static int fails = 0;

#define CHECK(cond, ...) \
    do { if (!(cond)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static int
leap(int y)
{
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

static int
mdays(int y, int m)
{
    static const int dm[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    return (m == 2 && leap(y)) ? 29 : dm[m - 1];
}

int
main(void)
{
    int y, m, d, wm;
    long prev = -1;
    unsigned short p, prevp = 0;

    /* Anchors computed independently (Python datetime).                    */
    CHECK(sbd_daynum(sbd_pack(1980, 1, 1)) == 0,      "1980-01-01 is day 0");
    CHECK(sbd_daynum(sbd_pack(2000, 2, 29)) == 7364,  "2000-02-29 is day 7364");
    CHECK(sbd_daynum(sbd_pack(2000, 3, 1)) == 7365,   "2000-03-01 is day 7365");
    CHECK(sbd_daynum(sbd_pack(2026, 10, 11)) == 17085, "2026-10-11 is day 17085");
    CHECK(sbd_daynum(sbd_pack(2099, 12, 31)) == 43829, "2099-12-31 is day 43829");
    CHECK(sbd_weekday(sbd_pack(1980, 1, 1)) == 2,     "1980-01-01 was a Tuesday");
    CHECK(sbd_weekday(sbd_pack(2026, 10, 11)) == 0,   "2026-10-11 is a Sunday");
    CHECK(sbd_weekday(sbd_pack(2099, 12, 31)) == 4,   "2099-12-31 is a Thursday");

    /* Walk every day the packed format can hold (1980 through 2107) and    */
    /* check the invariants that the windows rely on.                       */
    for (y = 1980; y <= 2107; y++) {
        for (m = 1; m <= 12; m++) {
            for (d = 1; d <= mdays(y, m); d++) {
                p = sbd_pack(y, m, d);
                CHECK(sbd_daynum(p) == prev + 1,
                      "%04d-%02d-%02d does not follow the day before", y, m, d);
                prev = sbd_daynum(p);

                CHECK(sbd_stamp(SBW_DAY, p, 0) != 0, "day stamp is never zero");
                CHECK(sbd_stamp(SBW_WEEK, p, 0) != 0, "week stamp is never zero");
                CHECK(sbd_stamp(SBW_MONTH, p, 0) != 0, "month stamp is never zero");

                if (prevp != 0) {
                    /* The day window rolls every day.                      */
                    CHECK(sbd_stamp(SBW_DAY, p, 0) != sbd_stamp(SBW_DAY, prevp, 0),
                          "%04d-%02d-%02d day stamp did not roll", y, m, d);
                    /* The month window rolls exactly on the 1st.           */
                    CHECK((sbd_stamp(SBW_MONTH, p, 0) != sbd_stamp(SBW_MONTH, prevp, 0))
                          == (d == 1),
                          "%04d-%02d-%02d month roll is wrong", y, m, d);
                    /* The week window rolls exactly on the first day of    */
                    /* the week: Sunday, or Monday when weekmon is set.     */
                    for (wm = 0; wm <= 1; wm++) {
                        CHECK((sbd_stamp(SBW_WEEK, p, wm) != sbd_stamp(SBW_WEEK, prevp, wm))
                              == (sbd_weekday(p) == (wm ? 1 : 0)),
                              "%04d-%02d-%02d week roll is wrong (weekmon=%d)",
                              y, m, d, wm);
                    }
                }
                prevp = p;
            }
        }
    }

    if (fails) {
        printf("%d check(s) FAILED\n", fails);
        return 1;
    }
    printf("PASS\n");
    return 0;
}
