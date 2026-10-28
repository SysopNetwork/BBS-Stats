/* 26.10.15.1 12:00PM */
/*****************************************************************************
 *   SBFMT.H                                      BBS Stats - Number display *
 *                                                                           *
 *   Formats counts and durations for the stats pages. No BBS dependencies;  *
 *   unit tested by TESTS\FMTTEST.C.                                         *
 *                                                                           *
 *   Copyright (C) 2026 Sysop Network.  All Rights Reserved.                 *
 *****************************************************************************/

#ifndef SBFMT_H
#define SBFMT_H

#include <stddef.h>

/* 1234567 -> "1,234,567". Always NUL-terminates within len.                */
char *sbf_num(char *buf, size_t len, unsigned long n);

/* A duration in seconds, shown at the two most useful units:               */
/*   45 -> "45s", 125 -> "2m 05s", 7380 -> "2h 03m", 190800 -> "2d 05h".    */
char *sbf_dur(char *buf, size_t len, unsigned long secs);

/* A size in bytes at a readable unit, one decimal rounded down:            */
/*   512 -> "512 bytes", 1536 -> "1.5 KB", 3221225472 -> "3.0 GB".          */
char *sbf_bytes(char *buf, size_t len, unsigned __int64 n);

/* Copy text that will become a text variable's value, doubling every '%'.  */
/* The BBS expands text variables into a message BEFORE handing it to      */
/* vsprintf (gcommlib PRF.C, prfmsg -> getMsgBlk), so a bare '%' in a      */
/* value - a module named "100% Fun" - would be read as a format code.     */
/* A pair is never split when the copy is cut short.                        */
char *sbf_pct(char *buf, size_t len, const char *src);

#endif /* SBFMT_H */
