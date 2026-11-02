# HTTP request-pipeline review — verified bugs and improvements (2026-11)

This document records the code survey done after implementing and testing Range/HEAD, Host enforcement,
absolute-form targets, chunked bodies, `Expect: 100-continue`, conditional requests and `ammServ.conf`
parsing. The automated review agent failed with an API error mid-run; this review was completed manually by
reading the pipeline and probing a live `ammarserver` instance.

Status: **the three critical findings and the directly related ETag-comparison and fast-path version issues
were fixed (2026-11); regression tests are in `tests/`. The remaining findings are open.**
See `knowledge/ammarserver.md` for the surrounding architecture and `knowledge/issues.md` for older findings.

## Request-pipeline map (the parts this review exercised)

```
threads/clientServer.c
  ServeClientKeepAliveLoop
    -> network/recvHTTPHeader.c:receiveAndParseIncomingHTTPRequest     (recv loop, validation, 100-continue, dechunk)
       -> header_analysis/generic_header_tools.c:keepAnalyzingHTTPHeader
          -> header_analysis/http_header_analysis.c:AnalyzeHTTPLineRequest
             -> ProcessFirstHTTPLine (method/version/target/absolute-form)
             -> request-header switch (Host, Range, Expect, TE, IMS/IUS, If-None-Match, ...)
             -> header_analysis/post_header_analysis.c:AnalyzePOSTLineRequest (POST fields/multipart)
       -> header_analysis/generic_header_tools.c:HTTPRequestIsComplete  (framing / completion)
       -> header_analysis/chunked_transfer.c                            (chunk scan + in-place decode)
       -> header_analysis/post_header_analysis.c:AnalyzePOSTBodyLines   (re-analysis after dechunk)
    -> network/file_server.c:SendFile                                   (preconditions, ranges, headers, body)
       -> cache/file_caching.c:cache_GetResource
       -> network/sendHTTPHeader.c:SendSuccessCodeHeader / SendNotModifiedHeader / SendErrorResponse
    -> threads/epollFastPathServer.c:TryServeOneRequest                 (cached static GET/HEAD bypass)
```

## Critical findings

### 1. Remote SIGSEGV in `If-None-Match` parsing — ✅ FIXED

**Where:** `src/AmmServerlib/header_analysis/http_header_analysis.c:409-417`

```c
output->eTagLength = request_length-payload_start;
if (output->eTag[0]==' ') { ++output->eTag; --output->eTagLength; }
if (output->eTag[0]=='\"') { ++output->eTag; --output->eTagLength; }
if (output->eTag[output->eTagLength-1]=='\"') { --output->eTagLength; }
```

`eTagLength` is unsigned. A header value consisting only of spaces and/or a quote (for example
`If-None-Match: ` or `If-None-Match: "`) drives it to 0; the next check then indexes
`eTag[0-1] == eTag[UINT_MAX]`, a wild read.

**Verified:** sending
`GET /index.html HTTP/1.1\r\nHost: x\r\nIf-None-Match: \r\n\r\n`
kills the server with SIGSEGV (process exit status `-11`).

**Fix applied:** every trim is length-guarded (and now trims all leading spaces/tabs), and the ETag
comparison moved into `RequestETagMatches()` (`file_server.c`), which rejects zero-length tags.

```c
while ((output->eTagLength>0) && ((output->eTag[0]==' ')||(output->eTag[0]=='\t'))) { ++output->eTag; --output->eTagLength; }
if ((output->eTagLength>0) && (output->eTag[0]=='"')) { ++output->eTag; --output->eTagLength; }
if ((output->eTagLength>0) && (output->eTag[output->eTagLength-1]=='"')) { --output->eTagLength; }
```

Regression tests: `tests/test_conditionals.py::TestETagEdgeCases`.

### 2. Premature POST completion / request desync — ✅ FIXED

**Where:** `src/AmmServerlib/header_analysis/generic_header_tools.c:548-603`
(the `requestType==POST` branch of `HTTPRequestIsComplete`).

When the header terminator has not yet arrived (`headerRAWHeadSize==0`), `httpHeaderLength` is 0 and
`totalHTTPRecvSize` is just `ContentLength`. If the bytes received so far exactly equal `Content-Length`
(e.g. a client sends a complete-looking header block with no blank line and sets `Content-Length` to that
byte count), the final `if (headerRAWSize <= totalHTTPRecvSize) return 1;` declares the request complete.
The server then responds before the blank line and body exist; the rest of the request stays in the socket and
can be read as the next request.

**Verified:** the 59-byte request
`POST /formtest.html HTTP/1.1\r\nHost: x\r\nContent-Length: 59\r\n`
(no blank line, no body) receives an immediate `HTTP/1.1 200 OK`.

**Fix applied:** after the fallback header-end scan, `HTTPRequestIsComplete()` now refuses to decide anything
until the header terminator exists:

```c
if (transaction->incomingHeader.headerRAWHeadSize==0) { return 0; }
```

This also lets the existing grow-header path handle a genuinely full buffer. Regression test:
`tests/test_security.py::TestRequestSmuggling.test_08_unterminated_header_with_matching_content_length_waits`.

### 3. Request-body lines are parsed as HTTP request headers — ✅ FIXED

**Where:** `src/AmmServerlib/header_analysis/generic_header_tools.c:437`
(`AnalyzeHTTPLineRequest()` is called for every complete line, including bytes after the header terminator).

`keepAnalyzingHTTPHeader()` does not distinguish header lines from body lines for non-chunked requests.
A body line therefore goes through the full request-header switch. Verified effects:

- A GET with **no Host header** but a body line `Host: x\r\n` passes HTTP/1.1 Host validation and gets 200;
  the same request without the body line gets 400.
- A GET with `Host` and a body line `Range: bytes=0-0\r\n` returns **206 Partial Content** with a
  `Content-Range` header.

Other request fields (`Transfer-Encoding`, `Expect`, `If-Modified-Since`, cookies, ...) are equally
injectable from body data. Chunked requests are protected by `CapRequestLengthToHeaderEnd()`; non-chunked
POSTs intentionally feed body lines to the multipart boundary scanner, but those lines must not be allowed to
mutate request-header fields.

**Fix applied:** `keepAnalyzingHTTPHeader()` now classifies each line by its offset relative to
`headerRAWHeadSize`. Body lines are never passed to `AnalyzeHTTPLineRequest()`; for non-chunked POSTs they go
only to `AnalyzePOSTLineRequest()` (multipart boundaries), and for other methods they are skipped. The POST
scanner's `Content-Type`, `Content-Disposition` and `Content-Length` cases use the same `POSTLineIsBody()`
guard, so a multipart part cannot reset the request's boundary/POST items mid-body.

Regression tests: `tests/test_security.py::TestBodyHeaderInjection` and
`tests/test_request_framing.py::TestChunkedAndExpect.test_16_part_content_type_with_boundary_does_not_reset_items`.

## High / medium findings

### 4. ETag comparison semantics (`file_server.c:613`, `:628`) — ✅ FIXED

- **Empty tag matched everything:** `If-None-Match: ""` left `eTagLength==0`, and
  `strncmp(..., 0)==0` was always true.
- **Prefix match:** `strncmp(request->eTag, LocalETag, request->eTagLength)` compared only the client's
  length, so a prefix of the real ETag matched.
- **Wildcard unsupported:** `If-None-Match: *` returned 200 instead of 304.

**Fix applied:** added `RequestETagMatches()` in `file_server.c`. It rejects zero-length tags, treats `*` as
the RFC 7232 wildcard (matching any existing representation, including disk-served files with no local ETag),
and requires an exact length match before `memcmp`. Both the dynamic-memory and static branches use it.
Regression tests: `tests/test_conditionals.py::TestETagEdgeCases`.

Remaining open items in this area: weak tags (`W/"..."`) and multi-tag lists are not parsed, and the local
ETag is still built from `(version, cache hash, range_start, range_end)`, so it changes with the requested
range instead of identifying the representation.

### 5. Fast-path Host/version parsing (`epollFastPathServer.c:96-106`, `:162`, `:207`) — ✅ FIXED

- **Host requirement bypass:** `request_line_is_http_1_1()` only recognized the version when it was the last
  token with no trailing whitespace. `GET /range.bin HTTP/1.1 \r\n` (trailing space, no Host) was served 200
  from the fast path for a cached file, while the normal path would answer 400. Verified.
- **HTTP version misclassification:** `is_http_1_0 = strcasestr(peek," HTTP/1.0")` searched the whole
  header block. A header value containing ` HTTP/1.0` (e.g. a User-Agent) made the fast path emit
  `Connection: close` for an HTTP/1.1 request. Verified.
- `says_keepalive` / `says_close` were whole-buffer substring checks rather than parsed header lines.

**Fix applied:** `parse_request_line_version()` now parses method/target/version positionally from the first
line, stops the version token at whitespace (so trailing spaces are tolerated), and rejects unknown versions
by falling back to the worker. The keep-alive default is derived from that parsed version only, and the Host
count is required for HTTP/1.1. `says_keepalive` / `says_close` now require a real `\nConnection:` line, though
they are still substring checks rather than full token parsing.
Regression tests: `tests/test_request_framing.py::TestHostAndAbsoluteForm.test_14...` and `test_15...`.

### 6. HEAD responses from non-file paths still send a body

`SendMemoryBlockAsFile()` (`network/file_server.c:786`) and `SendEmbeddedFile()` send the response body
unconditionally. A `HEAD` for a generated directory listing or an embedded template resource therefore
returns a body, unlike the main `SendFile()` path, which now suppresses it. Add a
`transaction->incomingHeader.requestType==HEAD` check before sending the body (still send the correct
`Content-Length`).

### 7. Access log records the wrong status for non-200 SendFile results

`ServeClientKeepAliveLoop()` calls `logSuccess(...,200,...)` whenever `SendFile()` returns non-zero
(`threads/clientServer.c:224`, `:389`, `:425`). `SendFile()` returns non-zero for 206, 304, 412, 416 and for
its internal 404/400 error paths, so those are all logged as `200` in the access log. Either teach
`SendFile()` to return the status it sent (out-parameter) or log from inside `SendFile()`/the send helpers.

### 8. `SendErrorCodeHeader()` builds an invalid status line

`network/sendHTTPHeader.c:32-52` formats `"<code> <reason> <templates_root><code>.html"` into a variable
named `response`, then puts that whole string after `HTTP/1.1`. A 403 response therefore starts with
something like `HTTP/1.1 403 Forbidden /path/templates/403.html`. Only the 403 paths still use this function
(`clientServer.c:311`, `:519`); use `SendErrorResponse()` there instead, or split the reason phrase from the
template path.

### 9. Conditional request gaps

- `If-Range` is not parsed: a Range request is honored even when the client's validator does not match the
  current representation. RFC 7233 requires ignoring Range in that case.
- `If-Match` is not parsed, and `If-Unmodified-Since` is evaluated even when `If-Match` is present; RFC 7232
  says IUS must be ignored when If-Match is present.
- `If-None-Match` with a *specific* tag is only evaluated for cached resources (they have an ETag); disk-served
  files advertise no ETag. The `If-None-Match: *` wildcard now works for disk files as well, because it
  matches any existing representation without needing a local ETag.

### 10. `Expect: 100-continue` is sent to HTTP/1.0 clients

`network/recvHTTPHeader.c:221-245` checks only method/body/expect flags, not the HTTP version. RFC 7231 says
a server must not send 100 Continue to an HTTP/1.0 client. Add
`httpVersionMajor==1 && httpVersionMinor>=1` to the condition.

### 11. File can disappear between `stat()` and body transmission

`SendFile()` stats the file, sends the 200/206 header and content headers, then calls
`TransmitFileBodyToSocket()`. If the file is deleted/renamed in between, the client receives a complete
header with a `Content-Length` and no body, then the connection closes. Open the file (or hold an fd) before
sending the headers, or use `sendfile()` from an fd obtained up front.

### 12. Fast path omits `Last-Modified`

The worker now emits `Last-Modified` for static files, but the fast path does not, so the same URL can return
a 200 with `Last-Modified` via the worker and a 200 without it via the fast path (verified: second GET is
fast-path served and has no `Last-Modified`). `cache[index].modification_time` is available; add the header
to the fast-path response (and keep the conditional disqualifiers, which already send such requests to the
worker).

## Lower-priority improvements

- `HTTPRequestIsComplete()` still has an unconditional `fprintf()` per POST recv (generic_header_tools.c:575);
  put it behind `DEBUG_MESSAGES`.
- Unknown keys in the auto-loaded `default.conf` produce ~15 warning lines at every startup
  (`server_configuration.c`, unknown-key branch). Consider a single summary line, or a "known but
  unimplemented" list.
- `InputParser_Create()` failure is not checked in `LoadConfigurationFile()`; a NULL dereference follows on
  allocation failure.
- `SendSuccessCodeHeader()` always advertises `Accept-Ranges: bytes`, including dynamic memory and compressed
  representations where Range is ignored. Omit it there or make it conditional.
- `SendErrorFile()` returns the error body length, which callers treat as "success" (see finding 7). A
  separate "response was sent" return value would remove the conflation.
- `cache[index].modification_time` is written during cache insertion and read from other threads without
  synchronization (the cache's existing content/size metadata has the same pre-existing race). Consider
  populating the item before publishing its hash entry, or protecting metadata with the cache lock.
- `ParseHTTPDate()` does not accept the RFC 7231 asctime format and accepts a date without verifying `GMT`.
- The test harness resolves `AMMARSERVER_BINARY` relative to the server's temporary CWD; make env-provided
  paths absolute before `Popen`.
- The `If-None-Match` parser trims only one leading space; use a whitespace loop.

## Test-suite gaps (regression tests to add)

The suite had 156 tests before the fixes; it now has 170 and passes. Cases 1–6 below are now covered by
regression tests; 7–11 remain open test gaps:

1. ✅ `If-None-Match:` with empty/space-only/quoted-empty values — `TestETagEdgeCases`.
2. ✅ `If-None-Match: *` and prefix-ETag matching — `TestETagEdgeCases`.
3. ✅ Unterminated POST header whose byte length equals `Content-Length` —
   `TestRequestSmuggling.test_08_unterminated_header_with_matching_content_length_waits`.
4. ✅ Host/Range/Transfer-Encoding injected via a GET body line — `TestBodyHeaderInjection`.
5. ✅ Fast-path request line with trailing whitespace after `HTTP/1.1` and no Host —
   `TestHostAndAbsoluteForm.test_14_trailing_space_version_still_requires_host`.
6. ✅ HTTP/1.1 request whose User-Agent contains ` HTTP/1.0` —
   `TestHostAndAbsoluteForm.test_15_header_value_does_not_change_keepalive`.
7. `HEAD` on a generated directory listing and on an embedded template resource (expect no body bytes).
8. Access/error log status for 206, 304, 412, 416 (currently logged as 200).
9. 403 status line (currently contains the template path).
10. `Expect: 100-continue` from an HTTP/1.0 client (expect no interim 100).
11. `If-Range` mismatch (expect full 200, not 206) and `If-Match`/IUS interaction once implemented.

## Verification notes

All of the "verified" items above were reproduced against a temporary `ammarserver` instance started by the
test harness pattern (temp web root/config, loopback port). The current test suite was run with:

```bash
bash tests/run_tests.sh --no-build
# 170 tests, OK
```

The crash finding was observed as the server process exiting with status `-11` (SIGSEGV) while handling the
crafted `If-None-Match` request; the desync finding as a `200 OK` received before the rest of the POST was
sent; the body-injection findings as 200-vs-400 and 206-vs-200 differences between a request with and without
the injected body line.
