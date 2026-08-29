# AmmarServer — How It Works

AmmarServer is a small, dependency-light HTTP/HTTPS server written in C, structured as a reusable library
(`libAmmarServer`) plus a collection of example "Services" (standalone executables) that link against it. Its
central design idea, stated directly in the source (`src/AmmServerlib/main.c`'s dynamic-content comment block),
is: instead of routing dynamic requests through an interpreter (PHP/Ruby/Python), a request handler is a plain
compiled C function linked directly into the server binary. There is no bytecode, no template engine, no
database driver bundled — the library gives you an HTTP request/response lifecycle and a memory buffer; you
fill the buffer.

This document describes the architecture as it exists after this session's work (epoll accept layer + epoll
fast path for static content, on top of the pre-existing thread-per-connection model). See `knowledge/issues.md`
for known problems and improvement opportunities found while writing this.

## 1. Repository layout

```
AmmarServer/
├── src/
│   ├── AmmServerlib/        The core HTTP server library (libAmmarServer.a / .so)
│   ├── Services/            ~20 standalone demo/example executables built on top of it
│   ├── Hashmap/             Generic hashmap used by the cache and client-list subsystems
│   ├── InputParser_C/       Generic key/value config-file and string parsing helper
│   ├── BasicImaging/        Standalone image (JPEG/PNG/PZP) thumbnailing + SIMD resize library
│   ├── AmmCaptcha/          Standalone CAPTCHA image generator library
│   ├── AmmClient/           A companion HTTP *client* library (curl-like), separate from the server
│   ├── AmmMessages/         Small pub/sub-style messaging helper
│   └── UserAccounts/        User account/auth helper library (if present in this checkout)
├── scripts/                 Install/deploy/benchmark/security-audit shell scripts (see §10)
├── pentest_suite/           Load-testing / stress-testing scripts against a running instance
├── public_html/             Default static-file webroot for the AmmarServer demo service
├── doc/                     Misc docs, banner image
├── 3dparty/                 NOT part of AmmarServer - reference sources (nginx) and the wrk benchmarking
│                             tool, added this session purely for comparison benchmarking, see §11
├── run_*.sh, start/stop*.sh Convenience launchers for each Service, at the repo root
└── CMakeLists.txt           Top-level build file; delegates into each src/ subdirectory
```

The library (`AmmServerlib`) is deliberately generic — it doesn't know about blogs, chat, or file uploads. Each
Service under `src/Services/` is a `main.c` that calls into the library's public API
(`AmmServerlib.h`) to register static/dynamic resources and starts an event... no — a *thread-per-connection*
server on a port. See §6 for the full Services inventory.

## 2. Core library structure (`src/AmmServerlib/`)

```
AmmServerlib/
├── AmmServerlib.h            The public API + all core structs (HTTPHeader, HTTPTransaction, AmmServer_Instance, ...)
├── server_configuration.h/.c Global tunables (#define's and a few `extern` globals), config-file loading
├── main.c                    Public API implementations: AmmServer_Start*, AddResourceHandler, _GET/_POST/_COOKIE accessors, string/file utilities
├── version.h                 FULLVERSION_STRING
│
├── threads/                  Connection acceptance and request-serving concurrency model (§3)
├── network/                  Socket I/O, SSL abstraction, header send/receive, file transmission (§3)
├── header_analysis/          Raw HTTP request parsing (request line, headers, GET/POST/COOKIE bodies) (§4)
├── cache/                    The central resource cache: static files, dynamic-content contexts, client list, sessions (§5)
├── tools/                    Grab-bag: logging, date formatting, misc file/string helpers, directory listing, geolocation, monitor page
├── templates/                Built-in HTML fragments: error pages (400/401/403/404/408/500/501), a file-manager-style editor, a login form
├── security/                 IP ban-list mechanism
├── scheduler/                Periodic (non-HTTP) callback timer mechanism
├── stringscanners/           Small generated lexers (HTTP method / header-name matching) - see §4
└── AString/                  A separate small string/file utility library the rest of the code calls into
```

## 3. Connection lifecycle — from `accept()` to response

This is the part of the codebase this session's work touched most, so it's covered in the most detail.

### 3.1 Startup

`AmmServer_Start[SSL|WithArgs]()` (`main.c`) allocates an `AmmServer_Instance`, loads an optional config file,
initializes the resource cache (`cache_Initialize`), and calls `StartThreadedHTTPServer()`
(`threads/threadedServer.c`), which spawns `MainThreadedHTTPServerThread` — this binds the listening socket(s),
optionally pre-spawns a worker thread pool (`PreSpawnThreads`, disabled by default —
`MAX_CLIENT_PRESPAWNED_THREADS_DEFAULT` is 0, needs `-prefork N`), starts the epoll accept-layer thread (§3.3),
and then loops on `select()` across the HTTP and (if enabled) HTTPS listening sockets, calling
`drain_and_dispatch()` whenever one becomes readable.

### 3.2 Accepting a connection

`drain_and_dispatch()` calls `accept4(..., SOCK_CLOEXEC)` in a loop until `EAGAIN` (the listening socket is
non-blocking), draining the whole accept backlog in one pass rather than one connection per `select()` wakeup.
Each accepted socket is handed off — see §3.3 — to either the epoll accept layer, or (if that's unavailable)
directly to `dispatch_accepted_client()`, which tries, in order:
1. `UsePreSpawnedThreadToServeNewClient()` (`threads/prespawnedThreads.c`) — round-robins across a fixed pool of
   already-running threads, each blocked on a `pthread_cond_timedwait` (a ~200ms deadline, so a stopped server
   is noticed promptly) waiting for a `busy` flag. A "busy" slot is only cleared *after* the connection is fully
   served (not before), specifically to avoid a second connection being silently stranded in a slot whose worker
   is still mid-service.
2. `SpawnThreadToServeNewClient()` (`threads/freshThreads.c`) — `pthread_create`s a brand-new detached thread if
   no prespawned slot took it (or none exist). Its per-thread context (`struct PassToHTTPThread`) is
   heap-allocated so there's no stack-lifetime hazard handing it across the `pthread_create` boundary.

Either way, the connection ends up in `ServeClientAfterUnpackingThreadMessage()` (`threads/clientServer.c`),
which builds a stack-local `struct HTTPTransaction`, does the TLS handshake if this is an HTTPS connection
(`network/openssl_server.c`), and calls `ServeClientInternal()`.

### 3.3 The epoll accept layer and the epoll fast path (added this session)

Two related, but distinct, mechanisms live in `threads/threadedServer.c` and the new
`threads/epollFastPathServer.c`, both built around one shared `epoll` instance owned by the server instance
(`instance->accept_epoll_fd`) and serviced by one thread (`EpollAcceptLayerThread`):

**Accept layer.** A freshly accepted connection is *not* immediately hand off to a worker thread. It's
registered with `epoll_ctl(EPOLLIN|EPOLLONESHOT)` and only dispatched once it actually has a byte to read. This
means an idle/slow client (a browser tab left open, a deliberately slow "slowloris"-style connection) costs one
file descriptor while it's idle, not a whole blocked worker thread. This was validated with a synthetic
idle-connection test: without it, N idle connections consume N threads; with it, they consume ~0.

**Fast path** (`epollFastPathServer.c`). When a registered connection becomes readable, before falling into the
normal worker-thread dispatch, `EpollFastPath_TryServe()` peeks (`MSG_PEEK`, non-consuming) at the request. If
it's a plain `GET`/`HEAD`, with no query string, `Range`, `If-None-Match`/`If-Modified-Since`,
`Accept-Encoding`, or `Authorization` header, resolving to a cached, non-dynamic, non-"do-not-cache",
≤256KB static file, it's served *directly from this thread* — header + body copied into one buffer and sent
with a single non-blocking `send()` — without ever creating or reusing a worker thread. Anything not eligible
falls back to the normal dispatch path having consumed *zero* bytes (the worker's own `recv()` sees exactly the
same bytes it would have if the fast path didn't exist).

**Important, deliberately-scoped design constraint:** the fast path serves a connection's first request, and
any of its own truly back-to-back pipelined requests still sitting in the socket buffer (bounded by
`FASTPATH_MAX_PIPELINED_REQUESTS` = 64, so one very chatty connection can't starve others). The moment that runs
dry, the connection is hand off to a *normal worker thread* (`DispatchContinuationToWorker`) for whatever comes
next — which then just loops in place with a plain blocking `recv()`, exactly like the pre-epoll code always
did. It deliberately does **not** try to recycle an already-active connection back through the shared epoll
thread between every request. An earlier version of this session's work did try that (mirroring nginx), and it
measured **~3x slower** for realistic (non-pipelined, "wait for response then send next request") traffic —
waking a *different* thread through epoll costs more scheduling latency than a thread waking itself via its own
blocking `recv()`, and funnelling many connections' continuations through one shared thread throws away the
parallelism N independent worker threads had. See `epollFastPathServer.c`'s top comment block and
`knowledge/issues.md` for the reasoning and its implications (idle time *between* a connection's 2nd+ request
still costs a thread — only true first-contact idle time, and first-request serving, got solved).

HTTPS connections are excluded from both the fast path and any thread hand-off tricks (`is_ssl_connection`
gates them out) — relocating an OpenSSL session's state across worker threads safely was judged out of scope.

### 3.4 Serving one request

`ServeClientInternal()` (`threads/clientServer.c`) resolves the peer IP once (`getSocketIPAddress`), checks the
client-ban list (`clientList_isClientBanned`), then loops `ServeClientKeepAliveLoop()` for as long as the client
wants (see §3.5 for why AmmarServer's keep-alive detection differs from the HTTP spec's default). Each iteration:

1. `receiveAndHandleHTTPHeaderSentByClient()` → `receiveAndParseIncomingHTTPRequest()` (`network/recvHTTPHeader.c`)
   loops on `ASRV_Recv()` (a thin wrapper choosing plain `recv()` or `SSL_read()`,
   `network/networkAbstraction.c`) until a full header is buffered, parsing it line-by-line in place via
   `keepAnalyzingHTTPHeader()`/`AnalyzeHTTPLineRequest()` (§4) as bytes arrive.
2. `decideAboutHowToHandleRequestedResource()` figures out whether the resource is a cached item, an
   uncached-but-existing file, a directory (→ index file or generated listing), or a "template" (internal
   resource re-route, e.g. directory-listing icons).
3. Dispatches to `SendEmbeddedFile`, `respondToClientBySendingAGeneratedDirectoryList`, or — the common case —
   `SendFile()` (`network/file_server.c`), which either serves straight from the in-memory cache (`cache_GetResource`,
   §5) or streams a file off disk (`TransmitFileToSocket`) in `MAX_FILE_READ_BLOCK_KB`-sized chunks.
4. Headers are sent piecewise via `ASRV_Send()` calls tagged with `MSG_MORE` on every piece except the true
   last one, so the kernel coalesces a response's header lines + body into as few TCP segments as possible
   (`network/sendHTTPHeader.c`, `network/file_server.c`).

### 3.5 Keep-alive — an important non-obvious behavior

AmmarServer's keep-alive default is **not** what HTTP/1.1 specifies. `output->keepalive` starts at 0 (false) and
is only ever set to 1 if the request explicitly contains a `Connection: keep-alive` header
(`header_analysis/http_header_analysis.c`, `AnalyzeHTTPLineRequest`'s `HTTPHEADER_CONNECTION` case). Per spec,
HTTP/1.1 should default to keep-alive *unless* the client says `Connection: close`. Many real clients
(benchmark tools especially) don't send an explicit keep-alive header, relying on the spec default — against
this server, they get a fresh TCP connection (and full 3-way handshake) per request instead. This was discovered
and measured this session (`wrk` without `-H "Connection: keep-alive"` got ~33K req/s on a fast-path-eligible
static file; the same request *with* that header got ~145K req/s on the same build). See `knowledge/issues.md`.

### 3.6 Shutdown

`AmmServer_Stop()` → `StopThreadedHTTPServer()` shuts down the listening socket(s) (forcing the accept-loop's
`select()` to unblock), waits for `stop_server==2`, joins prespawned threads and the epoll accept-layer thread,
then tears down the client/session lists. Fresh (non-prespawned) worker threads are detached, not joined — they
exit on their own once their current request finishes.

## 4. HTTP request parsing (`header_analysis/`)

Parsing is deliberately zero-copy where possible: the raw received bytes (`headerRAW`) are tokenized *in
place* — lines are NUL-terminated by overwriting their CR/LF, and most `struct HTTPHeader` fields
(`cookie`, `host`, `referer`, `eTag`, `userAgent`, ...) are just pointers into that same buffer, not copies.
This is fast but means the buffer's lifetime and any pointer-recalculation-on-realloc logic
(`recalculateHeaderFieldsBasedOnANewBaseAddress` in `generic_header_tools.c`) matter a lot for correctness.

- `recvHTTPHeader.c` — owns the receive loop and buffer growth (`growHeader`, used for POST bodies that exceed
  the initial allocation).
- `generic_header_tools.c` — the line-splitting state machine (`keepAnalyzingHTTPHeader`), header-completeness
  detection (`HTTPRequestIsComplete` — POST uses `Content-Length`, everything else scans for a blank line), and
  the buffer-growth/pointer-recalculation machinery.
- `http_header_analysis.c` — `ProcessFirstHTTPLine()` parses the request line (method, resource path, query
  string) and does the path-traversal safety checks (`FilenameStripperOk`, `StripHTMLCharacters_Inplace`,
  `ReducePathSlashes_Inplace` — all in `tools/http_tools.c`) that resolve the client's requested path into
  `verified_local_resource`, the only string ever handed to `fopen()`. `AnalyzeHTTPLineRequest()` dispatches
  each subsequent header line by name (`Authorization`, `Accept-Encoding`, `Cookie`, `Connection`, `Host`,
  `If-None-Match`, `If-Modified-Since` [not implemented, see issues.md], `Range`, `Referer`).
- `post_header_analysis.c` — parses POST-specific header lines (`Content-Type`, `Content-Disposition`,
  `Content-Length`) as they're scanned; on `Content-Type` it extracts and NUL-terminates the multipart
  `boundary=` value (capped at 64 bytes). Once a boundary is known, every subsequent line is `strstr`-checked
  for a boundary occurrence (guarded against false positives inside binary payload data by requiring it not be
  preceded by a newline).
- `post_data.c` — does the heavy lifting once the header is fully parsed: `finalizePOSTData()` walks each
  detected boundary, pulls `name=`/`filename=` out of its `Content-Disposition` line, and finds the field's
  payload end by `memmem()`-searching forward for the *next* boundary (stripping the trailing CRLF). Distinguishes
  file parts from plain-text parts. `getPOSTItemFromName()` does an exact, NUL-bound `strcmp` lookup — this was
  a deliberate fix (an inline comment documents that it used to be a prefix `strncmp`, causing e.g. a `"captcha"`
  lookup to match a `"captchaID"` field).
- `get_data.c` — parses GET query strings and other generic `name=value&name=value` payloads via a small state
  machine (`finalizeGenericGETField`). Unlike `post_data.c`, `getGETItemFromName()` still uses a **prefix**
  `strncmp` match (not fixed) — see issues.md.
- `cookie_data.c` — parses `Cookie: name1=value1; name2=value2`, with its own separate state machine (not
  reusing `get_data.c`'s, since `;`-separated cookies need different parsing *and* — critically — the outer
  per-line tokenizer is still walking forward through the same shared buffer while this runs). Writing a NUL
  terminator past a cookie value would corrupt the byte the outer tokenizer needs next, so this function
  deliberately does **not** NUL-terminate the last cookie on a line — its value is bounds-checked via a
  pointer+length, never `strlen()`. `_COOKIEcpy()`/`_COOKIEcmp()` in `main.c` are written accordingly (bounded
  `memcmp`/`memcpy`, not `strcmp`/`strcpy`) — this is a deliberate, documented, load-bearing constraint, not an
  oversight; if you ever touch this code, preserve it.
- `stringscanners/*.c` — small perfect-hash/keyword-matching lexers, one per fixed word-set: HTTP method lines
  (`firstLines.c`), request header field names (`httpHeader.c`), POST header field names (`postHeader.c`), and
  file-extension → content-type classification (`imageFiles.c`, `audioFiles.c`, `videoFiles.c`, `textFiles.c`,
  `archiveFiles.c`, `applicationFiles.c`). These genuinely *are* generated — not by a build step, but by a
  separate tool at `src/StringRecognizer/` (`generateAmmServerScanners.sh` invokes a compiled `StringRecognizer`
  binary once per word-set and copies the output `.c`/`.h` into `stringscanners/`). To add a new recognized
  header/keyword: edit the word list under `src/StringRecognizer` and re-run that script — don't hand-edit the
  generated files.

## 5. The resource cache and dynamic content (`cache/`)

`struct cache_item` (`cache/file_caching.h`) is the unit of everything servable: a static file's bytes, *or* a
dynamic resource's context (`dynamicRequestCallbackFunction` + `dynamicRequest` pointer back to its
`AmmServer_RH_Context`). `cache_GetResource()` is the single lookup-or-generate entry point `SendFile()` calls.

**Static files** are read once, optionally compressed, and kept in memory (`cache_AddFile`) — with a size/count
budget (`MAX_SEPERATE_CACHE_ITEMS`, `MAX_CACHE_SIZE_IN_MB`, `MAX_CACHE_SIZE_FOR_EACH_FILE_IN_MB`).

**Dynamic content** is registered via `AmmServer_AddResourceHandler(instance, &context, "/path.html",
max_response_bytes, callback_cooldown_ms, callback_fn, scenario)`. `scenario` is a bitmask
(`AmmServerlib.h`, `enum RHScenarios`):
- `SAME_PAGE_FOR_ALL_CLIENTS` — one shared response buffer (`context->requestContext.content`), regenerated by
  the callback at most once per `callback_cooldown_ms` (`checkRequestFrequency()` in `dynamic_requests.c`);
  concurrent requests during the cooldown just get served the last-generated copy. The whole
  check-then-regenerate-then-read sequence is serialized with `context->content_mutex` (this session's fix — see
  issues.md for the original race). Good for pages whose content is genuinely global (a clock, a stats page).
- `DIFFERENT_PAGE_FOR_EACH_CLIENT` — a fresh heap buffer is allocated per request, callback always runs, no
  sharing/locking needed. This is what this session's `/primes.html` compute benchmark uses, and what any
  resource reflecting request-specific data (form input, GPS coordinates, a computed page) must use — using
  `SAME_PAGE_FOR_ALL_CLIENTS` for per-client data would leak one client's response to another.
- `ENABLE_RECEIVING_FILES` — turns on POST handling for this resource.

The callback itself (`void * my_callback(struct AmmServer_DynamicRequest * rqst)`) writes into
`rqst->content` (up to `rqst->MAXcontentSize`), sets `rqst->contentSize`, and returns — no framework
lifecycle beyond that. `_GET`/`_POST`/`_COOKIE`/`_SESSION` accessors (declared in `AmmServerlib.h`, implemented
via `header_analysis/get_data.c` / `post_data.c` / `cookie_data.c` / `cache/session_list.c`) read the parsed
request data out of `rqst`.

`cache/client_list.c` tracks per-IP state (a hashmap keyed by IP string) — ban status and simple resource-access
throttling. `cache/session_list.c` provides `_SESSION()`-backed server-side session storage, keyed by a cookie
AmmarServer manages.

## 6. Services (`src/Services/`)

Each subdirectory is a standalone executable (`main.c`) linking `libAmmarServer`, demonstrating one thing. There
is no `src/Services/CMakeLists.txt` — every service is wired directly into the top-level `CMakeLists.txt` via
its own `add_subdirectory()` call (build order: AmmServerlib → AmmClient → small support libs → showcase
services → "less developed" services → AmmBus-related → SimpleTemplate → MyRemoteDesktop (opt-in) → misc
utilities → Deprecated). The canonical starting point for building your *own* service is
`src/Services/SimpleTemplate/main.c` — deliberately minimal, registers just one `SAME_PAGE` and one
`DIFFERENT_PAGE` resource side by side with heavy explanatory comments.

| Service | Purpose | Port | Mechanism | Built by default? |
|---|---|---|---|---|
| **AmmarServer** | Primary demo/reference service. `stats.html` (SAME_PAGE clock), `gps.html` (DIFFERENT_PAGE, lat/lon), `formtest.html`, `chatbox.html`, `random.html`, `monitor.html` (built-in health page), `hello.html` (periodic self-update), `stop.html` (disabled by default — unsafe, see issues.md), `execute.html` (runs a configured shell script via `system()` and returns its output), `primes.html` (this session's compute benchmark). | 8080 (admin 8082) | Mix | Yes |
| **MyURL** | URL-shortener (create / redirect / captcha via AmmCaptcha). | 8080 | Mix | Yes |
| **MyLoader** | Generic file upload/download/hosting; enforces a 3.5GB total / 4MB per-upload cap. | 8085 | DIFFERENT_PAGE | Yes |
| **MyBlog** | Minimal blog engine (index/page/post/rss.xml); has a `database.c` and a stray `#sqlite3` comment suggesting DB support was planned but never wired in. | 8080 | Mix | Yes |
| **MyTube** | Local YouTube-like host; **shells out to `youtube-dl`** to download videos, and presumably `ffmpeg` for thumbnails. | 8080 | DIFFERENT_PAGE | Yes |
| **MySearch** | Trivial query redirector to DuckDuckGo — mostly a query-parsing demo. | 8080 | Mix | Yes |
| **GeoPosShare** | GPS/location sharing; serves a companion Android APK for submitting coordinates. | 8081 (admin 8082) | Mix | Yes |
| **Social** | Minimal chat/login demo, backed by `UserAccounts`. | 8087 | DIFFERENT_PAGE | Yes |
| **HabChan** | Imageboard/textboard clone with file uploads and moderation; links AmmCaptcha + BasicImaging. | 8080 | Mix + ENABLE_RECEIVING_FILES | Yes |
| **ShareTex** | Multi-user LaTeX collaboration tool; login/dashboard/editor/compile, backed by `UserAccounts`. | 8090 | DIFFERENT_PAGE + ENABLE_RECEIVING_FILES | Yes |
| **Availability** | Group-scheduling poll tool (create/vote/results), modeled on whenavailable.com. | 8091 | DIFFERENT_PAGE + ENABLE_RECEIVING_FILES | Yes |
| **SuperMarket** | Shared shopping-list app — a C port of an existing `go.php`/`supermarket.py` tool, wire-compatible with it; links BasicImaging for item photos. | 8092 | DIFFERENT_PAGE + ENABLE_RECEIVING_FILES | Yes |
| **WebFramebuffer** | Captures and serves the Linux framebuffer as JPEG, with a 12-slot ring buffer. | 8080 | Mix | Yes |
| **V4L2ToHTTP** | Webcam (V4L2) streamer, `/cam.jpg` polled every 250ms; links a bundled V4L2 acquisition lib. | 8081 | Mix | Yes |
| **ImageGeneration** | Front-end for AI image generation — **shells out via `system()`** to external txt2img/img2img scripts. Hardcoded personal absolute paths (`/home/user/workspace/...`) — not portable as shipped. | 8080 | Mix + ENABLE_RECEIVING_FILES | Yes |
| **AmmBus** | Serial-bus/IoT device commander. | 8080 (admin 8082) | DIFFERENT_PAGE | Yes |
| **APushService** | Push-notification/device/account backend with a temperature-sensor module; persists to `db/pushService.state`. | 8087 | DIFFERENT_PAGE | Yes |
| **SimpleTemplate** | The documented starter template (see above). | 8080 | SAME_PAGE + DIFFERENT_PAGE | Yes |
| **MyRemoteDesktop** | X server screen/background capture as JPEG, `/cmd` for remote command execution via `system()`; bundles its own `xwd-1.0.5`. | 8090 | Mix | Only if `USE_XSERVER=ON` (default OFF) |
| **Various/NaoNetWalk** | Joystick control panel for an Aldebaran NAO robot (C++). | n/a | n/a | Yes |
| **Various/ScriptRunner** | Runs scripts over HTTP (C++) — another `system()`-shell-out pattern. | n/a | n/a | Yes |
| **Deprecated/CinemaPilot** | Media-player remote control; still compiled despite living under `Deprecated/`. | 8080 | Mix | Yes |
| **Apolls** | Near-duplicate of SimpleTemplate/AmmarServer's demo pattern; has a stray `TODO` file. | 8080 | SAME_PAGE + DIFFERENT_PAGE | **No** — absent from CMakeLists, looks superseded/abandoned |
| **Deprecated/SQLiteServer** | SQLite-backed demo. | 8080 | Mix | **No** — orphaned from the build, dead code kept for reference |
| **Various/libkindrvserver** | Kinect-related C++ server. | n/a | n/a | **No** — orphaned from the build |

Several services shell out to external binaries via `system()`/`popen()` — AmmarServer's `/execute.html`,
MyRemoteDesktop's `/cmd`, MyTube's `youtube-dl` call, ImageGeneration's image-generation scripts. See
`knowledge/issues.md` for why this is a real attack surface if any of these are exposed to untrusted clients.

## 7. Supporting libraries (`src/` siblings to AmmServerlib)

| Library | Purpose | Consumers |
|---|---|---|
| **Hashmap** (`src/Hashmap/`) | String-keyed hashmap, thread-safe (pthread mutex), byte-blob or `unsigned long` payloads, optional sort-then-binary-search mode. Its own header admits: *"uses serial searches for now, needs a lot of work."* | `cache/file_caching.c` (the resource-path → cache-index lookup), `cache/client_list.c`, `cache/session_list.c` |
| **InputParser** (`src/InputParser/`) | Generic string tokenizer (configurable delimiters, word/int/float accessors) — reused across the author's other repos, not AmmarServer-specific. | General config/argument parsing utility, vendored in |
| **BasicImaging** (`src/BasicImaging/`) | JPEG/PNG/PZP decode+encode, SIMD-optimized resize/thumbnail/cover-crop, EXIF-aware load, "never crashes" fallback contract — built earlier this same session. | **HabChan, SuperMarket, ImageGeneration** link it directly; **AmmCaptcha, MyURL, V4L2ToHTTP, MyRemoteDesktop, MyLoader** get it transitively through AmmCaptcha (which dropped its own duplicate `struct Image` in favor of BasicImaging's) |
| **AmmCaptcha** (`src/AmmCaptcha/`) | Renders distorted-text image CAPTCHAs to in-memory JPEG; also exposes a generic raw-pixels→JPEG encode used outside the captcha use case (frame capture). | MyURL, V4L2ToHTTP, HabChan, MyLoader, MyRemoteDesktop |
| **AmmClient** (`src/AmmClient/`) | A small, deliberately server-independent HTTP *client* library (connect/send/recv/GET/POST-file helpers) — exists mainly to give AmmMessages-generated code a way to phone home. | Only referenced (indirectly, via generated code) by AmmMessages |
| **AmmMessages** (`src/AmmMessages/`) | A code *generator* (not a runtime lib): given a message-definition file, emits C structs + AmmClient-based network code for a client/server messaging pattern. Gated behind `USE_AMMMESSAGES` (OFF by default). | **Zero services currently use it or its generated output** — looks like unused/aspirational tooling |
| **UserAccounts** (`src/UserAccounts/`) | Flat-file user account DB (SHA1/plaintext password, session tokens) plus an AmmServerlib-integration layer (`userAccountsWeb.c`) that resolves a `?s=` session parameter — notes explicitly that "AmmServerlib has no way to set a cookie," so sessions ride in the URL. | ShareTex, Social |

## 8. Templates and content-generation helpers

- `templates/errors.c` — the 6 built-in error page bodies (400/401/403/404/408/500), each embedded via a raw
  `#include` of a static HTML fragment (`templates/include/*.html`).
- `templates/icons.c` — dead code: its entire body is commented out. Directory-listing file-type icons appear
  to be served as separate on-disk files instead (`GetExtensionImage()`/`TemplatesInternalURI`), so this may be
  superseded rather than a functional gap.
- `templates/editor.c`, `templates/login.c` — each embeds one static HTML shell (an in-browser editor UI, a
  login form) and exposes a trivial callback that just serves it. No server-side auth/CSRF logic lives in
  either file — any real enforcement has to happen elsewhere.
- `tools/directory_lists.c` — generates the auto-index page for a directory with no index file
  (`ENABLE_DIRECTORY_LISTING`, on by default). Builds the page incrementally into a `realloc`-grown buffer
  (64KB initial, 16KB steps, 256KB hard cap — truncates with an in-page notice past that), with manual
  `mem_remaining` bookkeeping after every `strncat`.
- `AString/AString.c` — the template-variable substitution engine behind `AmmServer_ReplaceVariableInMemoryHandler`
  et al.: finds a literal placeholder (e.g. `%NAME%`) in a `struct AmmServer_MemoryHandler` buffer and splices
  in a replacement, shifting the remainder left/right as needed (`realloc`-ing if the replacement is longer).
  The realloc/grow branch carries the author's own `#warning "...not 100% sane"`.

## 9. Two features that exist in the API surface but are not implemented

- **Sessions** (`_SESSION()`, `cache/session_list.c`): every function in `session_list.c` is a stub —
  `sessionList_initialize` always returns NULL, `sessiontList_GetInfo`/`getSessionFromHeader` are non-void
  functions with no `return` on some paths. `instance->sessionList` is always NULL at runtime. `_SESSION()` has
  no working backing store.
- **Scheduler** (`AmmServer_AddScheduler()`, `scheduler/scheduler.c`): both the public API and the internal
  `schedulerAdd()`/`schedulerFlush()` are stubs that log "not implemented" and return 0. Nothing runs
  periodically; no timer thread exists.
- **IP-based ban/blacklist** (`security/banlist.c`): `executeBanlist()` is a stub, and — separately —
  `cache/client_list.c` (the structure that *would* back bans, per its own header's `@bug` comment) is also
  unimplemented. The two were never wired together.
- **IP geolocation** (`tools/geolocation.c`): `getIPCountry()` is a stub; the header comments the intended data
  source (ipdeny.com CIDR zone files) but nothing was ever built against it.
- **Content compression** (`cache/file_compression.c`): the zlib-based implementation itself looks complete and
  careful (memory-budget checks, realloc-to-shrink, proper zlib error handling), but is gated off by
  `ENABLE_COMPRESSION 0` / `ENABLE_DYNAMIC_CONTENT_COMPRESSION 0` — the config comment for the former reads
  *"Compression doesn't work all that well yet."*

See `knowledge/issues.md` for the full list of concrete bugs and improvement opportunities, including several
found by direct code reading this session (not just the stub/TODO comments above).

## 10. Build system

Top-level `CMakeLists.txt` sets shared compiler flags (`-O2 -fstack-protector -D_FORTIFY_SOURCE=2`,
hardening linker flags `-z relro -z now`) and `add_subdirectory`'s into `src/AmmServerlib` and each Service.
`src/AmmServerlib/CMakeLists.txt` lists every library source file explicitly (`AMMARSERVER_INGREDIENTS` — no
globbing, so a new `.c` file must be added there by hand, as this session did for `epollFastPathServer.c`) and
builds both a static (`AmmarServer`) and shared (`AmmarServerDynamic`) library, optionally linking OpenSSL
(`USE_OPENSSL`) and libevent (`USE_LIBEVENT`, currently unused by the request path — see issues.md).
A parallel Code::Blocks project file (`AmmServerlib.cbp`) is kept in sync by hand for IDE users. `scripts/get_dependencies.sh`
installs the apt packages needed to build; `scripts/install.sh` / `uninstall.sh` handle system-wide deployment
(systemd/upstart/init.d unit files are under `scripts/`, one per service).

## 11. This session's benchmarking additions (not part of AmmarServer itself)

- `3dparty/nginx`, `3dparty/wrk` — reference source and a benchmarking tool, cloned/built for comparison, not
  linked into anything.
- `scripts/benchmark_ammarserver.sh` — reusable wrk-based matrix benchmark (static + dynamic content, multiple
  concurrency levels), can start a local instance or point at any URL (`--url`), and compare two servers
  side-by-side (`--compare-url`).
- `scripts/benchmark_primes.php` — a PHP port of the `/primes.html` C callback (Sieve of Eratosthenes), for
  comparing compiled-C vs interpreted-PHP compute performance under a real web server (Apache+mod_php).
