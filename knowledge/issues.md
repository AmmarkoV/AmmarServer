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

**✅ FIXED** — **`post_data.c` had four flagged spots in `finalizePOSTData()`**: `//TODO : This calls inserts
garbage in data..!`, two `//TODO : use memmem` markers on `strstr()` calls scanning not-fully-NUL-safe buffer
regions, `//TODO : output->boundary value is wrong..` / `"...contains fewer chars"`, and an explicit fallback
path whose own error message admitted the problem: `"Could not detect boundary in file payload, using unsafe
length value..!"`.

**Audit result — two were real bugs, two were stale comments already resolved elsewhere in the same function:**
- **Real (fixed): the "inserts garbage in data" TODO.** `countStringUntilQuotesOrNewLine()` (which NUL-terminates
  wherever it stops scanning) was called with `configurationLength` — the length of the *whole* configuration
  block measured from its start — as the bound, even though the scan actually starts partway into that block
  (at `filename`/`name`, not at `configuration`). For a value with no closing quote, this let the scan run past
  the field's true end and into the payload/file data that follows, writing a stray NUL into someone else's
  file content and reporting a size that included part of it. Fixed at all 3 call sites: bound each scan by
  what's actually left of the configuration block *from that field's own position*
  (`configurationLength - (fieldPtr - configuration)`), not the whole block's length.
- **Real (fixed): the two "use memmem" TODOs.** When `reachNextBlock()` can't find the `\r\n\r\n` that ends a
  part's own header block within bounds (malformed/truncated multipart body), it returns unchanged —
  `configuration` isn't reliably NUL-terminated in that case, so the old `strstr()` calls could scan past this
  part's true boundary into unrelated later buffer data. Switched all 3 `strstr()` calls (`filename=\"`,
  `name=\"` ×2) to `memmem()` bounded by `configurationLength` (which is correctly 0 in the not-found case, so
  the search safely finds nothing instead of scanning unbounded).
- **Real (fixed): the "unsafe length value" fallback.** On the boundary-not-found path, it computed the
  fallback size from `output->boundary` — a pointer back in this part's own *header*, nowhere near the file
  payload — instead of from `payload` itself. Now reuses `payloadSize` (already computed just above), the
  actually-correct "remaining buffer from the payload's real start" value.
- **Stale (removed, not "fixed" — nothing to fix):** `//TODO : output->boundary value is wrong..` and
  `//TODO : This is not perfect..! output->boundary contains fewer chars`, sitting directly above code that
  already correctly implements a bounded `memmem()`-based boundary search with proper `--`/CRLF stripping —
  these described a problem some earlier pass had already solved without deleting the leftover comments.

**Verification**: rebuilt clean. Live end-to-end test against MyLoader's real `/upload.html` — a normal `curl -F`
multipart file upload was round-tripped (filename parsed correctly, uploaded content fetched back and diffed
byte-for-byte identical to the source file, only the deliberately-stripped trailing CRLF framing differing).
Then a deliberately malformed multipart body (bare `\n` line endings throughout, so the `\r\n\r\n` boundary
`reachNextBlock()` needs never appears — exactly the `configuration==payload`/`configurationLength==0` case the
fix targets) was sent 5× in a row directly against the running server via a raw socket: no crash, no hang, and
normal uploads continued to work correctly on the same server instance afterward.

**✅ FIXED** — **`finalizePOSTData()`'s `success` local variable never meant anything**: initialized to 0, never
incremented anywhere in the loop, so `return (success!=PNum)` was effectively always `1` whenever there was at
least one POST item, regardless of whether any individual item actually parsed correctly.

**Scan confirmed exactly one caller**: `network/recvHTTPHeader.c:194`, which already consumes the return value
meaningfully (`if (!finalizePOSTData(output)) { AmmServer_Error("Server failed to parse POST data"); }`) — it
was just being fed a signal that didn't reflect reality (false "success" whenever ≥1 item existed regardless of
how it parsed ; false "failure" whenever a POST had 0 multipart parts, e.g. an empty body, which isn't really a
parse failure at all).

**Fix applied**: added a per-item `itemOk` flag, cleared on every "this part is malformed" path already present
in the function — a file part with no `name=` field (previously silently allowed through with no warning at
all, unlike the symmetric non-file-part case which already warned; added the missing warning for consistency),
a text part with no `name=` field (already warned, now also clears the flag), and the "could not detect
boundary, using remaining buffer length as a fallback" case. `success` now only counts items where `itemOk`
stayed set, and the function returns `success==PNum` — true only when every item parsed cleanly, still true for
the `PNum==0` (nothing to parse) case since that isn't a failure.

**Verified live** against MyLoader's real `/upload.html`: a normal upload no longer triggers the (previously
silent, now-eliminated) *false* `Server failed to parse POST data` log line ; the same malformed multipart body
used to verify the earlier post_data.c fixes now correctly triggers exactly one real error log entry, and the
server stays fully functional (a subsequent normal upload on the same running instance still works) afterward.

## 2. Security-relevant gaps

**No functioning ban/blacklist mechanism.** `security/banlist.c`'s `executeBanlist()` is a stub, and the
structure that would actually back it — `cache/client_list.c`, whose own header comment says *"Client Lists are
a stub and not implemented yet"* — is also unimplemented. `scripts/enforceBanlist.sh` (which scans access/error
logs for bad IPs) exists and presumably feeds *something*, but there's no code path that consults a ban list at
request time beyond the always-empty `clientList_isClientBanned()`/`clientList_isClientAllowedToUseResource()`
stubs.

**✅ FIXED (sanitizers) + AUDITED (call sites)** — **Command-execution attack surface across several Services.**
`AmmServer_ExecuteCommandLine*()` in `AmmServerlib/main.c` runs `popen()`/shells out directly on a
caller-supplied string, and its own doc comment says *"Executing commands can be dangerous , always check and
sanitize input before executing."* The two sanitizer functions meant to help with this were themselves stubs
that did nothing but log a warning and return 0.

**Sanitizers implemented** (`AmmServerlib/main.c`):
- `filterStringForShellInjection(buffer, bufferSize)` — wraps the string in single quotes, escaping any
  embedded `'` as `'\''` (close quote, escaped literal quote, reopen quote) — the standard, complete way to
  make an arbitrary byte string safe as one POSIX shell argument. In-place via a scratch buffer (escaping only
  ever grows the string); refuses to truncate and returns 0 if the result wouldn't fit the caller's buffer.
- `filterStringForHtmlInjection(buffer, bufferSize)` — reuses the already-correctly-implemented
  `AmmServer_HTMLEscape()` rather than re-implementing entity escaping a second time; same
  scratch-buffer/refuse-to-truncate approach.
- **Verified**: a standalone test round-tripped a real shell-injection payload (`foo'; rm -rf /tmp/pwned; echo
  '`) through `filterStringForShellInjection()` and then through an *actual* `system()` call — the shell
  printed back the exact original string byte-for-byte with no command executed. HTML escaping verified against
  a classic `<script>alert('xss')</script>` payload. Buffer-too-small cases verified to fail cleanly (return 0,
  buffer left untouched) rather than truncate or overflow.

**Full audit of every `system()`/`popen()` call site in the repo** (a fork traced each one's actual data
provenance; a repo-wide `execl`/`execv`/`execve` grep found zero additional call sites of that family):

| Call site | Runs | Client-controlled input reaches it? | Verdict | Action |
|---|---|---|---|---|
| `MyTube/thumbnailer.c` (via `videoFilename`, ultimately a YouTube video's *title*) | `ffmpeg` | **Yes** — a YouTube video's title is attacker-influenceable and was interpolated unescaped into a double-quoted ffmpeg argument. `DO_YOUTUBE_DOWNLOADING` **defaults to 1 (on)** — reachable in a default build, not gated as originally assumed. | Was **HIGH** | **✅ Fixed** — added `filterVideoTitleToSafeText()`, a whitelist filter (alnum + space + `.,()-_`) applied to the title immediately after fetching it, before it's used to build any filename or reach any downstream command |
| `Various/ScriptRunner/main.cpp`, `say` branch | `rostopic pub ...` (ROS message with an embedded shell string) | **Yes** — `?say=` GET parameter, embedded unescaped as an inner YAML-style single-quoted value *inside* an already double-quoted shell argument (a two-layer quoting problem — single-quote-wrapping the whole value like the shell-injection sanitizer does would nest incorrectly here). Built unconditionally, no auth on the endpoint. | Was **HIGH** | **✅ Fixed** — added `filterSpokenTextForSafety()`, a targeted blacklist of the specific dangerous bytes (`"`,`'`,`` ` ``,`$`,`\`,`;`,control chars) that preserves UTF-8 (spoken text can legitimately be non-ASCII — the code's own hardcoded test string is Greek) |
| `MyRemoteDesktop/main.c`, `/cmd` keystroke path | `xdotool key '...'` | Thin — a single character (`dokey` cast to `char`), can't build a full multi-token payload alone. Double-gated: `USE_XSERVER` CMake option **off by default**, and a runtime `allowControl` flag defaulting to 0 (only enabled via `--control` CLI flag) | **MEDIUM** (gated) | **✅ Fixed anyway** — reject shell-special bytes (`'`,`"`,`` ` ``,`$`,`\`,`;`,control chars) outright before use, cheap defense-in-depth even though the realistic exploit surface was already thin |
| `ImageGeneration/main.c`, upload + `/go` query handlers | `img2imgOnlyOnce.sh`/`tex2imgOnlyOnce.sh` | Yes, but already passed through `filterQuery()` (whitelist to `[a-zA-Z0-9.,()]`) before use — this is the same pattern the two new filters above now also follow | **LOW** (already mitigated) | No change needed |
| `Deprecated/CinemaPilot/main.c` (5 sites) | `xdotool`, `FullScreenViewer`, `mkfifo`, `killall mplayer`, `mplayer` | No — all either fully hardcoded or built from a server-side playlist file, no HTTP path feeds them | **LOW** | No change needed |
| `AmmServerlib/tools/logs.c` (`compressLog`) | `mv`/`gzip`/`rm` on a log file | No — only ever called with the server's own configured `AccessLog`/`ErrorLog` paths | **LOW** | No change needed |
| `AmmServerlib/tools/http_tools.c` (`ServerThreads_DropRootUID`) | `id -u <username>` | No — username is a server-config constant (default `www-data`), not client input | **LOW** | No change needed |
| `AmmServerlib/main.c`, `AmmServer_ExecuteCommandLine*()` (the generic library primitives, 3 `popen()` call sites) | Whatever the caller passes | Contract-only — takes a pre-built command string, applies no sanitization itself (by design, it's a raw primitive) | N/A (not itself a vuln) | AmmarServer's own `/execute.html` confirmed to only run `executeScript`, set solely from the server's own `-e` startup CLI argument — operator-controlled, not client-controlled |

**Not fixed, still open** (unrelated to this pass, tracked separately below): `AmmServer_StringHasSafePath()` —
already flagged as a self-admitted stub in its own source, see below.

**✅ FIXED** — **`AmmServer_StringHasSafePath()` was an explicit, self-admitted stub**: its own body said
`AmmServer_Stub("TODO : AmmServer_StringHasSafePath better checking ( also use directory ).. https://www.owasp.org/index.php/Path_Traversal\n");`
and it didn't use its `directory` parameter at all — just rejected a handful of "dangerous" characters
(control bytes, `\`, `%`, `/`).

**Usage scan found 3 real call sites**, all security-critical: `MyLoader/main.c` gates both a file **read** by a
client-supplied name (`?i=` on `/vfile.html`) and — the more serious one — a file **write**, using the
client-supplied *upload filename* as part of the path the uploaded bytes get written to
(`AmmServer_WriteFileFromMemory`); `ImageGeneration/main.c` gates the identical upload-filename-to-write-path
pattern. If this check were ever bypassable, the write-path cases become an arbitrary-file-write primitive
(upload a file named to escape the uploads directory, write anywhere the process has permissions — a realistic
path to remote code execution, e.g. dropping a web-shell where it'd be served).

**Audit of the existing blacklist**: since it already rejected every `/` outright, the classic multi-component
`../../etc/passwd`-style payload could never reach it in the first place. But a filename that is *literally*
`..` (two dots, **no slash**) sailed through untouched — neither `.` alone is "dangerous" to a
character-by-character check — and `directory + "/" + ".."` resolves to the parent of `directory`, handing back
whatever's stored one level up. And because `directory` was never used, there was no way for the check to catch
a symlink placed inside the trusted directory pointing somewhere else — a bypass a character blacklist can
never detect in principle, no matter how thorough, since the filename itself contains nothing "dangerous."

**Fix applied**: kept the entire original character blacklist untouched (so nothing that used to pass or fail
changes behavior), and added a `realpath()`-based canonical containment check *on top* of it, finally using
`directory` as the OWASP-recommended fix suggests — resolve both `directory` and the full candidate path to
their real, canonical, symlink-free absolute form, and require the resolved candidate to be `directory` itself
or a path underneath it. This is purely additive on top of the existing blacklist (can only reject *more* than
before, never accept something the blacklist already rejected). Handles the case where the target doesn't exist
yet (an upload's filename, about to be created) by falling back to resolving just the containing directory and
manually appending the final path component, rejecting outright if that component is `.`, `..`, or empty.

**Verified against a real filesystem**, not just traced: existing legitimate filenames still pass ; brand-new
(not-yet-existing) filenames still pass, proving the upload-scenario fallback works ; multi-component `/`
traversal still rejected (regression check, unchanged blacklist) ; a bare `..` is now correctly rejected (the
actual gap this fix closes) ; **a symlink planted inside the trusted directory pointing to a file outside it is
correctly rejected** (the exact class of bypass no character blacklist could ever have caught). `AmmarServer`
rebuilds clean and the full project (every Service) rebuilds with zero errors.

**✅ FOLLOW-UP DONE** — the item above ("not fixed, still open") was resolved in a follow-up pass: rather than
leaving `FilenameStripperOk()` (the live request path's own, older, string-only validator) as a separate,
unaudited mechanism, its one real remaining gap — no canonicalization, so it can't catch a symlink inside
`webserver_root` pointing elsewhere, the exact class of bug the `AmmServer_StringHasSafePath()` fix above closed
— was closed too, **by extracting and sharing the same canonicalization logic** instead of writing a second,
parallel implementation of it (reducing code attack surface : one audited implementation of "is this path
really inside this directory" instead of two that could drift out of sync or carry different bugs).

**Key design constraint**: `FilenameStripperOk()` itself runs on the ultra-hot per-request path — every single
request, including inside `epollFastPathServer.c`'s fast path, which this same session built specifically to
avoid syscalls per request. Adding `realpath()` calls (real `stat()`/`readlink()` filesystem syscalls) there
would have been a direct, measurable regression against that work. So `FilenameStripperOk()` itself was **left
completely untouched** — instead, the shared check was placed at the points a file actually gets opened off
disk, which for a cache-hit ( the common case for every request after the first ) never happens at all:

- **`tools/http_tools.c`** — extracted `AmmServer_StringHasSafePath()`'s realpath-based core into a new shared
  function, `PathResolvesWithinDirectory(trustedRoot, candidatePath)` (declared in `http_tools.h`, alongside
  `FilenameStripperOk`). `AmmServer_StringHasSafePath()` (`main.c`) now just builds its joined candidate string
  and calls this — no more duplicated canonicalization logic between the two.
- **`cache/file_caching.c`** — `cache_GetResource()`'s cache-population step (the one place a newly-requested
  file gets read off disk for the first time — every subsequent request for it is served from the in-memory
  cache instead, so this runs **once per unique file, not once per request**) now calls
  `PathResolvesWithinDirectory(instance->webserver_root,...)` before caching it. This is the single check that
  covers *both* callers of `cache_GetResource` — the traditional `SendFile()` path and the epoll fast path.
- **`network/file_server.c`** — two more guards were needed, found only by testing this live end-to-end (see
  below), because `SendFile()` has a *second*, separate code path for files that don't get cached (too large,
  `doNOTCacheRule`, etc.) that bypasses the caching layer entirely: `TransmitFileToSocket()` (the direct
  disk-streaming fallback) got the same check right before its `fopen()`, and `SendFile()` itself got it added
  at its existing top-of-function validation point (alongside the pre-existing `FilenameStripperOk` check) so a
  rejected request gets a clean `400` *before* any header is sent, rather than a `200` header going out followed
  by a silently-truncated connection ( a real, if minor, HTTP-framing bug caught only by testing the fix
  end-to-end rather than just tracing the code — the first version of this fix protected the caching layer
  correctly but missed this second disk-read path entirely, and a symlink request was still served in full ).

**Verified live**, escalating through the exact bug that was found and fixed: set up a real webroot containing
a legitimate file and a symlink pointing to a file genuinely outside it. First pass: cache layer correctly
refused to cache the symlink target, but the "serve from disk directly" fallback still leaked the secret
file's full contents (confirmed via `curl` — this is the real gap `TransmitFileToSocket` closes). After adding
that guard: the secret content was no longer served, but as a `200 OK` header with a silently-dropped
connection (malformed HTTP framing, confirmed via `curl -v`) — not a data leak, but not clean either. After
adding the early `SendFile()` check: a proper `400 Bad Request` is returned before any header goes out,
confirmed consistent across repeated requests, with normal (non-symlink) file serving and caching unaffected
throughout every iteration of this fix. Full project rebuilds with zero errors at each step.

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

**✅ FIXED** — **Keep-alive used to require an explicit client header, contrary to the HTTP/1.1 default.** Found
and measured earlier this session: `output->keepalive` in `header_analysis/http_header_analysis.c` started at 0
and was only set to 1 if the request literally contained `Connection: keep-alive`. Per HTTP/1.1, keep-alive
should be the *default* unless the client sends `Connection: close`. In practice this meant any client that
didn't explicitly ask for keep-alive (some benchmarking tools, and potentially some real HTTP libraries/proxies
that rely on the spec default) got a fresh TCP connection — and a full handshake — per request. Measured impact:
~33K req/s vs ~145K req/s for the same request on the same build, purely from adding/removing that one header.

**Fix applied** in two places that needed to stay consistent with each other:
- `ProcessFirstHTTPLine()` (`http_header_analysis.c`): now reads the HTTP version token off the request line
  (`HTTP/1.0` vs anything else, i.e. `HTTP/1.1`) and seeds `output->keepalive` accordingly — `0` (opt-in,
  unchanged legacy behavior) for HTTP/1.0, `1` (new default) otherwise — *before* any header lines are parsed.
- The `Connection:` header case in `AnalyzeHTTPLineRequest()` already set `keepalive=1` on `KEEP-ALIVE`; added
  the missing `CLOSE` check so an explicit `Connection: close` can now override the new HTTP/1.1 default back
  down to 0 (previously there was no code path that could ever set `keepalive` back to 0 once headers started
  being parsed).
- `threads/epollFastPathServer.c`'s fast path does its **own independent** keepalive detection (raw `MSG_PEEK`
  byte-matching, not a call into the real parser — see `ammarserver.md` §3.3) and had the identical
  explicit-header-only behavior. Left unfixed, it would now *disagree* with the real parser on identical
  requests — updated it to the same HTTP-version-based default/override logic, as a byte-peeking approximation
  (good enough given a rare miscategorization here just means a connection closes/stays open slightly
  differently than the real parser would, not a correctness issue for the response itself).

**Verified live** with a 4-scenario test matrix over raw sockets (checking actual second-request-on-the-
same-connection success, not just the response header AmmarServer claims) : HTTP/1.1 with no explicit header now
correctly stays alive (previously closed) ; HTTP/1.1 with explicit `Connection: close` still closes ; HTTP/1.0
with no header still closes (unchanged) ; HTTP/1.0 with explicit `Connection: keep-alive` still stays alive
(unchanged opt-in). Re-ran the original benchmark that surfaced this: `wrk` *without* the explicit keep-alive
header now gets **147,019 req/s**, up from ~33K and now matching (fractionally exceeding, within noise) the
145K achieved *with* the header — the gap this issue described is closed. Full project (every Service) rebuilds
with zero errors.

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

- **✅ IMPLEMENTED** — **Sessions** (`_SESSION()`) — was a complete stub (`instance->sessionList` always `NULL`); now a
  real, PHP-`$_SESSION`-style cookie-based session store tied into user-account login. Full design plan at
  `~/.claude/plans/cheeky-fluttering-valley.md`. Summary:
  - **Store**: `cache/session_list.c` — one hashmap keyed by an unguessable session token string, each entry
    holding its own ( also hashmap-backed ) key/value data plus `created`/`lastSeen` timestamps. Reuses
    `src/Hashmap/` as-is for its existing internal locking, plus one extra per-session `pthread_mutex_t` so
    unrelated sessions never contend with each other. In-memory only ( not persisted across restarts, by
    design — matches the ephemeral, per-visitor nature of the data and avoids adding disk I/O to a request-hot
    path ). Lazy idle-timeout eviction (`SESSION_IDLE_TIMEOUT_SECONDS`, default 1800s) and a capacity cap
    (`MAX_SESSIONS`, default 10000, oldest-`lastSeen` evicted first) — both new externs in
    `server_configuration.h`/`.c`, no background thread needed ( the scheduler subsystem stays deferred, see
    above — not a dependency for this ).
  - **Hashmap gained two small, generic, reusable primitives it was missing**: `hashMap_GetPayloadAtIndex()`
    ( the correct, working equivalent of `hashMap_GetPayload()`, which was found to be broken while building
    this — see below ) and `hashMap_RemoveKey()` ( no per-key removal existed at all before ; needed for session
    eviction/logout, and for `_SESSIONset()` to not leak duplicate entries on repeated calls, since
    `hashMap_Add()` is append-only ). Verified standalone under ASan/UBSan ( add/remove/re-add, non-existent-key
    removal, payload round-trip ).
  - **🐛 Found in passing, not fixed** — `hashMap_GetPayload(struct hashMap*,const char*,void*)` is dead code
    with a real bug: `payload` is passed *by value*, so `payload = hm->entries[i].payload;` inside the function
    only reassigns the local copy — the caller never receives anything. Currently unreachable (grep confirms
    nothing in the repo calls it), so left as-is with a doc comment pointing callers at the new, correct
    `hashMap_GetPayloadAtIndex()` instead, rather than fixing a function nothing uses.
  - **Cookies**: reading already existed (`_COOKIE*`, `main.c`, earlier this session); **writing never did** —
    added `AmmServer_SetCookie()` as a general-purpose primitive (not session-specific), building a proper
    `Set-Cookie` line (`Path=/`, optional `Max-Age`, `HttpOnly`, `Secure` when the instance has TLS enabled,
    `SameSite=Lax`) into a new `pendingResponseHeaders` buffer on `AmmServer_DynamicRequest`. Plumbed out to the
    actual response via a new `HTTPTransaction::pendingResponseHeaders` field — `cache_GetResource()` and
    `dynamicRequest_serveContent()` both gained an out-parameter to copy this out of the per-callback `rqst`
    right before it's freed (they're all synchronous, same-thread, same call stack — traced end to end), and
    `SendSuccessCodeHeader()` (the single choke point already used for every 200/206 response, static and
    dynamic alike) appends it and clears it. The epoll fast path never touches this — by design it only ever
    serves anonymous, cache-hit, non-dynamic GET/HEAD requests, so it can never have a cookie to set.
  - **CSPRNG**: no cryptographically-random token source existed anywhere in the repo — every "random token"
    (`UserAccounts`, `HabChan/csrf.c`, etc.) called unseeded `rand()`. Added
    `AmmServer_GenerateSecureToken()` (`tools/http_tools.c`) — `getrandom()`, retrying on `EINTR`, falling back
    to `/dev/urandom` only if `getrandom()` itself is unavailable, base64url-encoded. Verified standalone: 1000
    generated tokens, zero duplicates, correct length, clean refusal (not truncation) on a too-small output buffer.
  - **Request lifecycle**: reused the *existing* `useSessionLifecycle` opt-in flag on `AmmServer_RH_Context`
    (already wired into `dynamic_requests.c`, just never functional) rather than adding a new one. On a
    session-enabled resource, an absent/unknown/expired cookie transparently gets a brand new session + cookie
    issued (PHP's `session_start()` auto-create behaviour); a valid one gets its `lastSeen` touched.
  - **Public API** (`main.c`/`AmmServerlib.h`), matching the existing `_GET*`/`_POST*`/`_COOKIE*` naming
    convention exactly: `_SESSION`, `_SESSIONcpy`, `_SESSIONuint`, `_SESSIONexists`, `_SESSIONset`,
    `_SESSIONunset`. `_SESSION()`'s doc comment was also stale (copy-pasted from `_FILES()`) — corrected.
  - **User-account integration** (`src/UserAccounts/`) — this is where the three real, pre-existing security
    gaps flagged by this session's research got fixed:
    - **Plaintext passwords → salted, iterated SHA-256.** Added a small, self-contained, from-scratch SHA-256
      (`src/UserAccounts/sha256.c`/`.h` — deliberately *not* gated behind the optional, off-by-default
      `USE_OPENSSL` flag, since account security must work in the default build) verified against 4 FIPS test
      vectors (empty string, `"abc"`, the 448-bit padding-boundary case, and a 1000-byte multi-block input) under
      ASan/UBSan, all passing. Stores `<16-byte salt hex>:<SHA-256^10000(salt‖password) hex>` instead of the raw
      password ; final comparison is constant-time. Deliberately modest (not bcrypt/Argon2-grade) — noted as the
      "upgrade later if this ever needs to be production-grade" line. **Tradeoff**: this changes the on-disk
      format, so the existing `db/users.db`/`data/db/users.db` test accounts (plaintext, confirmed while
      researching this) can no longer authenticate — need a one-time reset. Verified standalone under ASan:
      identical passwords for two different users produce two different stored hashes (different salt), correct/
      wrong/nonexistent credentials behave correctly, and the hash round-trips correctly through a save-to-disk +
      reload-from-disk cycle.
    - **Weak session-ID generation → CSPRNG.** `RegisteredUser.sessionID` (still used as-is by `Social`/
      `ShareTex` — see below) previously came from `uadb_getBackRandomFileDigitsInplace()`, seeded from
      unseeded `rand()`. Now hex-encoded CSPRNG bytes are used instead where available (self-contained
      `getrandom()`/`/dev/urandom` helper local to `userAccounts.c` — deliberately *not* reusing
      `AmmServer_GenerateSecureToken()`, to keep this library's existing zero-dependency-on-AmmServerlib design
      intact), falling back to the old generator only if the CSPRNG is genuinely unavailable (a session ID isn't
      worth failing account creation over, unlike the password salt, which does refuse account creation outright
      if the CSPRNG can't be reached).
    - **Session ID in the URL → real HttpOnly cookies.** New `AmmServer_Login()`/`AmmServer_Logout()`/
      `AmmServer_CurrentUsername()` (`userAccountsWeb.c`) tie the new AmmServerlib session system to
      `UserAccounts` authentication: `AmmServer_Login()` verifies credentials via the existing
      `uadb_authenticateUser()`, then **rotates to a brand new session token** (session-fixation defense — a
      pre-login anonymous session must never become the authenticated one — verified live, see below) and stores
      `username`/`uid` into the session's own data ; `AmmServer_Logout()` destroys the session server-side and
      clears the cookie (`Max-Age` in the past) ; `AmmServer_CurrentUsername()` is the one call a resource
      handler needs for "is this request logged in, and as whom."
    - **Deliberately left as-is, not rewired**: `Social` and `ShareTex` still use the old `?s=` URL-param idiom
      (`uadb_getUserTokenFromSessionID()`, `outputToken.sessionID`, `RegisteredUser.sessionID`) across ~7 call
      sites in `login.c`/`home.c`/`chat.c`/`auth.c`. The original plan for this work called for removing that
      legacy mechanism entirely ; doing so turned out to break both services' builds, so the legacy path was kept
      fully intact instead and the new API added purely additively — `Social`/`ShareTex` migrating to
      `AmmServer_Login`/`AmmServer_CurrentUsername` is a real, currently-unstarted follow-up, not done here.
  - **Verified live**, end to end, against a real running throwaway test server (not just traced): first request
    to a session-enabled resource gets a fresh `Set-Cookie` ; replaying that cookie reuses the same session
    (`_SESSIONset`/`_SESSION` round-trip via a hit counter, confirmed incrementing correctly across requests) ; a
    garbage/unknown cookie value is silently issued a brand new session, no crash ; after the idle timeout
    (temporarily set to 2s for the test) the old cookie stops resolving and a fresh session is issued ; a login
    with the wrong password correctly fails and doesn't authenticate ; a login with the correct password logs in
    **and** rotates the session cookie to a new value (fixation defense confirmed by diffing the cookie before/
    after) ; the pre-login cookie no longer resolves to anything after that ; `AmmServer_CurrentUsername()`
    correctly reflects logged-out → logged-in → logged-out (post-`AmmServer_Logout()`) across the same flow ; 200
    concurrent requests against one session (`xargs -P 20`) produced no crash and no data corruption (the one
    "lost increment" observed is an expected non-atomic read-then-write race in the *test callback itself*, the
    same class of race PHP's own `$_SESSION` has without an explicit lock — not a session-store bug) ; a
    capacity cap of 3 sessions with 5 created stayed stable, no crash. Full project (every Service, including
    `Social`/`ShareTex`) rebuilds with zero errors.
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
