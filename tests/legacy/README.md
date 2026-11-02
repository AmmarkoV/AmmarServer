# Legacy pentest assets

These files are the original shell-based pentest harness:

- `TestServer.sh` pipes the raw `testfile*` requests into a server already
  listening on `127.0.0.1:8080` and only checks that the process is still
  alive.
- `testfile1` … `testfile11` are raw malformed/hostile requests: slash floods,
  overlong request lines and User-Agent values, traversal attempts, an empty
  request, a normal request, and similar probes.
- `unitTest.sh` is an old valgrind wrapper for the HashMap unit test.

They are kept for reference only. The automated cases live in
`../test_security.py` and `../test_server_behavior.py`, which make real
assertions about status codes, response framing and server liveness.
