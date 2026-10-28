# Changelog

All notable changes to BBS Stats are recorded here. Versions follow
Semantic Versioning.

## [1.0.0] - 2026-10-27

First release.

### Added
- Module usage tracking: once a second, every logged-on caller's current
  module is sampled. Time and visits are written per module and per user.
  Windows roll over on their own; no nightly job is needed.
- Module stats page: distinct users, visits, total time and average time
  per user for every module, for today, this week, this month and
  all-time, with a one-key switch between periods and a choice of sort
  order (most time, most users, name).
- BBS stats page with 31 figures: online now and peaks, logons, new
  users, the BBS's own call/account/message/file totals, ANSI users,
  modules installed, total time online, uptime, and server health (CPU
  load, free memory, free disk space).
- Logon advertisement: a "most popular modules" list shown to callers as
  they log on. Ranked by time played, users or visits, for any period,
  1-20 modules, in three layouts, with a headline of the sysop's own, a
  preview, and optional keys to see or hide it.
- Global commands `/BBSSTATS` and `/MODSTATS [T|W|M|A]` show the pages
  from anywhere on the BBS. The sysop can rename either or switch it off.
- In-module sysop configuration, applied live and logged to the Audit
  Trail: count sysop time, default period, sort order, week start, access
  keys per page, global command names, show/hide for every BBS page item,
  module page column and module, and the logon advertisement.
- Stop tracking a module (`T#`) or delete its statistics (`D#`) from the
  Modules shown list.
- All screen text in `SNBBSS.MSG`, with named text variables (`SBS_*`,
  listed in `TEXTVARS.MD`) so every screen can be reworded and recolored.
- Per-user module records are removed when an account is deleted.
