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
| Public API surface | `main.c` | ✅ `filterStringForShellInjection`/`filterStringForHtmlInjection`/`AmmServer_StringHasSafePath` all implemented properly (were stubs), verified incl. real shell round-trip + symlink-bypass tests ; ✅ `_SESSION*` family + `AmmServer_SetCookie()` implemented from stubs ; ✅ `AmmServer_GenerateCSRFToken`/`ValidateCSRFToken` added, session-bound (issues.md, MyURL CSRF) | 🟡 2 bugs fixed (issues.md #1 `AmmServer_DynamicRequestReturnMemoryHandler`, #2 `SIGKILL` registration); rest of file not yet reviewed | 🟡 supersedes `HabChan/csrf.c`'s weaker, non-session-bound (global token pool, unseeded `rand()`) CSRF implementation — HabChan not yet migrated | 🟡 ad-hoc tests for the sanitizers/safe-path/sessions/CSRF only (scratchpad, not in-repo) | |
| Configuration | `server_configuration.c/h` | | | | | |
| Accept loop + epoll accept layer | `threads/threadedServer.c` | | | | | |
| Epoll static-content fast path | `threads/epollFastPathServer.c` | | 🟡 keepalive default logic updated to match http_header_analysis.c's fix (issues.md); rest of file not re-reviewed | | | |
| Per-connection serving | `threads/clientServer.c` | | | | | |
| Fresh worker threads | `threads/freshThreads.c` | | | | | |
| Prespawned thread pool | `threads/prespawnedThreads.c` | | | | | |
| Socket abstraction (plain/TLS) | `network/networkAbstraction.c` | | | | | |
| TLS/SSL wiring | `network/openssl_server.c` | | | | | |
| File/response transmission | `network/file_server.c` | ✅ symlink-escape guard added at `SendFile()` (clean early 400) and `TransmitFileToSocket()` (disk-streaming fallback) — found via live testing that the caching-layer fix alone missed this second path entirely (issues.md) | | | | |
| Response header sending | `network/sendHTTPHeader.c` | | | 🟡 `SendSuccessCodeHeader()` now emits `transaction->pendingResponseHeaders` (Set-Cookie etc — issues.md "Sessions") | | |
| Request header receiving | `network/recvHTTPHeader.c` | | | | | |
| Request line + header dispatch | `header_analysis/http_header_analysis.c` | | 🟡 keep-alive default fixed (issues.md) — HTTP/1.1 now defaults to keep-alive per spec, verified +4x throughput impact; rest of file not fully reviewed | | | |
| Header tokenizing/buffer growth | `header_analysis/generic_header_tools.c` | | | | | |
| POST header parsing | `header_analysis/post_header_analysis.c` | | | | | |
| POST body/multipart parsing | `header_analysis/post_data.c` | | ✅ 5 issues fixed (2 real bounds bugs, 2 stale TODOs removed, `success`/return-value semantics fixed — see issues.md); verified live via MyLoader upload + malformed-multipart robustness test | | | |
| GET query parsing | `header_analysis/get_data.c` | | 🟡 issues.md #3 prefix-match lookup fixed; possible dropped-trailing-field bug spotted, not yet confirmed/fixed | | | |
| Cookie parsing | `header_analysis/cookie_data.c` | | 🟡 issues.md #3 prefix-match lookup fixed (length-bounded, not strcmp — see rationale in issues.md) | | | |
| Static file + dynamic resource cache | `cache/file_caching.c` | ✅ symlink-escape guard added to the one-time (per-unique-file, not per-request) disk-load path in `cache_GetResource()`, covers both `SendFile()` and the epoll fast path (issues.md) | | | | |
| Dynamic content dispatch (SAME/DIFFERENT_PAGE) | `cache/dynamic_requests.c` | | ✅ `cacheMemory`/`shared_context->requestContext.content` now re-synced from `rqst->content` after a callback returns, fixing a callback whose content buffer gets `realloc()`'d (e.g. via `AmmServer_ReplaceVariableInMemoryHandler` growing) previously being served stale/freed memory — verified live under a 100-request concurrent burst (issues.md, MyURL CSRF) | | | |
| Client list / ban tracking | `cache/client_list.c` | | | | | |
| Session storage | `cache/session_list.c` | ✅ implemented from a complete stub: PHP-`$_SESSION`-style cookie-based store, CSPRNG tokens, salted-hash user-account integration (issues.md §5 "Sessions") | | | 🟡 live end-to-end tested (scratchpad, not in-repo) — cookie issuance/reuse/expiry, login/logout, 200-concurrent-request race test, capacity eviction | |
| Response compression | `cache/file_compression.c` | | | | | |
| Logging | `tools/logs.c` | ✅ `system()` call in `compressLog()` audited (issues.md) — only ever called with server-configured log paths, no client input reaches it | | | | |
| Date/time formatting | `tools/time_provider.c` | | | | | |
| Path safety / misc HTTP helpers | `tools/http_tools.c` | ✅ `popen()` in `ServerThreads_DropRootUID()` audited (issues.md) — username is a server-config constant, not client input, no fix needed ; added shared `PathResolvesWithinDirectory()` (canonicalization core, reused by `AmmServer_StringHasSafePath()` and the file-serving/caching guards) without touching `FilenameStripperOk()` itself — that one stays string-only deliberately, since it runs on the hot per-request/fast-path route where a `realpath()` syscall per request wasn't acceptable | | | | |
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
| Hashmap | `src/Hashmap/` | | 🐛 found (not fixed, unreachable) `hashMap_GetPayload()` is broken — pass-by-value out-param never reaches the caller (issues.md "Sessions") | | 🟡 standalone ASan test for the two new functions only (scratchpad, not in-repo) | |
| InputParser | `src/InputParser/` | | | | | |
| BasicImaging | `src/BasicImaging/` | | | | | |
| AmmCaptcha | `src/AmmCaptcha/` | | | | | |
| AmmClient | `src/AmmClient/` | | | | | |
| AmmMessages | `src/AmmMessages/` | | | | | |
| UserAccounts | `src/UserAccounts/` | ✅ plaintext passwords → salted/iterated SHA-256 (new `sha256.c`/`.h`) ; ✅ weak unseeded-`rand()` session-ID generation → CSPRNG ; ✅ session-ID-in-URL → real cookie-based login added (`AmmServer_Login`/`Logout`/`CurrentUsername`, additive — legacy `?s=` idiom kept intact since `Social`/`ShareTex` still depend on it) (issues.md "Sessions") | | 🟡 `Social`/`ShareTex` not yet migrated to the new cookie-based login API — still on the legacy URL-param idiom | 🟡 standalone ASan test for password hashing only (scratchpad, not in-repo) | |
| StringRecognizer (generator for stringscanners) | `src/StringRecognizer/` | | | | | |

## Services — `src/Services/`

| Service | Path | Security Gaps | Bugs | Common-Module Refactor | Automated Tests | Optimized |
|---|---|---|---|---|---|---|
| AmmarServer (reference/demo) | `src/Services/AmmarServer/` | ✅ `/execute.html` audited (issues.md) — only ever runs the server's own `-e` startup CLI argument, operator-controlled not client-controlled, no fix needed | | | | |
| SimpleTemplate (starter template) | `src/Services/SimpleTemplate/` | | | | | |
| MyURL | `src/Services/MyURL/` | ✅ CSRF on `/go`'s URL-creation branch fixed — session-bound `AmmServer_GenerateCSRFToken`/`ValidateCSRFToken`, verified live (upstream #27, issues.md) | | | | |
| MyLoader | `src/Services/MyLoader/` | ✅ arbitrary-file-write risk closed — `AmmServer_StringHasSafePath()` (gates both its `/vfile.html` read-by-name and its upload write path) now does real `realpath()`-based canonicalization, not just a character blacklist (issues.md) | | | | |
| MyBlog | `src/Services/MyBlog/` | | | | | |
| MyTube | `src/Services/MyTube/` | ✅ command-injection fixed: YouTube video title (attacker-influenceable, reachable by default since `DO_YOUTUBE_DOWNLOADING=1`) whitelist-filtered before it reaches any filename/ffmpeg command (issues.md) | | | | |
| MySearch | `src/Services/MySearch/` | | | | | |
| GeoPosShare | `src/Services/GeoPosShare/` | | | | | |
| Social | `src/Services/Social/` | | | | | |
| HabChan | `src/Services/HabChan/` | 🟡 found (not fixed) `csrf.c`'s CSRF defense is weaker than it looks: tokens come from unseeded `rand()` and validity is checked against a global "was this token issued to *anyone* recently" pool, not bound to the requester's own session — an attacker can obtain a valid token themselves and replay it against a different victim. `main.c`'s new session-bound `AmmServer_GenerateCSRFToken`/`ValidateCSRFToken` is the recommended migration target (issues.md, MyURL CSRF) | | | | |
| ShareTex | `src/Services/ShareTex/` | | | | | |
| Availability | `src/Services/Availability/` | | | | | |
| SuperMarket | `src/Services/SuperMarket/` | | | | | |
| WebFramebuffer | `src/Services/WebFramebuffer/` | | | | | |
| V4L2ToHTTP | `src/Services/V4L2ToHTTP/` | | | | | |
| ImageGeneration | `src/Services/ImageGeneration/` | ✅ audited (issues.md) — client input reaches `system()` but is already whitelist-filtered via `filterQuery()`, no fix needed there ; also benefits from the `AmmServer_StringHasSafePath()` fix (same upload-write pattern as MyLoader) | | | | |
| AmmBus | `src/Services/AmmBus/` | | | | | |
| APushService | `src/Services/APushService/` | | | | | |
| MyRemoteDesktop (opt-in, `USE_XSERVER`) | `src/Services/MyRemoteDesktop/` | ✅ `/cmd` keystroke path hardened against shell metacharacters, defense-in-depth (issues.md) — was already double-gated (compile+runtime flags off by default) | | | | |
| Various/NaoNetWalk | `src/Services/Various/NaoNetWalk/` | | | | | |
| Various/ScriptRunner | `src/Services/Various/ScriptRunner/` | ✅ command-injection fixed: `?say=` text (client-controlled, no auth, built unconditionally) blacklist-filtered before reaching a two-layer-quoted shell command, UTF-8 preserved (issues.md) | | | | |
| Deprecated/CinemaPilot | `src/Services/Deprecated/CinemaPilot/` | ✅ audited (issues.md) — all `system()` calls hardcoded or fed from a server-side playlist file, no client-controlled input reaches them, no fix needed | | | | |
| Apolls *(not currently built)* | `src/Services/Apolls/` | | | | | |
| Deprecated/SQLiteServer *(not currently built)* | `src/Services/Deprecated/SQLiteServer/` | | | | | |
| Various/libkindrvserver *(not currently built)* | `src/Services/Various/libkindrvserver/` | | | | | |
