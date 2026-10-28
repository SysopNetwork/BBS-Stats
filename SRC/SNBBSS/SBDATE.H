/* 26.10.11.1 11:56PM */
/*****************************************************************************
 *   SBDATE.H                                 BBS Stats - Calendar windows   *
 *                                                                           *
 *   Turns the BBS's packed dates into "window stamps": one small number     *
 *   that names the current day, week, or month. Every usage counter that    *
 *   belongs to a window carries the stamp it was counted under; when the    *
 *   stamp no longer matches the current one, the window has rolled over     *
 *   and the counter starts again from zero. No nightly job is needed.       *
 *                                                                           *
 *   This file has no BBS dependencies, so it is unit tested on its own      *
 *   (TESTS\DATETEST.C).                                                     *
 *                                                                           *
 *   Copyright (C) 2026 Sysop Network.  All Rights Reserved.                 *
 *****************************************************************************/

#ifndef SBDATE_H
#define SBDATE_H

/* The three rolling windows, used as array indexes throughout the module.  */
#define SBW_DAY     0
#define SBW_WEEK    1
#define SBW_MONTH   2
#define SBW_COUNT   3

/* A packed date is the BBS's today() format: bits 15-9 years since 1980,   */
/* bits 8-5 month (1-12), bits 4-0 day of month (1-31).                     */
unsigned short sbd_pack(int year, int month, int day);

/* Days since 1 January 1980 (which is day 0).                               */
long sbd_daynum(unsigned short packed);

/* Day of the week, 0 = Sunday through 6 = Saturday.                         */
int sbd_weekday(unsigned short packed);

/* The stamp naming the window that the packed date falls in. Stamps are    */
/* never zero, so a zeroed counter always reads as "not this window".       */
/* weekmon: the week starts on Monday instead of Sunday.                    */
unsigned short sbd_stamp(int window, unsigned short packed, int weekmon);

#endif /* SBDATE_H */
