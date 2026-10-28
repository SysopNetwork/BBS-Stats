/* 26.10.27.1 09:44PM */
/*****************************************************************************
 *   SBVER.H                                     BBS Stats - Version number  *
 *                                                                           *
 *   The single place the module's version lives. Both the C code (audit     *
 *   trail entry, on-screen version text variable) and the Windows version  *
 *   resource (SNBBSS.rc) include this file, so they can never disagree.     *
 *   It must stay plain #defines only - the resource compiler reads it.      *
 *                                                                           *
 *   A version number names exactly one binary: never rebuild under a        *
 *   version that has already been shared - bump it instead.                 *
 *                                                                           *
 *   Copyright (C) 2026 Sysop Network.  All Rights Reserved.                 *
 *****************************************************************************/

#ifndef SBVER_H
#define SBVER_H

#define SB_VER_MAJOR    1
#define SB_VER_MINOR    0
#define SB_VER_PATCH    0

/* Packaged-build counter: the DLL's fourth version number. It goes up by   */
/* one for EVERY packaged build and never resets, so a pre-release and the  */
/* final release of the same version can never share a Windows file        */
/* version.                                                                 */
#define SB_VER_BUILD    9

/* 1 while SB_VERSION is a pre-release (-beta.N etc.); 0 for a release.   */
#define SB_PRERELEASE   0

/* The full SemVer version, shown everywhere a sysop reads it.              */
#define SB_VERSION      "1.0.0"         /* keep in step with the numbers above */

#endif /* SBVER_H */
