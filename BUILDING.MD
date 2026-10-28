# Building BBS Stats

## Requirements

- Microsoft Visual Studio 2022 (or the 2022 Build Tools) with the
  **Desktop development with C++** workload and the **v143** toolset.
- The Major BBS v10 Module SDK
  (https://github.com/TheMajorBBS/MBBS-v10-module-SDK).
- Python 3, to regenerate the message file and to package releases.
- A Major BBS v10 test system for deployment and runtime testing.

> **Build for x86 (Win32) only.** The Major BBS v10 host process is 32-bit
> and cannot load a 64-bit module.

## SDK location

Copy `MBBSSDK.props.example` to `MBBSSDK.props` (git-ignored) and set
`MbbsSdkDir` to your SDK folder (trailing backslash required). The project
imports it, so the committed project file never contains a local path.

## Build the DLL

From the project root:

```
MSBuild SNBBSS.sln -p:Configuration=Release -p:Platform=Win32
```

Output: `BUILD\Release\SNBBSS.dll`. The project links `wgserver_lib.lib` and
`galgsbl_lib.lib`, compiles with `/J` (unsigned `char`) and 8-byte struct
alignment, exports `init__snbbss` through `SNBBSS_EXP.DEF`, and takes its
version resource from `SRC\SNBBSS\SBVER.H` through `SNBBSS.rc`.

Check a build with `dumpbin`: `/exports` must list `_init__snbbss`, and
`/headers` must show machine `14C (x86)`.

## Message file

`TOOLS\GENMSG.PY` is the source of truth for all screen text. It writes
`SRC\SNBBSS\SNBBSS.MSG` (CP437, CRLF line endings) and the matching
`SRC\SNBBSS\MSGNUM.H`:

```
python TOOLS\GENMSG.PY
```

Add new messages only at the **end** of the generator's list, so existing
message numbers never move. The BBS compiles the `.MCV` from the deployed
`.MSG` itself; there is no MCV build step. The DLL and `.MSG` deploy
together as a matched set.

## Unit tests

The calendar-window and number-formatting code has no BBS dependencies and
has console tests:

```
TOOLS\RUNTESTS.BAT
```

It sets up the x86 compiler itself, builds each test into `BUILD\TESTS`, and
prints `PASS` per test (exit code 0 when all pass).

## Packaging a release

1. Bump the version in `SRC\SNBBSS\SBVER.H`: the numbers, the string, and
   always `SB_VER_BUILD` (+1 for every packaged build, never reset). The
   DLL resource, Audit Trail entry and on-screen version all come from there.
2. Update `CHANGELOG.MD`.
3. Build Release and regenerate the message file.
4. Write the release notes as `DIST\<version>\SNBBSS.RLN`. The MDF names
   it as the module's online manual.
5. Run `python TOOLS\PACKAGE.PY`. It copies `SNBBSS.DLL`, `SNBBSS.MDF`,
   `SNBBSS.MSG` and `SNBBSS.RLN` into `DIST\<version>\` and builds
   `BBS-Stats-v<version>.zip`, the file attached to the GitHub release.
   `DIST\` is not committed. It refuses a version that is already packaged,
   because a version number names exactly one binary. Bump it instead.

## Deploying to a test system

With the BBS shut down, copy the contents of `DIST\<version>\` into the BBS
directory, start the BBS, and add BBS Stats to a menu with the Menu Tree
Editor. The Audit Trail entry **BBS Stats loaded** names the running version.
Deploying the `.MSG` replaces any text the sysop has edited, so ship it only
when it has changed.
