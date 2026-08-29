# AmmarServer — Module Review Tracker

A checklist of every module/library/service in the repository, to be worked through incrementally. For each
row, in order:

1. **Security Gaps** — identify and fix any security-relevant issues.
2. **Bugs** — identify and fix any correctness bugs.
3. **Common-Module Refactor** — identify any API workflow duplicated here that's also duplicated elsewhere and
   worth extracting into (or reusing from) a shared module.
4. **Automated Tests** — does an automated test suite exist for this module? (Not load/stress testing —
   correctness tests.)
5. **Optimized** — has this module's computational performance been reviewed/optimized (e.g. the SIMD work
   already done on `BasicImaging`'s resize path)?

All status columns start blank. See `knowledge/ammarserver.md` for how each module works and
`knowledge/issues.md` for the issues already found as of this writing (useful starting context when a row's
turn comes up, but not a substitute for actually reviewing the row).

**Legend for filling in cells as work happens:** ✅ done · 🟡 in progress / partial · ❌ reviewed, issue(s) found
and open · — not yet reviewed (default/starting state).

## Core library — `src/AmmServerlib/`

| Module | Path | Security Gaps | Bugs | Common-Module Refactor | Automated Tests | Optimized |
|---|---|---|---|---|---|---|
| Public API surface | `main.c` | | 🟡 2 bugs fixed (issues.md #1 `AmmServer_DynamicRequestReturnMemoryHandler`, #2 `SIGKILL` registration); rest of file not yet reviewed | | | |
| Configuration | `server_configuration.c/h` | | | | | |
| Accept loop + epoll accept layer | `threads/threadedServer.c` | | | | | |
| Epoll static-content fast path | `threads/epollFastPathServer.c` | | | | | |
| Per-connection serving | `threads/clientServer.c` | | | | | |
| Fresh worker threads | `threads/freshThreads.c` | | | | | |
| Prespawned thread pool | `threads/prespawnedThreads.c` | | | | | |
| Socket abstraction (plain/TLS) | `network/networkAbstraction.c` | | | | | |
| TLS/SSL wiring | `network/openssl_server.c` | | | | | |
| File/response transmission | `network/file_server.c` | | | | | |
| Response header sending | `network/sendHTTPHeader.c` | | | | | |
| Request header receiving | `network/recvHTTPHeader.c` | | | | | |
| Request line + header dispatch | `header_analysis/http_header_analysis.c` | | | | | |
| Header tokenizing/buffer growth | `header_analysis/generic_header_tools.c` | | | | | |
| POST header parsing | `header_analysis/post_header_analysis.c` | | | | | |
| POST body/multipart parsing | `header_analysis/post_data.c` | | | | | |
| GET query parsing | `header_analysis/get_data.c` | | 🟡 issues.md #3 prefix-match lookup fixed; possible dropped-trailing-field bug spotted, not yet confirmed/fixed | | | |
| Cookie parsing | `header_analysis/cookie_data.c` | | 🟡 issues.md #3 prefix-match lookup fixed (length-bounded, not strcmp — see rationale in issues.md) | | | |
| Static file + dynamic resource cache | `cache/file_caching.c` | | | | | |
| Dynamic content dispatch (SAME/DIFFERENT_PAGE) | `cache/dynamic_requests.c` | | | | | |
| Client list / ban tracking | `cache/client_list.c` | | | | | |
| Session storage | `cache/session_list.c` | | | | | |
| Response compression | `cache/file_compression.c` | | | | | |
| Logging | `tools/logs.c` | | | | | |
| Date/time formatting | `tools/time_provider.c` | | | | | |
| Path safety / misc HTTP helpers | `tools/http_tools.c` | | | | | |
| Directory listing generation | `tools/directory_lists.c` | | | | | |
| IP geolocation | `tools/geolocation.c` | | | | | |
| Built-in monitor.html page | `tools/serverMonitor.c` | | | | | |
| Built-in error pages | `templates/errors.c` | | | | | |
| Directory-listing icons | `templates/icons.c` | | | | | |
| Built-in editor UI | `templates/editor.c` | | | | | |
| Built-in login UI | `templates/login.c` | | | | | |
| IP ban-list execution | `security/banlist.c` | | | | | |
| Periodic callback scheduler | `scheduler/scheduler.c` | | 📋 deferred — full stub, needs real implementation (design notes in issues.md §5) | | | |
| Generated method/header/content-type lexers | `stringscanners/*.c` | | | | | |
| Template-variable substitution / file I/O helper | `AString/AString.c` | | ✅ realloc-grow path audited + fixed (issues.md), verified under ASan/UBSan | | 🟡 standalone ASan test written for this fix (not a full suite — covers grow/shrink/shrink-then-grow/repeated-grow) | |

## Supporting libraries — `src/`

| Module | Path | Security Gaps | Bugs | Common-Module Refactor | Automated Tests | Optimized |
|---|---|---|---|---|---|---|
| Hashmap | `src/Hashmap/` | | | | | |
| InputParser | `src/InputParser/` | | | | | |
| BasicImaging | `src/BasicImaging/` | | | | | |
| AmmCaptcha | `src/AmmCaptcha/` | | | | | |
| AmmClient | `src/AmmClient/` | | | | | |
| AmmMessages | `src/AmmMessages/` | | | | | |
| UserAccounts | `src/UserAccounts/` | | | | | |
| StringRecognizer (generator for stringscanners) | `src/StringRecognizer/` | | | | | |

## Services — `src/Services/`

| Service | Path | Security Gaps | Bugs | Common-Module Refactor | Automated Tests | Optimized |
|---|---|---|---|---|---|---|
| AmmarServer (reference/demo) | `src/Services/AmmarServer/` | | | | | |
| SimpleTemplate (starter template) | `src/Services/SimpleTemplate/` | | | | | |
| MyURL | `src/Services/MyURL/` | | | | | |
| MyLoader | `src/Services/MyLoader/` | | | | | |
| MyBlog | `src/Services/MyBlog/` | | | | | |
| MyTube | `src/Services/MyTube/` | | | | | |
| MySearch | `src/Services/MySearch/` | | | | | |
| GeoPosShare | `src/Services/GeoPosShare/` | | | | | |
| Social | `src/Services/Social/` | | | | | |
| HabChan | `src/Services/HabChan/` | | | | | |
| ShareTex | `src/Services/ShareTex/` | | | | | |
| Availability | `src/Services/Availability/` | | | | | |
| SuperMarket | `src/Services/SuperMarket/` | | | | | |
| WebFramebuffer | `src/Services/WebFramebuffer/` | | | | | |
| V4L2ToHTTP | `src/Services/V4L2ToHTTP/` | | | | | |
| ImageGeneration | `src/Services/ImageGeneration/` | | | | | |
| AmmBus | `src/Services/AmmBus/` | | | | | |
| APushService | `src/Services/APushService/` | | | | | |
| MyRemoteDesktop (opt-in, `USE_XSERVER`) | `src/Services/MyRemoteDesktop/` | | | | | |
| Various/NaoNetWalk | `src/Services/Various/NaoNetWalk/` | | | | | |
| Various/ScriptRunner | `src/Services/Various/ScriptRunner/` | | | | | |
| Deprecated/CinemaPilot | `src/Services/Deprecated/CinemaPilot/` | | | | | |
| Apolls *(not currently built)* | `src/Services/Apolls/` | | | | | |
| Deprecated/SQLiteServer *(not currently built)* | `src/Services/Deprecated/SQLiteServer/` | | | | | |
| Various/libkindrvserver *(not currently built)* | `src/Services/Various/libkindrvserver/` | | | | | |
