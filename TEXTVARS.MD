# BBS Stats: Text Variables

Every changing value on BBS Stats' screens reaches `SNBBSS.MSG` through a
named text variable. You can move, recolor, repeat or remove any of them
in the BBS's message editor without touching the module.

There are two kinds:

- **Live** variables read the running build or the current settings, so
  they're correct in any message, including another module's.
- **Slot** variables are filled just before the messages listed for them.
  Used anywhere else, a slot shows whatever it last held, often from
  another screen or another caller.

Names never change once released, so edits you make to `SNBBSS.MSG` keep
working across upgrades.

## Live variables

| Variable | Displays | Example |
|---|---|---|
| `SBS_VERSION` | The running build's version | `1.0.0` |
| `SBS_BBSCMD` | The BBS stats global command, as set under General settings (blank when switched off) | `/BBSSTATS` |
| `SBS_MODCMD` | The module stats global command, as set under General settings (blank when switched off) | `/MODSTATS` |

## Slot variables

| Variable | Displays | Messages |
|---|---|---|
| `SBS_LABEL` | A name. Which one depends on the message: | |
| | a BBS stats page item | `BBSROW` |
| | a setting's name | `SETROW`, `SETASK`, `SETOK` |
| | an entry on a show/hide list ("Name (not tracked)" for an untracked module) | `TOGROW` |
| | a module's name in the ad's table layout | `ADTROW` |
| | the module whose statistics are being deleted | `DELASK`, `DELGONE`, `DELRST` |
| `SBS_VALUE` | The value that goes with `SBS_LABEL`: | |
| | a BBS stat, e.g. `1,234`, `3h 12m`, `42%` | `BBSROW` |
| | a setting's current value, e.g. `Monday` | `SETROW`, `SETASK`, `SETOK` |
| | the module's figure in the ad (blank when figures are switched off) | `ADTROW` |
| `SBS_NUM` | A row number to pick or rank by: `1`, `2`, ... | `SETROW`, `TOGROW`, `ADTROW` |
| `SBS_COUNT` | The highest number a prompt accepts (the N in "1-N") | `SETPRM`, `TOGPRM`, `ADSPRM`, `MODTPRM` |
| | the number of modules in the ad ("Top 5") | `ADHDR` |
| `SBS_RANGE` | What a setting accepts, e.g. `0 = Sunday, 1 = Monday` | `SETASK`, `SETBAD` |
| `SBS_MARK` | `*` for an entry that is shown, a blank for one that is hidden | `TOGROW` |
| `SBS_TITLE` | A show/hide list's title: `BBS PAGE ITEMS`, `MODULE PAGE COLUMNS` or `MODULES SHOWN` | `TOGHDR` |
| | the ad's headline (yours, or "The most popular modules on this BBS") | `ADHDR` |
| `SBS_WINDOW` | The module stats page's period: `Showing today`, `Showing this week`, `Showing this month` or `Showing all-time` | `MODHDR` |
| `SBS_MHEAD` | The module stats page's column headings, for the columns you have switched on | `MODHDR` |
| `SBS_MROW` | A line already laid out in columns: | |
| | the dashes under the headings, as wide as `SBS_MHEAD` | `MODHDR` |
| | one module's row on the module stats page | `MODROW` |
| | one line of the ad's two-column or compact layout | `ADLROW` |
| `SBS_RANKBY` | What the ad ranks by: `time played`, `users` or `visits` | `ADHDR` |
| `SBS_PERIOD` | The ad's period: `today`, `this week`, `this month` or `all-time` | `ADHDR` |

## Notes

- `SBS_MHEAD` and `SBS_MROW` arrive as whole lines, lined up by the module.
  You can recolor or move the line but not its separate columns. In the
  ad's table layout each module comes through `ADTROW` as `SBS_NUM`,
  `SBS_LABEL` and `SBS_VALUE`, so every part of it can be styled.
- `SBS_VERSION` is used in `MAINMNU`. Showing the version this way, not
  typing it into the text, keeps it right after a DLL-only update.
- Messages with no text variables: `SYSOPT`, `MAINPRM`, `BADSEL`, `NOACC`,
  `BBSHDR`, `BBSFTR`, `MODNONE`, `MODPRM`, `CFGMNU`, `CFGPRM`, `SETHDR`,
  `TOGBAD`, `EXITMSG`, `ADFTR`, `ADNONE`, `ADSHDR`, `DELBUSY` and `DELNO`.
  `GMODFTR` and `GMODUSE` use only `SBS_MODCMD`.
