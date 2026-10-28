/* 26.10.15.1 12:00PM */
/*****************************************************************************
 *   SBFMT.C                                      BBS Stats - Number display *
 *                                                                           *
 *   See SBFMT.H.                                                            *
 *                                                                           *
 *   Copyright (C) 2026 Sysop Network.  All Rights Reserved.                 *
 *****************************************************************************/

#include <stdio.h>

#include "SBFMT.H"

/* MSVC's _snprintf does not terminate a truncated string, so every call     */
/* here terminates the buffer itself.                                        */
static void
put(char *buf, size_t len, const char *s)
{
    size_t i;

    if (len == 0)
        return;
    for (i = 0; i + 1 < len && s[i] != '\0'; i++)
        buf[i] = s[i];
    buf[i] = '\0';
}

char *
sbf_num(char *buf, size_t len, unsigned long n)
{
    char digits[16], out[24];
    int nd, i, o = 0;

    nd = sprintf(digits, "%lu", n);
    for (i = 0; i < nd; i++) {
        if (i > 0 && (nd - i) % 3 == 0)
            out[o++] = ',';
        out[o++] = digits[i];
    }
    out[o] = '\0';
    put(buf, len, out);
    return buf;
}

char *
sbf_dur(char *buf, size_t len, unsigned long secs)
{
    char tmp[32];
    unsigned long d = secs / 86400UL;
    unsigned long h = (secs / 3600UL) % 24UL;
    unsigned long m = (secs / 60UL) % 60UL;
    unsigned long s = secs % 60UL;

    if (d > 0)
        sprintf(tmp, "%lud %02luh", d, h);
    else if (h > 0)
        sprintf(tmp, "%luh %02lum", h, m);
    else if (m > 0)
        sprintf(tmp, "%lum %02lus", m, s);
    else
        sprintf(tmp, "%lus", s);
    put(buf, len, tmp);
    return buf;
}

char *
sbf_bytes(char *buf, size_t len, unsigned __int64 n)
{
    static const char *unit[] = { "bytes", "KB", "MB", "GB", "TB", "PB" };
    char tmp[32];
    unsigned __int64 whole = n, rem = 0;
    int u = 0;

    while (whole >= 1024 && u < 5) {
        rem = whole % 1024;
        whole /= 1024;
        u++;
    }
    if (u == 0)
        sprintf(tmp, "%lu bytes", (unsigned long)whole);
    else                             /* one decimal, rounded down            */
        sprintf(tmp, "%lu.%lu %s", (unsigned long)whole,
                (unsigned long)(rem * 10 / 1024), unit[u]);
    put(buf, len, tmp);
    return buf;
}

char *
sbf_pct(char *buf, size_t len, const char *src)
{
    size_t o = 0;

    if (len == 0)
        return buf;
    for (; *src != '\0'; src++) {
        if (*src == '%') {
            if (o + 2 >= len)        /* never split a %% pair               */
                break;
            buf[o++] = '%';
        }
        else if (o + 1 >= len)
            break;
        buf[o++] = *src;
    }
    buf[o] = '\0';
    return buf;
}
