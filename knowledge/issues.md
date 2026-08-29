# AmmarServer — Known Issues and Improvement Opportunities

Compiled from direct code reading of `src/AmmServerlib/` (this session, plus three parallel surveys of the
parts not already touched), and from this session's own architecture/benchmarking work. Organized by category,
roughly most-actionable first. See `knowledge/ammarserver.md` for how the surrounding code works.

## 1. Correctness bugs found by direct reading

**✅ FIXED** — **`AmmServer_DynamicRequestReturnMemoryHandler()` — two bugs in the same function** (`src/AmmServerlib/main.c`):
```c
memset(rqst->content,rqst->contentSize,0);       // args swapped: memset(ptr, value, size) — this fills 0 bytes
memcpy(rqst->content,content->content,content->contentSize);
rqst->contentSize = content->contentSize+1;      // off-by-one
rqst->content[rqst->contentSize]=0;               // writes NUL one byte past where it should go
```
1. `memset` args are in the wrong order (`memset(ptr, size, 0)` instead of `memset(ptr, 0, size)`) — the call
   clears zero bytes, silently doing nothing. Currently harmless in practice since the following `memcpy`
   overwrites everything that was meant to be cleared, but it's dead/wrong code that should either be fixed or
   removed.
2. `contentSize` is set to `content->contentSize+1` before the NUL is written at `content[contentSize]` — that
   writes the NUL one byte *past* the correct position (right after the copied data), leaves the byte that
   *should* hold the NUL uninitialized/garbage, and reports the content size as one byte larger than what was
   actually copied. Should be `rqst->contentSize = content->contentSize;` then `rqst->content[rqst->contentSize]=0;`.

**Fix applied**: removed the dead `memset` call and corrected the off-by-one (`rqst->contentSize` now equals the
actual copied byte count, NUL lands immediately after it). This function is **not dead code** — it's the memory-
handler response path used by MyTube (JS/CSS/favicon), MyRemoteDesktop (background/index/simple pages),
APushService, and ImageGeneration (index page, logo, loading images/gif, generated images), all built by
default. The bug meant every response served through it (mostly binary image/JS/CSS content) carried one extra
uninitialized garbage byte and reported a `Content-Length` one byte too large. Verified: `AmmServerlib`,
`apushservice`, `imagegeneration`, and `mytube` all rebuild cleanly after the fix.

**✅ FIXED** — **`AmmServer_RegisterTerminationSignal()` tried to catch `SIGKILL`** (`main.c`):
```c
if (signal(SIGKILL, AmmServer_GlobalTerminationHandler) == SIG_ERR) { AmmServer_Warning("AmmarServer cannot handle SIGKILL!\n"); ++failures; }
```
POSIX forbids catching, blocking, or ignoring `SIGKILL` — this call can never succeed, by design of the OS, not
a bug in AmmarServer's environment. It always logged "AmmarServer cannot handle SIGKILL!" on every startup (this
warning was visible in every server's normal startup log). Beyond the log noise, it also meant this function's
return value was a lie: `failures` was incremented by the guaranteed SIGKILL failure even when SIGINT/SIGHUP/
SIGTERM all registered fine, so `AmmServer_RegisterTerminationSignal()` always returned 0 (false) regardless of
whether anything was actually wrong.

**Fix applied**: removed the `SIGKILL` registration attempt entirely (with a comment explaining why). Verified
no caller anywhere in the repo (`grep` across all Services + AmmServerlib) checks this function's return value,
so fixing its return semantics is side-effect-free. Rebuilt and confirmed the "cannot handle SIGKILL" warning no
longer appears on startup.

**✅ FIXED** — **`getGETItemFromName()` / `getCOOKIEItemFromName()` used a prefix match, not exact match**
(`header_analysis/get_data.c`, `cookie_data.c`): both did `strncmp(p->name, nameToLookFor, sizeOfNameToLookFor)`,
so looking up `"id"` would also match a field literally named `"identity"`. The equivalent function for POST
fields, `getPOSTItemFromName()` (`post_data.c`), was already fixed to do an exact NUL-bound `strcmp` — with a
comment documenting the exact bug this caused (`"captcha"` matching `"captchaID"`). The same class of bug was
still present for GET and COOKIE lookups.

**Fix applied, but not identically for both** — the two needed different fixes because the underlying data isn't
NUL-terminated the same way:
- `getGETItemFromName()`: every item that makes it into `GETItem[]` has its `name` NUL-terminated by
  `finalizeGenericGETField()` (traced all its code paths to confirm) — switched to a plain exact `strcmp()`,
  matching the POST fix exactly.
- `getCOOKIEItemFromName()`: **not** switched to `strcmp()`. `finalizeGenericCookieField()`'s own doc comment
  explains that a bare, valueless cookie name at the very end of the header line is deliberately *not*
  NUL-terminated in place (writing a NUL there would corrupt the shared header buffer's next byte). Used a
  length-bounded exact comparison instead (`nameSize` equality check, then `memcmp`), matching the pattern
  `_COOKIEcmp()`/`_COOKIEcpy()` (`main.c`) already use for cookie *values* for the identical reason.

Verified live against a running server: `GET /gps.html?latitude=51.5` (no exact `lat` field) no longer resolves
a `lat` lookup to `latitude`'s value (previously it would have); `GET /gps.html?lat=51.5&latitude=999` still
correctly resolves `lat` to `51.5`; a request carrying `Cookie: sessionid=abc123; session=shouldnotmatch`
continues to work without crashing. `AmmarServer` and `ammarserver` rebuild cleanly.

**Related finding spotted while tracing this (not fixed — separate, out of scope for this issue):**
`finalizeGenericGETField()` (`get_data.c`)'s main scan loop only handles `state==SEEKING_VALUE` in its
post-loop "final item" block (`if (state==SEEKING_VALUE) {...}`, no equivalent `SEEKING_NAME` branch) — so a
GET query string that ends in a bare name with no trailing `=`, `&`, or newline/NUL (i.e. the buffer's counted
`valueLength` simply ends mid-name) silently drops that last field instead of adding it to `GETItem[]`. Whether
this is reachable in practice depends on whether `value`/`valueLength` passed in always ends with a
terminator character by construction elsewhere in the header-parsing pipeline — not confirmed either way this
pass. Worth a dedicated look before treating it as either a real bug or a non-issue.

**`cache/session_list.c` is fully unimplemented, with undefined-behavior fallthrough**: `sessiontList_GetInfo()`
and `getSessionFromHeader()` are non-`void` functions with no `return` on some/all code paths — currently
unreachable in practice (`sessionList_initialize()` always returns `NULL`, so nothing ever calls into these),
but fragile: the moment sessions are implemented and `sessionList_initialize` starts returning something real,
these become live undefined behavior unless fixed at the same time.

**✅ AUDITED + FIXED** — **`AString.c`'s realloc-grow path was explicitly flagged untrusted by its own author**:
`astringInjectDataToMemoryHandlerOffset()`'s "replacement value is longer than the placeholder" branch (which
reallocs the buffer and shifts the remainder) carried `#warning "...not 100% this part of the code is sane..!"`
in the source, and the public API (`AmmServerlib.h`) independently doc-flags the same scenario:
`@Bug If the length of var is smaller than the length of value there might be problems (?)` on
`AmmServer_ReplaceAllVarsInMemoryHandler`, and `AString.h` itself says `astringInjectDataToMemoryHandler is not
implemented correctly , it is be buggy..!`.

**Audit result**: traced the full realloc/reverse-copy path by hand — it does **not** have an out-of-bounds
read/write. What it actually had was imprecise size bookkeeping: the realloc size was computed from
`mh->contentSize` (meant to track allocated capacity), but the *shrink* branch (`valueLength<=varLength`) never
updates `contentSize` when it shrinks `contentCurrentLength` — so after a shrink, `contentSize` is a stale,
larger-than-necessary leftover from before the shrink. A subsequent grow computed its realloc size from that
stale figure — always *enough* space (never undersized, so never unsafe), just wastefully more than needed, and
the "clean the buffer" debug-zeroing loop used the same stale index, so it zeroed already-dead bytes instead of
the genuinely-new ones (harmless in practice since nothing reads past the NUL terminator, but not what the
comment claimed it was doing).

**Fix applied**: size the reallocation off `mh->contentCurrentLength` (always exact) instead of `mh->contentSize`
(can be stale) — removes the wasteful over-allocation and makes the debug-zero loop cover the actually-new
bytes. Removed the `#warning` now that the path has been genuinely audited and corrected.

**Verification**: wrote a standalone test (`AString.c` compiles independently of the rest of AmmServerlib) with
4 scenarios — simple grow, simple shrink, a shrink-then-grow-then-shrink sequence specifically constructed to
exercise the stale-`contentSize` state the fix targets, and `astringReplaceAllInstancesOfVarInMemoryFile()`'s
repeated-grow path — compiled and run under AddressSanitizer + UBSan. All passed with zero sanitizer findings,
both before and after the fix (confirming the original code was memory-safe, just imprecise) and correct
content/length in all cases after the fix. `AmmServerlib` rebuilds clean with the `#warning` gone.

**`post_data.c` has three still-open TODOs in `finalizePOSTData()`** (verbatim from the source): `//TODO : This
calls inserts garbage in data..!`, two `//TODO : use memmem` markers on `strstr()` calls scanning
not-fully-NUL-safe buffer regions, and `//TODO : output->boundary value is wrong..` / `"...contains fewer
chars"`. There's also an explicit fallback path whose own error message admits the problem: `"Could not detect
boundary in file payload, using unsafe length value..!"` — a malformed/truncated multipart body can produce a
bogus large `valueSize` here.

## 2. Security-relevant gaps

**No functioning ban/blacklist mechanism.** `security/banlist.c`'s `executeBanlist()` is a stub, and the
structure that would actually back it — `cache/client_list.c`, whose own header comment says *"Client Lists are
a stub and not implemented yet"* — is also unimplemented. `scripts/enforceBanlist.sh` (which scans access/error
logs for bad IPs) exists and presumably feeds *something*, but there's no code path that consults a ban list at
request time beyond the always-empty `clientList_isClientBanned()`/`clientList_isClientAllowedToUseResource()`
stubs.

**Command-execution attack surface across several Services.** `AmmServer_ExecuteCommandLine*()` in
`AmmServerlib/main.c` runs `popen()`/shells out directly on a caller-supplied string, and its own doc comment
says *"Executing commands can be dangerous , always check and sanitize input before executing."* The two
sanitizer functions meant to help with this are themselves stubs that do nothing but log a warning:
```c
int filterStringForShellInjection(char * buffer , unsigned int bufferSize)
{
  AmmServer_Warning("filterStringForSystemInjection not implemented ( %s , %u ) ",buffer,bufferSize);
  return 0;
}
```
Several Services actually use this pattern in production-shaped code, not just as a demo: AmmarServer's
`/execute.html`, MyRemoteDesktop's `/cmd` endpoint, MyTube's `youtube-dl` invocation, ImageGeneration's
image-generation script calls. None of these are safe to expose to untrusted clients as-is; any deployment that
does should audit exactly what input reaches the shell command and add its own sanitization — the library
offers none.

**`AmmServer_StringHasSafePath()` is an explicit, self-admitted stub**: its own body says
`AmmServer_Stub("TODO : AmmServer_StringHasSafePath better checking ( also use directory ).. https://www.owasp.org/index.php/Path_Traversal\n");`
and it doesn't even use its `directory` parameter — it just rejects a handful of "dangerous" characters. The
*actual* path-safety enforcement used by the live request path (`FilenameStripperOk`,
`StripHTMLCharacters_Inplace`, `ReducePathSlashes_Inplace` in `tools/http_tools.c`) is a separate, more complete
mechanism — but it's custom string validation, not canonicalization against a `realpath()`-resolved root, so a
security audit of that logic specifically (not covered in depth this session) would be worthwhile before
treating it as bulletproof.

**`monitor.html` has no visible access control** and exposes live internal server state (active thread count,
open files, cache memory usage, upload/download counters, and a per-thread listing) to anyone who can reach the
resource. It also renders a "STOP" link per thread that is **entirely decorative** — the handler reads the
`?stop=N` parameter and only logs it (`fprintf(stderr,"stop %u requested\n",...)`); no thread is actually
stopped. Not a security hole by itself (the button does nothing), but the page itself is an information-
disclosure surface if exposed publicly, and its own broken control could confuse an operator into thinking
they've stopped something they haven't.

**`AmmServer_AddRequestHandler()` self-flags as risky**: `AmmServer_Warning("AmmServer_AddRequestHandler could
potentially be buggy\n");` is printed unconditionally every time it's called (visible in every server's startup
log). Worth investigating what specifically is suspected buggy and either fixing it or replacing the warning
with a real explanation/doc comment.

**`/stop.html` (disabled by default) has a real use-after-free if ever enabled.** Found this session: enabling
`ENABLE_STOP_PAGE` and calling it triggers `AmmServer_Stop()` from *inside* a request-handling thread, which
frees the `AmmServer_Instance` — but `main()`'s `while (AmmServer_Running(default_server))` loop keeps running
and dereferencing the now-freed instance. Left disabled and documented ("you don't want this in production") in
the current code; a real fix would need to defer the actual teardown out of the request-handling thread.

**TLS configuration has no hardening beyond disabling SSLv2/SSLv3.** `network/openssl_server.c`'s
`ASRV_SSL_InitContext()` doesn't set an explicit cipher list, doesn't enforce a TLS 1.2+ floor, has no SNI or
multi-certificate support, and doesn't support mutual TLS (client certificate verification). Fine for a
single-domain personal deployment; worth revisiting before anything more exposed.

## 3. Spec-compliance / behavioral surprises

**Keep-alive requires an explicit client header, contrary to the HTTP/1.1 default.** Found and measured this
session: `output->keepalive` in `header_analysis/http_header_analysis.c` starts at 0 and is only set to 1 if the
request literally contains `Connection: keep-alive`. Per HTTP/1.1, keep-alive should be the *default* unless the
client sends `Connection: close`. In practice this means any client that doesn't explicitly ask for keep-alive
(some benchmarking tools, and potentially some real HTTP libraries/proxies that rely on the spec default) gets a
fresh TCP connection — and a full handshake — per request. Measured impact: ~33K req/s vs ~145K req/s for the
same request on the same build, purely from adding/removing that one header. This is likely costing real-world
performance in deployments where clients don't happen to send it explicitly. **Recommended fix**: default
`keepalive=1` for HTTP/1.1 requests unless `Connection: close` is present; keep the current opt-in behavior only
for HTTP/1.0.

**`If-Modified-Since` is parsed but explicitly not implemented**: `AnalyzeHTTPLineRequest()`'s
`HTTPHEADER_IF_MODIFIED_SINCE` case just does `fprintf(stderr,"304 Not Modified headers through dates not
supported yet\n"); return 0;`. `If-None-Match` (ETag-based) *is* implemented and used; date-based conditional
requests are not.

**No `Host:` header requirement, no absolute-URL support, no chunked transfer-encoding, no `100 Continue`.**
These are all listed as TODOs printed at every server startup (`src/Services/AmmarServer/main.c`): *"require the
Host: header from HTTP 1.1 clients"*, *"accept absolute URL's in a request"*, *"accept requests with chunked
data"*, *"use the 100 Continue response appropriately"*, plus *"Implement download resume capabilities (range
head request)"* even though `Range:` header parsing and a 206 response path do partially exist in
`network/file_server.c` — worth reconciling what's actually implemented vs. what the TODO still claims is missing.

## 4. Architecture findings from this session's own work

**Single shared epoll thread is a scalability ceiling for the fast path.** `epollFastPathServer.c` and the
accept layer both run on one thread (`instance->accept_epoll_fd` / `EpollAcceptLayerThread`). This was
sufficient for everything benchmarked this session (no core showed CPU saturation even at c=200), but if a
future workload pushes enough fast-path-eligible traffic to genuinely saturate that one thread's CPU, the fix
is sharding to N epoll threads (round-robin or hashed connection assignment) — not implemented, flagged as a
scoped follow-up in the code's own comments.

**No idle-connection timeout on epoll-managed connections.** A worker thread's blocking `recv()` has
`SO_RCVTIMEO` (`setSocketTimeouts()`), so a silently-abandoned keep-alive connection eventually gets dropped.
Once a connection is sitting in the accept-epoll set (waiting for its first byte, or — after the fast path
serves one request — waiting to be handed to a worker for its next one), there's no equivalent timeout: a
client that opens a connection and never sends anything holds a file descriptor indefinitely. Bounded by the
process's fd limit, not catastrophic, but a real gap versus a production server. Fix would be a timestamped
sweep, naturally extending the epoll thread's existing 1-second `stop_server`-recheck wakeup.

**The fast path is HTTP-only and intentionally narrow in scope**, by design (see `ammarserver.md` §3.3 and the
top-of-file comment in `epollFastPathServer.c`): HTTPS connections, any request with a query string, Range,
ETag/If-None-Match, Accept-Encoding, or Authorization header, anything over 256KB, and — critically — any
request past a connection's first (or immediately-pipelined) one, all fall back to the pre-existing
thread-per-connection path. An earlier, broader version that tried to recycle *every* keep-alive connection
through the shared epoll thread between requests was implemented, benchmarked, and found to be **~3x slower**
for realistic non-pipelined traffic — see the design-rationale comment block at the top of
`epollFastPathServer.c` for the full reasoning (waking a different thread through epoll costs more scheduling
latency than a thread waking itself via its own blocking `recv()`). This is a real, measured architectural
finding: naively porting nginx's single-event-loop model onto a thread-per-connection server without also
moving *all* request processing into that loop (which AmmarServer's synchronous callback model doesn't
currently support) makes things worse, not better, for busy connections. Closing this gap for real would need a
genuinely different design — e.g., one event-loop thread per core, each owning a subset of connections — which
is out of scope for what was built this session.

**HTTPS keep-alive connections still cost a thread for their whole lifetime.** Relocating an OpenSSL session's
handshake state across worker-thread reassignment safely was judged out of scope; the epoll accept layer and
fast path both gate on `!is_ssl_connection`. If HTTPS traffic volume grows, this is the next thing worth
tackling — likely requires storing `SSL*` alongside the recycled-connection metadata and restoring it into
whichever worker thread picks the connection back up.

## 5. Unimplemented feature stubs (lower priority — pre-existing, documented in code)

These were already flagged by comments in the code itself (`AmmServer_Stub(...)` calls, `@bug` doc comments) —
listed here for completeness since a "what works vs. what's aspirational" inventory is useful:

- **Sessions** (`_SESSION()`) — `cache/session_list.c` is a complete stub; `instance->sessionList` is always NULL.
- **📋 DEFERRED — Scheduler** (`AmmServer_AddScheduler()`) — both the public API and internal
  `scheduler/scheduler.c` are stubs; nothing runs periodically. Also has a units mismatch waiting to bite
  whoever implements it: the public API doc says "seconds," the internal function names its parameter
  `delayMilliseconds`. **Discussed 2026-08-29**: this needs a real implementation (dedicated timer thread,
  callback registration table, one-shot/repeating support, wiring into instance startup/shutdown) rather than a
  small bug fix — explicitly deferred to a future session rather than built now. When picked up: a dedicated
  thread woken by `pthread_cond_timedwait` against a min-heap or sorted list of next-fire-times (matching the
  style already used for `prespawnedThreads.c`'s cond-wait pattern) is the natural fit; resolve the
  seconds-vs-milliseconds mismatch as part of that work, not before (the "right" unit depends on the final
  API's granularity).
- **IP geolocation** (`tools/geolocation.c`) — `getIPCountry()` is a stub; intended data source (ipdeny.com CIDR
  zone files) is documented in a comment but never implemented against.
- **Compression** — the zlib-based implementation in `cache/file_compression.c` looks complete and careful
  (memory-budget accounting, realloc-to-shrink, proper zlib error-code handling), but is disabled by default
  and the codebase's own config comment says *"Compression doesn't work all that well yet."* Worth either
  fixing whatever's unreliable about it and turning it on, or removing it if it's not going to be maintained.
- **`templates/icons.c`** — entire file body is commented out; likely superseded by on-disk icon files rather
  than a real gap (directory listings reference `GetExtensionImage()` instead), but worth confirming and then
  either deleting the dead file or restoring it.
- **`AmmServer_SelfCheck()`** — literally `fprintf(stderr,"No Checks Implemented...")`.
- **`AmmServer_SignalCountAsBadClientBehaviour()`** — stub, no QoS/abuse-tracking behavior implemented.

## 6. Build system and repo tooling

- **`update_from_git.sh` references a stale path.** It has special-case logic to also update a nested repo at
  `src/AmmServerlib/InputParser/`, but `InputParser` now lives at the top level (`src/InputParser/`) — this
  logic no longer matches the current repo layout and likely silently does nothing useful.
- **`cmake_minimum_required(VERSION 2.8.7)`** at the top level (and in several sub-CMakeLists, per this
  session's own build output showing deprecation warnings from ~10 subprojects) is very dated. Bumping the
  floor version would silence a wall of CMake deprecation warnings on every configure.
- **`DO_TEST_AMMMESSAGES` CMake option is declared but never referenced** by any `if()` — dead configuration
  surface.
- **`AmmMessages` (the code generator) has zero consumers.** It's gated off by default (`USE_AMMMESSAGES OFF`)
  and grepping every `Services/*/CMakeLists.txt` and generated-code call site turns up nothing that uses it or
  its output. Either something depends on it that wasn't found, or it's unused tooling worth documenting as
  such (or removing).
- **`src/InputParser/README` is empty** — the only documentation for that library is its header's doc comments.
- **Stray untracked test artifacts** sitting in tracked source directories: `src/AmmClient/` has
  `downloaded.jpg`, `test.jpg`, `curltest`, `curltest.c.orig`, `TEST.txt` left over from manual testing;
  `src/AmmCaptcha/` has similar build/test residue (`captcha.jpg`, `captcha.ppm`, `AmmCaptchaTesterBin`, `obj/`).
  None of this is committed, but it clutters `git status` for anyone working in the repo — worth a `.gitignore`
  pass or a cleanup commit.
- **Hashmap's own header documents a known performance limitation**: *"uses serial searches for now, needs a
  lot of work"* — it does have an opt-in sort-then-search mode (`useSorting`), but it isn't the default, and
  it backs the resource cache lookup on every request that doesn't hit the epoll fast path (`cache_ResourceExists`
  / `cache_GetResource` → the shared hashmap). Worth profiling whether this matters at realistic cache sizes.

## 7. Minor / cosmetic

- `templates/icons.c` (see §5) and `src/Services/Apolls`, `src/Services/Deprecated/SQLiteServer`,
  `src/Services/Various/libkindrvserver` (orphaned from the CMake build entirely) are dead code kept in the tree
  without being compiled — fine to leave for reference, but worth a comment or a move to a clearly-marked
  `attic/` if they're never coming back, so it's obvious at a glance which services are live.
- `network/openssl_server.c`'s `ASRV_SSL_BindHTTPSSocket()` logs unconditionally on essentially every branch
  (not gated behind `DEBUG_MESSAGES` like the rest of the SSL code path) — looks like it was left in an
  actively-being-debugged state.
- Several `#warning` and inline `TODO` comments scattered through `header_analysis/` (see §1) are old enough
  that they're worth triaging: fix, or convert to a tracked issue, rather than leaving them as silent
  compiler-only noise that nobody reads unless they happen to build with warnings visible.
