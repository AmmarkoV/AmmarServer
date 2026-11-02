# AmmarServer test suite

This directory is the home of AmmarServer's automated and manual testing.

The automated suite is a Python 3 `unittest` package. It starts the real
`ammarserver` binary on a free loopback port, with a temporary web root,
templates directory, log directory and configuration file for each test class,
and then drives it over raw sockets so status lines, header framing and bodies
can be asserted exactly. It does not touch `public_html/`, does not require a
server to already be running, and needs no third-party Python packages.

## Quick start

```bash
# from the repository root
bash tests/run_tests.sh
```

The runner builds `ammarserver` first. Pass `--no-build` to skip that step.
To run a single module (from the repository root):

```bash
./tests/run_tests.sh tests.test_security
./tests/run_tests.sh tests.test_server_behavior
```

You can also run the package directly from the repository root:

```bash
python3 -m unittest discover -s tests -t . -p 'test_*.py' -v
```

### Environment variables

- `AMMARSERVER_BINARY` – use a different server binary (used by the harness).
- `AMMARSERVER_LIBRARY_DIR` – directory containing `libAmmarServer.so`.

## Layout

```
tests/
├── run_tests.sh              unified runner (build + unittest)
├── harness.py                Server, RawConnection, fixtures and assertion helpers
├── test_ranges.py            byte ranges, 206/416, HEAD+Range, cached and disk profiles
├── test_conditionals.py      Last-Modified, If-Modified-Since, If-Unmodified-Since, ETag
├── test_request_framing.py   Host, absolute-form, chunked bodies, 100-continue, size limits
├── test_configuration.py     ammServ.conf keys, search order, KeepAlive/EnablePost/limits
├── test_server_behavior.py   MIME types, index files, directory listings, methods,
│                             keep-alive, ETag stability, concurrency and logging
├── test_security.py          traversal, symlink escape, malformed/oversized input,
│                             request smuggling and slow-client resilience
├── legacy/                   original shell pentest scripts and raw request fixtures
└── load/                     manual siege/wget load and DoS trials
```

## What the tests cover

- **Functional HTTP features**: Range/HEAD download resume, conditional GETs,
  Host enforcement, absolute-form request targets, chunked request bodies,
  `Expect: 100-continue`, and configuration parsing.
- **General server behavior**: static file serving, MIME types, index files,
  generated directory listings, content-length framing, supported/unsupported
  methods, keep-alive semantics (HTTP/1.0 and 1.1), ETag stability, compression
  negotiation when compression is compiled out, concurrent requests, and
  access/error logging.
- **Security / pentests**: path traversal in several encodings, CRLF injection
  attempts, symlink escapes outside the web root, malformed and oversized
  requests, header floods, duplicate/invalid `Content-Length`, mixed
  `Transfer-Encoding`/`Content-Length`, oversized bodies, and a small
  slow-client test to make sure stalled connections do not block others.

## Legacy and load assets

`legacy/` retains the original shell-based pentest harness (`TestServer.sh`),
its raw request fixtures (`testfile1` … `testfile11`) and `unitTest.sh`. They
are kept for reference; their useful cases have been folded into
`tests/test_security.py` and `tests/test_server_behavior.py`.

`load/` contains the manual stress/DoS scripts (`siege_trial.sh`,
`multiSiege.sh`, `dos_trial.sh`). They expect a server on `127.0.0.1:8080` and
require external tools such as `siege`, `wget` and `gnuplot`; they are not run
by `run_tests.sh`.
