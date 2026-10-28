# 26.10.27.1 09:44PM
# -*- coding: utf-8 -*-
# Packages a BBS Stats release into DIST/<version>/.
#
# Reads the version from SRC/SNBBSS/SBVER.H (SB_VERSION), copies the
# deployable runtime files - and only those - into DIST/<version>/ with
# UPPERCASE names, and builds BBS-Stats-v<version>.zip alongside them:
#
#   SNBBSS.DLL   from BUILD/Release  (build Release first)
#   SNBBSS.MDF   from SRC/SNBBSS
#   SNBBSS.MSG   from SRC/SNBBSS    (regenerate with GENMSG.PY first)
#   SNBBSS.RLN   release notes, hand-written in DIST/<version>/ beforehand
#
# A version that already has a DIST folder is refused: a version number
# names exactly one binary, so never repackage - bump the version instead.
# (Delete the folder by hand first if the release truly never left this
# machine.)

import os
import re
import shutil
import sys
import zipfile

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))

def version():
    hdr = open(os.path.join(ROOT, 'SRC', 'SNBBSS', 'SBVER.H')).read()
    m = re.search(r'#define\s+SB_VERSION\s+"([^"]+)"', hdr)
    if not m:
        sys.exit('SB_VERSION not found in SBVER.H')
    return m.group(1)

def main():
    ver = version()
    dll = os.path.join(ROOT, 'BUILD', 'Release', 'SNBBSS.dll')
    mdf = os.path.join(ROOT, 'SRC', 'SNBBSS', 'SNBBSS.MDF')
    msg = os.path.join(ROOT, 'SRC', 'SNBBSS', 'SNBBSS.MSG')
    for f in (dll, mdf, msg):
        if not os.path.isfile(f):
            sys.exit('missing %s - build Release (and GENMSG.PY) first' % f)

    dest = os.path.join(ROOT, 'DIST', ver)
    zpath = os.path.join(dest, 'BBS-Stats-v%s.zip' % ver)
    if os.path.isfile(zpath):
        sys.exit('DIST/%s is already packaged - a version number names exactly '
                 'one binary. Bump the version instead.' % ver)
    os.makedirs(dest, exist_ok=True)

    shutil.copyfile(dll, os.path.join(dest, 'SNBBSS.DLL'))
    shutil.copyfile(mdf, os.path.join(dest, 'SNBBSS.MDF'))
    shutil.copyfile(msg, os.path.join(dest, 'SNBBSS.MSG'))

    # The MDF's "Online user manual:" line points at SNBBSS.RLN, so every
    # release must ship it or the board shows a dangling manual reference.
    names = ['SNBBSS.DLL', 'SNBBSS.MDF', 'SNBBSS.MSG', 'SNBBSS.RLN']
    if not os.path.isfile(os.path.join(dest, 'SNBBSS.RLN')):
        sys.exit('missing DIST/%s/SNBBSS.RLN - write the release notes first '
                 '(the MDF references them as the online user manual)' % ver)

    with zipfile.ZipFile(zpath, 'w', zipfile.ZIP_DEFLATED) as z:
        for n in names:
            z.write(os.path.join(dest, n), n)

    print('packaged DIST/%s: %s + %s' % (ver, ', '.join(names),
                                         os.path.basename(zpath)))

if __name__ == '__main__':
    main()
