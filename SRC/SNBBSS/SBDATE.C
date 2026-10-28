/* 26.10.11.1 11:56PM */
/*****************************************************************************
 *   SBDATE.C                                 BBS Stats - Calendar windows   *
 *                                                                           *
 *   See SBDATE.H. Plain C with no BBS headers, so the unit tests can build  *
 *   it on its own.                                                          *
 *                                                                           *
 *   Copyright (C) 2026 Sysop Network.  All Rights Reserved.                 *
 *****************************************************************************/

#include "SBDATE.H"

unsigned short
sbd_pack(int year, int month, int day)
{
    return (unsigned short)(((year - 1980) << 9) | (month << 5) | day);
}

/* ---------------------------------------------------------------------- *
 * sbd_daynum                                                             *
 *                                                                        *
 * Counts days from 1 January 1980 using the standard "days from civil"   *
 * method: treat March as the first month of the year so the leap day    *
 * falls at the very end, then count whole 400-year eras.                 *
 * ---------------------------------------------------------------------- */
long
sbd_daynum(unsigned short packed)
{
    long y   = 1980L + ((packed >> 9) & 0x7F);
    long m   = (packed >> 5) & 0x0F;
    long d   = packed & 0x1F;
    long era, yoe, doy, doe;

    if (m < 1)  m = 1;               /* guard against a corrupt date         */
    if (m > 12) m = 12;
    if (d < 1)  d = 1;

    if (m <= 2)
        y--;
    era = y / 400;                   /* years here are always positive       */
    yoe = y - era * 400;
    doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;

    /* era*146097+doe counts from 1 March of year 0; 723120 moves day 0 to  */
    /* 1 January 1980.                                                      */
    return era * 146097L + doe - 723120L;
}

int
sbd_weekday(unsigned short packed)
{
    /* 1 January 1980 was a Tuesday (2).                                    */
    return (int)((sbd_daynum(packed) + 2) % 7);
}

unsigned short
sbd_stamp(int window, unsigned short packed, int weekmon)
{
    long dn = sbd_daynum(packed);

    switch (window) {
    case SBW_DAY:
        return (unsigned short)(dn + 1);
    case SBW_WEEK:
        /* Shift so each week boundary lands on a multiple of seven: day   */
        /* -2 was a Sunday and day -1 a Monday.                            */
        return (unsigned short)((dn + (weekmon ? 1 : 2)) / 7 + 1);
    case SBW_MONTH:
        return (unsigned short)(((packed >> 9) & 0x7F) * 12
                                + ((packed >> 5) & 0x0F));
    }
    return 0;
}
