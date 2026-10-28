# 26.10.27.1 09:44PM
# -*- coding: utf-8 -*-
# Generates SRC/SNBBSS/SNBBSS.MSG (CP437, CRLF) and SRC/SNBBSS/MSGNUM.H.
#
# This script is the source of truth for all module text. Messages embed
# raw ESC color codes and 0x01-delimited named text variables, which do not
# survive hand editing well, so edit here and regenerate.
#
# Message numbers are POSITIONAL in the .MSG: the first LEVELn marker uses
# no slot and every later entry (option, text, or marker) uses one. This
# script writes the matching MSGNUM.H, so numbering cannot drift as long as
# the .MSG and the DLL are deployed together.
#
# ADD NEW MESSAGES ONLY AT THE END of the list below. Appending gives the
# new message the next free slot and leaves every existing number alone.
#
# BBS Stats has no CNF options: every setting is changed online from the
# module's sysop configuration screens. So the file has a single LEVEL6
# (text) section.
#
# Visual identity: a "dashboard" palette. Dark cyan frames, bright cyan
# hotkeys, white headings, yellow values, gray labels. Every line stays
# within 76 visible columns: the host word-wraps near the right margin and
# the exact wrap column varies from board to board.

import os

ESC = '\x1b'
def c(code): return ESC + '[' + code + 'm'
CYN = c('0;36')   # dark cyan - frames and rules
HOT = c('1;36')   # bright cyan - hotkeys
WHT = c('1;37')   # white - headings, prompts
YEL = c('1;33')   # yellow - values
GRY = c('0;37')   # gray - labels and body text
DIM = c('1;30')   # dim - fine print
RED = c('1;31')   # red - errors
GRN = c('1;32')   # green - confirmations
RST = c('0')
NL = '\r\n'

IND = '  '

# MBBS named text variable: the host replaces 0x01 <just> <width+32> NAME
# 0x01 with the value registered for NAME (see SBTVAR.C). just N = natural
# length; L/R/C = pad and justify to the width.
def tv(name, just='N', width=1):
    return '\x01' + just + chr(32 + width) + name + '\x01'

def hdr(label, width=72):
    bar = '═' * max(0, width - len(label) - 6)
    return IND + CYN + '══╡ ' + WHT + label + CYN + ' ╞' + bar + RST + NL

def hdrtv(var, width=72, fixed=24):
    # A header whose title comes from a text variable, padded to a fixed
    # width so the rule after it always lines up.
    bar = '═' * max(0, width - fixed - 6)
    return IND + CYN + '══╡ ' + WHT + tv(var, 'L', fixed) + CYN + ' ╞' + bar + RST + NL

def rule(width=72):
    return IND + CYN + '─' * width + RST + NL

def mrow(key, label, desc=''):
    line = IND + ' ' + CYN + '[' + HOT + key + CYN + ']' + RST + ' ' + WHT + label + RST
    if desc:
        line += ' ' + DIM + '.' * max(2, 22 - len(label)) + RST + ' ' + GRY + desc + RST
    return line + NL

# ---------------------------------------------------------------- messages
# ('marker', header)          - a LEVELn line
# ('text', name, body, label) - a text message (name: 8 characters at most)
# ORDER IS LOAD-BEARING (see the note at the top). Append only.

entries = []
entries.append(('marker', 'LEVEL6 {BBS Stats: module text}'))

def T(name, body, label):
    assert len(name) <= 8, name
    entries.append(('text', name, body, label))

T('MAINMNU',
  NL +
  hdr('BBS STATS') +
  IND + DIM + '    version ' + tv('SBS_VERSION') + RST + NL +
  NL +
  mrow('B', 'BBS statistics', 'how the system is doing') +
  mrow('M', 'Module statistics', 'who uses what, and for how long') +
  mrow('X', 'Exit'),
  'Main menu')

T('SYSOPT',
  mrow('S', 'Sysop configuration', 'what the pages show'),
  'Main menu: sysop line (shown to sysops only)')

T('MAINPRM',
  NL + IND + DIM + 'Created by Jacque Steyn (The Adept BBS) & Mark Laudenbach' + RST + NL +
  NL + IND + WHT + 'Make your selection ' + GRY + '(' + HOT + 'B' + GRY + ', ' + HOT + 'M' + GRY +
  ', ' + HOT + 'X' + GRY + ' to exit, ' + HOT + '?' + GRY + ' for menu)' + WHT + ': ' + RST,
  'Main menu prompt')

T('BADSEL',
  NL + IND + RED + "That's not one of the choices." + RST + NL,
  'Invalid selection')

T('NOACC',
  NL + IND + RED + 'Sorry, that page is restricted on this BBS.' + RST + NL,
  'No access to a page')

T('BBSHDR',
  NL + hdr('BBS STATISTICS') + NL,
  'BBS stats page: header')

T('BBSROW',
  IND + ' ' + GRY + tv('SBS_LABEL', 'L', 26) + ' ' + YEL + tv('SBS_VALUE') + RST + NL,
  'BBS stats page: one item')

T('BBSFTR',
  NL + rule(),
  'BBS stats page: footer')

T('MODHDR',
  NL + hdr('MODULE STATISTICS') +
  IND + ' ' + WHT + tv('SBS_WINDOW') + RST + NL + NL +
  IND + ' ' + HOT + tv('SBS_MHEAD') + RST + NL +
  IND + ' ' + CYN + tv('SBS_MROW') + RST + NL,
  'Module stats page: header (SBS_MROW holds the underline)')

T('MODROW',
  IND + ' ' + GRY + tv('SBS_MROW') + RST + NL,
  'Module stats page: one module')

T('MODNONE',
  IND + ' ' + GRY + 'No module activity in this period yet.' + RST + NL,
  'Module stats page: nothing to show')

T('MODPRM',
  NL + rule() +
  IND + WHT + 'Show ' + CYN + '[' + HOT + 'T' + CYN + ']' + GRY + 'oday ' +
  CYN + '[' + HOT + 'W' + CYN + ']' + GRY + 'eek ' +
  CYN + '[' + HOT + 'M' + CYN + ']' + GRY + 'onth ' +
  CYN + '[' + HOT + 'A' + CYN + ']' + GRY + 'll-time, or ' +
  CYN + '[' + HOT + 'X' + CYN + ']' + GRY + ' to go back' + WHT + ': ' + RST,
  'Module stats page: prompt')

T('CFGMNU',
  NL + hdr('SYSOP CONFIGURATION') + NL +
  mrow('1', 'General settings', 'sysop time, windows, access') +
  mrow('2', 'BBS page items', 'which stats the BBS page shows') +
  mrow('3', 'Module page columns', 'which columns are shown') +
  mrow('4', 'Modules shown', 'which modules are listed') +
  mrow('5', 'Logon advertisement', 'top modules, shown at logon') +
  mrow('X', 'Back') +
  NL + IND + DIM + 'Changes apply immediately and are logged to the audit trail.' + RST + NL,
  'Sysop configuration menu')

T('CFGPRM',
  NL + IND + WHT + 'Configure which ' + GRY + '(' + HOT + '1' + GRY + '-' + HOT + '5' + GRY +
  ', or ' + HOT + 'X' + GRY + ' to go back)' + WHT + ': ' + RST,
  'Sysop configuration prompt')

T('SETHDR',
  NL + hdr('GENERAL SETTINGS') + NL,
  'General settings: header')

T('SETROW',
  IND + ' ' + CYN + '[' + HOT + tv('SBS_NUM', 'R', 2) + CYN + ']' + RST + ' ' +
  GRY + tv('SBS_LABEL', 'L', 34) + ' ' + YEL + tv('SBS_VALUE') + RST + NL,
  'General settings: one setting')

T('SETPRM',
  NL + IND + WHT + 'Change which setting ' + GRY + '(' + HOT + '1' + GRY + '-' + HOT +
  tv('SBS_COUNT') + GRY + ', or ' + HOT + 'X' + GRY + ' to go back)' + WHT + ': ' + RST,
  'General settings: prompt')

T('SETASK',
  NL + IND + CYN + '─── ' + WHT + tv('SBS_LABEL') + CYN + ' ───' + RST + NL +
  IND + GRY + 'Currently ' + YEL + tv('SBS_VALUE') + GRY + '.' + RST + NL +
  IND + GRY + 'Accepts ' + WHT + tv('SBS_RANGE') + GRY + '.' + RST + NL +
  NL + IND + WHT + 'New value ' + GRY + '(' + HOT + 'ENTER' + GRY + ' cancels)' + WHT + ': ' + RST,
  'General settings: ask for a value')

T('SETOK',
  NL + IND + GRN + 'Done. ' + GRY + tv('SBS_LABEL') + ' is now ' + YEL + tv('SBS_VALUE') +
  GRY + '.' + RST + NL,
  'General settings: change confirmed')

T('SETBAD',
  NL + IND + RED + "That won't work here. " + GRY + 'This setting accepts ' + WHT +
  tv('SBS_RANGE') + GRY + '.' + RST + NL +
  IND + WHT + 'New value ' + GRY + '(' + HOT + 'ENTER' + GRY + ' cancels)' + WHT + ': ' + RST,
  'General settings: value not accepted')

T('TOGHDR',
  NL + hdrtv('SBS_TITLE') +
  IND + DIM + 'Entries marked ' + YEL + '*' + DIM + ' are shown. Pick a number to switch it.' + RST + NL + NL,
  'Show/hide list: header')

T('TOGROW',
  IND + ' ' + CYN + '[' + HOT + tv('SBS_NUM', 'R', 3) + CYN + ']' + RST + ' ' +
  YEL + tv('SBS_MARK', 'L', 1) + ' ' + GRY + tv('SBS_LABEL') + RST + NL,
  'Show/hide list: one entry')

T('TOGPRM',
  NL + IND + WHT + 'Switch which ' + GRY + '(' + HOT + '1' + GRY + '-' + HOT + tv('SBS_COUNT') +
  GRY + ', ' + HOT + 'A' + GRY + ' show all, ' + HOT + 'N' + GRY + ' hide all, ' + HOT + 'X' +
  GRY + ' back)' + WHT + ': ' + RST,
  'Show/hide list: prompt')

T('TOGBAD',
  NL + IND + RED + 'No such entry.' + GRY + ' Pick one of the numbers listed above.' + RST + NL,
  'Show/hide list: bad number')

T('EXITMSG',
  NL + IND + GRY + 'Thanks for checking the stats!' + RST + NL,
  'Leaving the module')

# ---- The logon advertisement -------------------------------------------
# The ad's wording and colors live here so the sysop can restyle them in
# the CNF editor; which modules, how many and the layout are chosen online.

T('ADHDR',
  NL + IND + CYN + '══╡ ' + WHT + tv('SBS_TITLE') + CYN + ' ╞════' + RST + NL +
  IND + DIM + 'Top ' + tv('SBS_COUNT') + ' by ' + tv('SBS_RANKBY') + ', ' +
  tv('SBS_PERIOD') + RST + NL + NL,
  'Logon ad: header (SBS_TITLE is the headline set online)')

T('ADTROW',
  IND + ' ' + CYN + '[' + HOT + tv('SBS_NUM', 'R', 2) + CYN + ']' + RST + ' ' +
  WHT + tv('SBS_LABEL', 'L', 30) + ' ' + YEL + tv('SBS_VALUE') + RST + NL,
  'Logon ad: one module in the table layout')

T('ADLROW',
  IND + ' ' + GRY + tv('SBS_MROW') + RST + NL,
  'Logon ad: one line of the two-column or compact layout')

T('ADFTR',
  rule(),
  'Logon ad: footer')

T('ADNONE',
  NL + IND + GRY + 'Nothing to advertise yet: no module has activity in this window.' +
  RST + NL,
  'Logon ad preview: nothing to show')

T('ADSHDR',
  NL + hdr('LOGON ADVERTISEMENT') +
  IND + DIM + 'Shown to callers as they log on. Restyle its text in SNBBSS.MSG.' + RST +
  NL + NL,
  'Logon ad settings: header')

T('ADSPRM',
  NL + IND + WHT + 'Change which setting ' + GRY + '(' + HOT + '1' + GRY + '-' + HOT +
  tv('SBS_COUNT') + GRY + ', ' + HOT + 'P' + GRY + ' to preview, ' + HOT + 'X' + GRY +
  ' to go back)' + WHT + ': ' + RST,
  'Logon ad settings: prompt')

# ---- The global commands -----------------------------------------------
# Typed from anywhere on the BBS; the sysop names them (SBS_BBSCMD and
# SBS_MODCMD, default /BBSSTATS and /MODSTATS). The BBS command shows the
# BBS stats page as it is; the module command shows the module stats page,
# then this footer in place of the page's own prompt (there is no prompt to
# answer outside the module).

T('GMODFTR',
  NL + rule() +
  IND + DIM + 'Type ' + HOT + tv('SBS_MODCMD') + ' T' + DIM + ', ' + HOT + 'W' + DIM + ', ' + HOT +
  'M' + DIM + ' or ' + HOT + 'A' + DIM + ' for today, this week, this month or all-time.' +
  RST + NL,
  'Module stats command: footer after the module stats page')

T('GMODUSE',
  NL + IND + WHT + 'Usage: ' + HOT + tv('SBS_MODCMD') + GRY + ' [' + HOT + 'T' + GRY + '|' + HOT + 'W' +
  GRY + '|' + HOT + 'M' + GRY + '|' + HOT + 'A' + GRY + ']  ' +
  'today, week, month or all-time' + RST + NL,
  'Module stats command: the period was not recognised')

# ---- Stop tracking a module, delete its statistics --------------------

T('MODTPRM',
  NL + IND + DIM + 'T# stops or restarts tracking a module. ' +
  'D# deletes its statistics.' + RST + NL +
  IND + WHT + 'Switch which ' + GRY + '(' + HOT + '1' + GRY + '-' + HOT + tv('SBS_COUNT') +
  GRY + ', ' + HOT + 'A' + GRY + ' all, ' + HOT + 'N' + GRY + ' none, ' + HOT + 'T#' + GRY +
  ', ' + HOT + 'D#' + GRY + ', ' + HOT + 'X' + GRY + ' back)' + WHT + ': ' + RST,
  'Modules shown: prompt (adds T# tracking and D# delete)')

T('DELASK',
  NL + IND + RED + 'Delete all statistics for ' + YEL + tv('SBS_LABEL') + RED + '?' + RST + NL +
  IND + GRY + "Its totals and every user's record for it are removed, and this" + RST + NL +
  IND + GRY + "can't be undone. A module still installed starts again from zero." + RST + NL +
  NL + IND + WHT + 'Delete it ' + GRY + '(' + HOT + 'Y' + GRY + '/' + HOT + 'N' + GRY + ')' +
  WHT + ': ' + RST,
  'Modules shown: confirm deleting a module\'s statistics')

T('DELGONE',
  NL + IND + GRN + 'Deleted. ' + GRY + tv('SBS_LABEL') + ' has been removed from the statistics.' +
  RST + NL,
  'Modules shown: deleted (module not installed)')

T('DELRST',
  NL + IND + GRN + 'Deleted. ' + GRY + tv('SBS_LABEL') + ' starts again from zero.' + RST + NL +
  IND + DIM + 'It is still installed. Use T# to stop tracking it.' + RST + NL,
  'Modules shown: deleted (module still installed)')

T('DELBUSY',
  NL + IND + RED + 'An earlier delete is still finishing.' + GRY + ' Try again in a minute.' +
  RST + NL,
  'Modules shown: a delete is already in progress')

T('DELNO',
  NL + IND + GRY + 'Nothing was deleted.' + RST + NL,
  'Modules shown: delete cancelled')

# ---------------------------------------------------------------- assembly

out = ['']                    # the file begins with a blank line
defines = []                  # (name, slot) for MSGNUM.H
slot = 0
first_marker_seen = False

for e in entries:
    if e[0] == 'marker':
        if first_marker_seen:
            slot += 1         # later LEVELn markers consume a message slot
        first_marker_seen = True
        out.append(e[1])
        out.append('')
    else:
        _, name, body, label = e
        slot += 1
        defines.append((name, slot))
        out.append(name + ' {' + body + '} T ' + label)
        out.append('')

data = (NL.join(out) + NL).encode('cp437')

base = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                     '..', 'SRC', 'SNBBSS'))
msg = os.path.join(base, 'SNBBSS.MSG')
with open(msg, 'wb') as f:
    f.write(data)
print('wrote %s  (%d bytes, %d messages)' % (msg, len(data), slot))

h = ['/* MSGNUM.H - message numbers for SNBBSS.MSG.',
     ' * Generated by TOOLS/GENMSG.PY - do not edit by hand; regenerate instead.',
     ' * The .MSG and the DLL must always be deployed together as a matched set. */',
     '',
     '#ifndef SNBBSS_MSGNUM_H',
     '#define SNBBSS_MSGNUM_H',
     '']
for name, num in defines:
    h.append('#define %-10s %3d' % (name, num))
h += ['', '#endif /* SNBBSS_MSGNUM_H */']

hfile = os.path.join(base, 'MSGNUM.H')
with open(hfile, 'wb') as f:
    f.write((NL.join(h) + NL).encode('ascii'))
print('wrote %s  (%d defines)' % (hfile, len(defines)))
