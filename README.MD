<div align="center">

```
       ██████╗ ██████╗ ███████╗   ███████╗████████╗ █████╗ ████████╗███████╗
       ██╔══██╗██╔══██╗██╔════╝   ██╔════╝╚══██╔══╝██╔══██╗╚══██╔══╝██╔════╝
       ██████╔╝██████╔╝███████╗   ███████╗   ██║   ███████║   ██║   ███████╗
       ██╔══██╗██╔══██╗╚════██║   ╚════██║   ██║   ██╔══██║   ██║   ╚════██║
       ██████╔╝██████╔╝███████║   ███████║   ██║   ██║  ██║   ██║   ███████║
       ╚═════╝ ╚═════╝ ╚══════╝   ╚══════╝   ╚═╝   ╚═╝  ╚═╝   ╚═╝   ╚══════╝
```

*Find out what your callers really play.*

**System and module usage statistics for The Major BBS v10.**

By **[Sysop Network](https://github.com/SysopNetwork)** — https://github.com/SysopNetwork/BBS-Stats

[![The Major BBS](https://img.shields.io/badge/The%20Major%20BBS-v10-blue)](https://github.com/SysopNetwork/BBS-Stats)
[![Language](https://img.shields.io/badge/language-C-555555?logo=c&logoColor=white)]()
[![Platform](https://img.shields.io/badge/platform-Windows%20x86%20(Win32)-lightgrey?logo=windows&logoColor=white)]()
[![License](https://img.shields.io/badge/license-MIT-blue?logo=opensourceinitiative&logoColor=white)](LICENSE)
[![Release](https://img.shields.io/github/v/release/SysopNetwork/BBS-Stats)](https://github.com/SysopNetwork/BBS-Stats/releases)

📋 **[See what's new in v1.0.0 →](CHANGELOG.MD)** · 🛠️ **[Technical reference →](TECHNICAL.MD)** · 🏷️ **[Text variables →](TEXTVARS.MD)**

</div>

---

BBS Stats watches how your board is used and puts the numbers on two pages
your callers can browse: one for the BBS itself, one for its modules. For
each module you see how many different callers used it, how often they went
in, how much time they spent there in total and the average per caller.
Every module figure is available for today, this week, this month and
all-time.

Everything about what the pages show is set by the sysop **online, while the
board runs**: no CNF options, no restart.

## ✨ Features

| | Feature | Description |
|---|---|---|
| 🎮 | **Module Stats Page** | One line per module: different users, visits, total time and average time per user |
| 📅 | **Four Periods** | Today, this week, this month and all-time, one key apart; windows roll over on their own, no nightly job |
| 🔀 | **Sort Orders** | Most time, most users, or name (the sysop's choice) |
| 📊 | **BBS Stats Page** | Up to 31 figures: online now and peaks, logons, new users, the BBS's own call/account/message/file totals, ANSI users, modules installed, time online, uptime |
| 🖥️ | **Server Health** | CPU load (the whole machine and the BBS server's share), free memory, free disk space |
| 📣 | **Logon Advertisement** | A "most popular modules" list at logon, ranked by time, users or visits, in three layouts, with a preview and keys to see or hide it |
| ⌨️ | **Global Commands** | `/BBSSTATS` and `/MODSTATS [T\|W\|M\|A]` from anywhere on the BBS; rename or switch them off |
| 🙈 | **Show / Hide Anything** | Every BBS page item, every module page column, every module |
| ⏸️ | **Stop Tracking or Reset** | Stop counting a module's time, or delete its statistics |
| 🔑 | **Access Keys** | An optional key per page; sysop time counted or ignored |
| 📝 | **Editable Text** | All screen text in `SNBBSS.MSG`; changing values arrive through named `SBS_*` text variables |
| ⚡ | **Live Configuration** | Every setting changes online, applies at once and is written to the Audit Trail |
| 🪶 | **Light on the Board** | One sample a second, and long jobs done in small batches so the board never waits |

---

## 🚀 Installation

### Requirements

- The Major BBS v10 (BBS Stats is a 32-bit Win32 module, as all v10 modules
  are; it runs normally on 64-bit Windows)
- A free slot in the Menu Tree to add the module

### Install a release

1. Download the latest **[release](https://github.com/SysopNetwork/BBS-Stats/releases)**
   zip and extract it.
2. Shut the BBS down.
3. Copy `SNBBSS.DLL`, `SNBBSS.MDF`, `SNBBSS.MSG` and `SNBBSS.RLN` into your
   Major BBS v10 directory.
4. Start the BBS, then use the **Menu Tree Editor** to add **BBS Stats** to
   a menu.
5. The Audit Trail shows **BBS Stats loaded** with the running version.

The module creates its three data files (`SNBBSS.DAT`, `SNBBSSM.DAT`,
`SNBBSSU.DAT`) the first time it starts, and the host compiles
`SNBBSS.MSG` to `SNBBSS.MCV`. Statistics start from the day the module is
installed.

### Upgrading

Shut the BBS down and copy the new files over the old ones. The DLL and the
MSG must always be installed together. Your statistics and settings live in
the `SNBBSS*.DAT` files and are kept. If you have reworded any messages,
re-apply your changes after replacing `SNBBSS.MSG`.

---

## ⚙️ Configuration

Enter BBS Stats as a sysop and press **S** for *Sysop configuration*:

| Option | What it does |
|---|---|
| 1. General settings | Sysop time, default period, sort order, week start, page access keys, global command names |
| 2. BBS page items | Show or hide each figure on the BBS stats page |
| 3. Module page columns | Show or hide users, visits, total time, average per user |
| 4. Modules shown | Pick which modules the module stats page lists, stop tracking a module, or delete its statistics |
| 5. Logon advertisement | Switch the ad on or off and choose what it lists and how. **P** previews it |

On the show/hide lists, type a number to switch an entry, **A** to show all
or **N** to hide all.

The *Modules shown* list also takes two commands with a module's number:

- **T** (for example `T5`) stops tracking that module, or starts it again.
  Time spent in an untracked module isn't counted, and the module is left
  off the module stats page and the logon ad. Its figures so far are kept.
- **D** (for example `D5`) deletes the module's statistics, after you
  confirm. A module that is no longer installed is removed from the list
  altogether. One that is still installed starts again from zero and keeps
  its show/hide and tracking choices, so use **T** as well to keep it out.

The advertisement is on by default. Set a key needed to see it to limit
it to some callers, or a key that hides it to spare some callers (that key
wins if a caller has both). Its wording and colors are the `ADHDR`, `ADTROW`, `ADLROW` and `ADFTR` messages in
`SNBBSS.MSG`; `SBS_TITLE` (the headline), `SBS_COUNT`, `SBS_RANKBY` and
`SBS_PERIOD` can be placed anywhere in them.

All screen text lives in `SNBBSS.MSG` and can be reworded or recolored in
the BBS's message editor. Changing values arrive through named text
variables (`SBS_...`), so they can be moved or removed freely.
[TEXTVARS.MD](TEXTVARS.MD) lists every variable and the messages it fills.

---

## 🎮 Usage

From the module's menu:

- **B**: the BBS stats page
- **M**: the module stats page. Press **T**, **W**, **M** or **A** to
  switch between today, this week, this month and all-time.
- **X**: leave the module

Or from anywhere on the BBS, without entering the module:

- **/BBSSTATS**: the BBS stats page
- **/MODSTATS**: the module stats page for the default window. Add **T**,
  **W**, **M** or **A** (for example `/MODSTATS W`) to pick the period.

The same page keys apply to the commands as to the module's menu. After
the page, the caller is returned to the prompt they were at.

Both names can be changed, or either command switched off with **NONE**,
under *General settings*. A name is a slash followed by up to 14 letters
or digits. The two commands can't share a name, and the BBS's own commands
(`/GO`, `/P`, `/RECENT`, `/L`, `/INVIS`) can't be used. If another module
on your board has a command of the same name, whichever module loads first
gets it, so pick a different name. ML, for one, has its own `/STATS`.

---

## 📏 How Usage Is Measured

Once a second, BBS Stats looks at every channel to see which module each
logged-on caller is in. Time is counted to about the second. A visit is
counted each time a caller enters a module. Callers still signing up or
part way through logging on or off are not counted. Statistics start from
the day the module is installed; nothing is imported from before.

---

## 🔨 Building from Source

BBS Stats is a Win32 (x86) C DLL built with Visual Studio 2022 (toolset
v143) against The Major BBS v10 Module SDK. Copy `MBBSSDK.props.example` to
`MBBSSDK.props`, point it at your SDK, then:

```
MSBuild SNBBSS.sln -p:Configuration=Release -p:Platform=Win32
```

> **x86 only — never x64.** The Major BBS v10 host is 32-bit and will not
> load a 64-bit module.

See **[BUILDING.MD](BUILDING.MD)** for prerequisites, tests and packaging.

---

## 📁 Architecture

```
BBS-Stats/
├── SRC/SNBBSS/
│   ├── SNBBSS.C/.H        # Entry point, menus, both stats pages, sysop editor
│   ├── SBTRACK.C/.H       # Once-a-second usage sampler, module summary cache
│   ├── SBBBS.C/.H         # BBS-wide counters and the BBS page item table
│   ├── SBCFG.C/.H         # Live settings: defaults, getters, editor rows
│   ├── SBSTORE.C/.H       # All Btrieve access and record layouts
│   ├── SBSYS.C/.H         # Server CPU, memory and disk figures
│   ├── SBTVAR.C/.H        # Named text variables (SBS_*)
│   ├── SBDATE.C/.H        # Day / week / month window stamps
│   ├── SBFMT.C/.H         # Number and duration formatting
│   ├── SBVER.H            # The version, shared by code and resource
│   ├── SNBBSS.MSG         # Screen text (generated by TOOLS/GENMSG.PY)
│   ├── MSGNUM.H           # Message numbers (generated)
│   ├── SNBBSS.MDF         # Module definition file
│   └── SNBBSS.rc          # Windows version resource
├── TESTS/                 # Unit tests for the date, format and system code
├── TOOLS/
│   ├── GENMSG.PY          # Generates SNBBSS.MSG and MSGNUM.H (CP437, CRLF)
│   ├── PACKAGE.PY         # Packages a release into DIST/<version>/
│   └── RUNTESTS.BAT       # Builds and runs the unit tests
├── TECHNICAL.MD           # Technical reference
├── TEXTVARS.MD            # Every text variable and the messages it fills
├── BUILDING.MD            # Build, test and package guide
├── CHANGELOG.MD           # Release notes
├── MBBSSDK.props.example  # SDK path template
└── SNBBSS.sln
```

Statistics are stored in three Btrieve files through the DFA API:
`SNBBSS.DAT` (settings and BBS counters), `SNBBSSM.DAT` (one summary per
module) and `SNBBSSU.DAT` (one record per module and user, for distinct-user
counts).

---

## 💬 Support

Questions and bug reports: open an issue on the
[GitHub repository](https://github.com/SysopNetwork/BBS-Stats/issues).

---

## 🏆 Credits

BBS Stats was created by Jacque Steyn (The Adept BBS) and Mark Laudenbach.

BBS Stats is developed and maintained by [Mark Laudenbach](https://github.com/laudenbachm) at [Sysop Network](https://github.com/SysopNetwork).

---

## 💖 Tips / Donations

- **Cash App:** [$laudenbachm](https://cash.app/$laudenbachm)
- **Ko-fi:** [ko-fi.com/laudenbachm](https://ko-fi.com/laudenbachm)

---

## 📄 License

MIT — Copyright (c) 2026 Sysop Network. See [LICENSE](LICENSE).
