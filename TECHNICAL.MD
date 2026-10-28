# BBS Stats: Technical Notes

## Architecture

| File | Role |
|---|---|
| `SNBBSS.C` | Entry point (`init__snbbss`), module block, menus, both stats pages, sysop editor |
| `SBTRACK.C` | Once-a-second sampler that measures module use; module summary cache |
| `SBBBS.C` | BBS-wide counters (logons, new users, peaks) and the BBS page item table |
| `SBCFG.C` | Live settings: defaults, getters, editor rows |
| `SBSTORE.C` | All Btrieve access; record layouts in `SBSTORE.H` |
| `SBTVAR.C` | Named text variables (`SBS_*`) |
| `SBDATE.C` | Packed date → day/week/month window stamps (no BBS dependencies) |
| `SBFMT.C` | Number and duration formatting (no BBS dependencies) |
| `SBVER.H` | The version, shared by the code and the resource script |

## Measuring module use

`usrptr->state` is the index into the engine's `module[]` array of the
module a channel is in (index 0 is the menu system). The engine samples it
the same way for its own module-hour statistics: `dec15s()` in
`ACCOUNT.C:875` adds 15 seconds to `mdstats[state]` every 15 seconds.

`SBTRACK.C` runs a one-second `rtkick()` timer. Each tick:

1. Credit the seconds since the last tick to the module each channel was
   seen in last time (if it is being timed).
2. If the channel is no longer `ACTUSR`, has a different user-id, or is in
   a different module, the visit is over: **flush** it.
3. A channel that is `ACTUSR` and not being timed starts a new visit.
4. A visit that has piled up 60 seconds is flushed early, so pages stay
   current and a crash loses at most a minute.

Only `usrcls == ACTUSR` counts. During logon the engine sets
`usrptr->state` to each module in turn while it runs their logon routines
(`cyclon()`, `MAJORBBS.C:4698`) with the user in `SUPLON`, and that is not
real use.

Sysop visits are skipped unless the sysop enables counting them. The test
is `gen_haskey(syskey, ch, usroff(ch))`, which reads that channel's own key
array (`LOCKNKEY.C:216-246`) and so works with no current user. The
setting is checked when a visit starts.

The design needs no logon, logoff, hangup or connect hooks. Each tick
compares the channel against what was seen the tick before, so a recycled
channel can never carry one caller's time into another caller's record.
The cost is up to one second of time per visit.

`trk_stop()` (called from the module's shutdown routine, which also runs
before the nightly restart) flushes every visit in progress.

## Windows and distinct users

Each window counter carries a **stamp** (`sbd_stamp`): day number + 1, week
number + 1, or months since 1980 + 1. Stamps are never 0, so a zeroed
counter is never mistaken for the current window. When a counter's stamp no
longer matches the current one, the window has ended and the counter reads
as zero. It starts over on the next write. There is no nightly reset job.

Distinct users come from the per-user records (`SNBBSSU.DAT`, one per
module and user-id). On a flush:

- a new user record adds 1 to the module's all-time users;
- for each window, if the user record's stamp is not current, it is reset
  and the module's users for that window go up by 1.

Changing the week-start setting changes every week stamp, so the current
week's figures start over from that moment.

## Data files

All three are created with `dfaCreateSpec` on first run (page size 1024,
matching the MDF). Record structs use `#pragma pack(1)` and have
compile-time size checks. New fields must come out of the `spare` bytes,
where zero means the old behaviour.

| File | Key 0 | Key 1 | Record |
|---|---|---|---|
| `SNBBSS.DAT` | `USHORT id` (1 = settings, 2 = counters) | none | 256 bytes (`sbcfg` / `sbctr`) |
| `SNBBSSM.DAT` | module name (25, string) | none | 128 bytes (`sbmod`) |
| `SNBBSSU.DAT` | module name + user-id (55, string, unique) | user-id (30, duplicates) | 113 bytes (`sbuse`) |

The settings record carries a layout version (currently 4). New fields
are only ever carved from the spare bytes, where zero means the old
behaviour, so a record of any earlier layout loads as is. The logon
advertisement's fields (`adon` ... `adtitle`) read zero as "every choice
at its default". A record older than layout 3 has the ad switched on once,
and one older than layout 4 gets the default global command names once;
after that an empty name means the sysop switched the command off.

`sbmod.notrack` came from the module summary's spare bytes; zero, as in
every older record, means "tracked".

Module records are keyed by the module's name (`module[i]->descrp`, from
its MDF). Renaming a module in its MDF starts new statistics under the new
name. A module summary record is created for every installed module at the
first tick, so the sysop can choose which modules to show before they are
used. The menu system (module 0) is hidden by default.

Account deletion removes that user's `SNBBSSU.DAT` records (key 1). Totals
already counted stay as they were.

### Untracked and deleted modules

`flush()` throws away the time of a module with `notrack` set, so its
figures stand still. Untracked modules are left off the module page and
the ad but stay on the sysop's *Modules shown* list.

`trk_delete()` (the **D** command) deals with the summary at once: a module
that is no longer installed loses its record (`st_mod_delete`); an
installed one gets a zeroed summary that keeps `hidden` and `notrack`,
written through the in-memory copy so a later flush can't write the old
totals back. Its per-user records are removed `DELBATCH` (200) per tick by
`st_use_delete_mod`, which acquires the lowest key at or after
(name, blank user-id) on key 0 and deletes it, so a module with thousands
of users never stalls the main thread. Until the last one is gone,
`flush()` drops that module's time: a user record not yet reached would
make a returning user look like one already counted. One delete runs at a
time (`SBDEL_BUSY`). `trk_stop()` finishes one still running at shutdown;
after a crash mid-delete the leftover user records stay, and a returning
user is then not counted again as a new distinct user.

## BBS page sources

From the engine's live globals (`MAJORBBS.H:347-406`): `sv2.totcalls`,
`sv2.numact`, `sv2.numans`, `sv.msgtot`, `sv.uplds`, `sv.dwnlds`, and
`sv.calls[]` (calls this month, kept per baud rate and summed). Also
`bbsttl`, `nterms` and `nmods`. Logons, new users (`hook_createacct`) and
peaks are this module's own counters in `SNBBSS.DAT`. "Time online" sums
every module summary, including the menu system.

Server health (`SBSYS.C`, which includes only `<windows.h>`): CPU load
comes from `GetSystemTimes` and `GetProcessTimes` readings that the
1-second tick stores in a ring, and spans the last 5 seconds, so showing it
never waits. Memory is `GlobalMemoryStatusEx`; disk is
`GetDiskFreeSpaceExA` on the current directory, which is the BBS's own
directory while the server runs.

## Logon advertisement

`sb_logon` (the module's logon routine, run for every caller by the
engine's `cyclon`, `MAJORBBS.C:4689`) shows the ad when it is switched on,
the caller is not a client/server session (`WSGCSU`, as `AAEML.C` emllon
does) and `sees_ad()` passes: the caller lacks the hiding key (`adnokey`,
if set) and has the key needed to see it (`adkey`, if set; sysops always
pass this one). `low_haskey` (`LOCKNKEY.C:256`) returns 1 for a MASTER
user before it looks the lock up, so sysops with MASTER skip the ad
whenever a hiding key is set. The preview ignores both keys. `show_ad()` ranks the
module summaries for the chosen window, skipping hidden modules and those
with nothing to show, and prints `ADHDR`, then `ADTROW` per module (table)
or `ADLROW` per laid-out line (two columns, compact), then `ADFTR`. With
nothing to show, a logon prints nothing; the sysop's preview prints
`ADNONE`.

## Global commands

`sb_global` is registered with `globalcmd()` and is offered every line a
user types, before the module they are in sees it (`MAJORBBS.C:3326`,
`hdlcri`). It claims only the BBS stats command with no arguments and the
module stats command with at most one, and only for `ACTUSR` callers;
everything else returns 0 so other handlers get it. The same page keys
apply as in the module.

The names are settings (`bbscmd`, `modcmd`; default `/BBSSTATS` and
`/MODSTATS`), read on every line, so a change applies at once. "" switches
a command off. Handlers run in the order they were registered, and the
engine's own (`glorys`, `MAJORBBS.C:1254`) comes before any module's, so
`setcmd` refuses the engine's names (`/GO`, `/P`, `/RECENT`, `/L`,
`/INVIS`). A clash with another module's command can't be detected: the
module that loads first takes the line. `/STATS` is not a default because
ML has one. The `SBS_BBSCMD` and `SBS_MODCMD` text variables read the live
names, so `GMODFTR` and `GMODUSE` always name the command as set.

Returning 1 makes the engine call `injacr()` (`MAJORBBS.C:3194`), which
feeds an empty line to the user's current module with `INJOIP` set, so the
module re-prompts. Inside BBS Stats an empty line means "go back", so
`sb_input` checks `INJOIP` and re-sends the current screen's prompt instead
(`reprompt`), as the engine's `FILEXFER.C:1353` does. This also covers other
modules' global commands (`/P`, `/RECENT`) typed inside BBS Stats.

## Output

`prfmsg()` expands text variables into the message *before* `vsprintf`
(gcommlib `PRF.C:116` via `getMsgBlk`, `MAJORBBS.C:5235`), so `tvar_set()`
doubles every `%` in a value (`sbf_pct`). `prfbuf` is only `OUTBSZ` bytes
(8K minimum, `MAJORBBS.C:821`) and `prfmsg` does not bound its writes, so
long lists call `flushlong()`, which sends the buffer once it passes 4K.

## Settings

The settings record is loaded at init and written through on every change.
All readers use the in-memory copy, which is always identical to the stored
one. There are no CNF options. The defaults in `cfg_defaults()` seed a new
install. BBS page items are numbered by their position in `SBBBS.C`, and
that number is stored in the `itemhide` bitmask, so items are only ever
added at the end.

## Text variables

`SBS_LABEL`, `SBS_VALUE`, `SBS_WINDOW`, `SBS_MHEAD`, `SBS_MROW`, `SBS_NUM`,
`SBS_RANGE`, `SBS_MARK`, `SBS_TITLE`, `SBS_COUNT`, `SBS_VERSION`,
`SBS_RANKBY`, `SBS_PERIOD`, `SBS_BBSCMD`, `SBS_MODCMD`. The
module page's columns are laid out in C (so hidden columns close up) and
passed whole through `SBS_MHEAD`/`SBS_MROW`. Everything else on screen is
editable `.MSG` text.
