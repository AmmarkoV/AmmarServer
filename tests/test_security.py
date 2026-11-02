"""Security / pentest cases.

These tests probe path traversal, symlink escapes, malformed and oversized
requests, request-smuggling framing and resilience against slow clients.  A
failure here is a security-relevant finding, not just a behavior mismatch.
"""

import os
import socket
import time
import unittest

from .harness import (
    MULTIPART_BOUNDARY,
    ServerTestCase,
    make_request,
    multipart_body,
    parsed_status,
    raw_exchange,
)


class TestPathTraversal(ServerTestCase):
    server_config = {"Caching": "On"}

    def test_01_traversal_variants_are_rejected(self):
        targets = [
            "/../etc/passwd",
            "/../../../../etc/passwd",
            "/%2e%2e/%2e%2e/etc/passwd",
            "/..%2f..%2fetc%2fpasswd",
            "/....//etc/passwd",
            "/..\\..\\etc\\passwd",
            "/%00index.html",
            "/index.html%00.txt",
            "..",
            "/../",
        ]
        for target in targets:
            with self.subTest(target=target):
                response = self.request("GET", target)
                self.assertIn(response.status, (400, 404))
                self.assertNotIn(b"root:", response.body)
                self.assertNotIn(b"/bin/", response.body)

        self.assert_server_alive()

    def test_02_crlf_in_target_does_not_inject_headers(self):
        response = self.request(
            "GET", "/index.html%0d%0aX-Injected:%20yes")
        self.assertIn(response.status, (400, 404))
        self.assertIsNone(response.header("x-injected"))
        self.assert_server_alive()

    def test_03_symlink_escape_is_blocked(self):
        secret_path = os.path.join(self.server.temp_dir, "outside-secret.txt")
        secret = b"top-secret-outside-the-web-root"
        with open(secret_path, "wb") as secret_file:
            secret_file.write(secret)

        link_path = os.path.join(self.server.web_root, "leak.txt")
        try:
            os.symlink(secret_path, link_path)
        except (OSError, NotImplementedError):
            self.skipTest("symlinks are not available on this platform")

        response = self.request("GET", "/leak.txt")
        self.assertIn(response.status, (400, 404))
        self.assertNotIn(secret, response.body)
        self.assert_server_alive()

    def assert_server_alive(self):
        self.assertEqual(self.request("GET", "/index.html").status, 200)


class TestMalformedRequests(ServerTestCase):
    server_config = {"Caching": "On"}

    def assert_server_alive(self):
        self.assertEqual(self.request("GET", "/index.html").status, 200)

    def test_01_disconnect_without_request(self):
        sock = socket.create_connection(("127.0.0.1", self.port), timeout=3)
        sock.close()
        self.assert_server_alive()

    def test_02_binary_garbage(self):
        raw = raw_exchange(self.port, b"\x00\x01\x02\x03\xff\xfe\r\n\r\n")
        self.assertIn(parsed_status(raw), (None, 400, 404, 501))
        self.assert_server_alive()

    def test_03_incomplete_request_line(self):
        raw = raw_exchange(self.port, b"GET\r\n\r\n")
        self.assertIn(parsed_status(raw), (None, 400, 404, 501))
        self.assert_server_alive()

    def test_04_request_line_without_version(self):
        raw = raw_exchange(self.port, b"GET /index.html\r\n\r\n")
        self.assertNotEqual(parsed_status(raw), 200)
        self.assert_server_alive()

    def test_05_unknown_method(self):
        response = self.request("BOGUS", "/index.html")
        self.assertIn(response.status, (400, 501))
        self.assert_server_alive()

    def test_06_slash_flood(self):
        target = "/" + ("/" * 500)
        response = self.request("GET", target)
        self.assertIn(response.status, (400, 404))
        self.assert_server_alive()

    def test_07_overlong_request_line(self):
        raw = raw_exchange(
            self.port,
            b"GET /" + (b"A" * 16384) + b" HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n",
        )
        self.assertIn(parsed_status(raw), (None, 400, 404, 414))
        self.assert_server_alive()

    def test_08_header_flood(self):
        raw = raw_exchange(
            self.port,
            b"GET /index.html HTTP/1.1\r\nHost: x\r\n"
            + b"".join(b"X-Flood-%d: a\r\n" % index for index in range(1000))
            + b"Connection: close\r\n\r\n",
        )
        self.assertIn(parsed_status(raw), (None, 400, 404, 431))
        self.assert_server_alive()

    def test_09_many_headers_within_limit(self):
        headers = [("X-Test-%d" % index, "a") for index in range(100)]
        response = self.request("GET", "/index.html", headers)
        self.assertEqual(response.status, 200)

    def test_10_large_user_agent_within_limit(self):
        response = self.request("GET", "/index.html", [("User-Agent", "U" * 4096)])
        self.assertEqual(response.status, 200)

    def test_11_long_nonexistent_path(self):
        response = self.request("GET", "/" + ("0" * 760))
        self.assertIn(response.status, (400, 404))
        self.assert_server_alive()

    def test_12_missing_target(self):
        raw = raw_exchange(self.port, b"GET HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n")
        self.assertNotEqual(parsed_status(raw), 200)
        self.assert_server_alive()


class TestRequestSmuggling(ServerTestCase):
    server_config = {"Caching": "On", "EnablePost": "On"}

    def assert_rejected(self, headers, body=b""):
        response = self.request("POST", "/formtest.html", headers, body)
        self.assertEqual(response.status, 400)
        self.assertEqual(self.request("GET", "/index.html").status, 200)

    def test_01_duplicate_content_length(self):
        self.assert_rejected([("Content-Length", "5"), ("Content-Length", "5")])

    def test_02_conflicting_content_length(self):
        self.assert_rejected([("Content-Length", "5"), ("Content-Length", "6")])

    def test_03_non_numeric_content_length(self):
        self.assert_rejected([("Content-Length", "abc")])

    def test_04_content_length_with_trailing_junk(self):
        self.assert_rejected([("Content-Length", "5x")])

    def test_05_content_length_comma_list(self):
        self.assert_rejected([("Content-Length", "5, 5")])

    def test_06_valid_zero_content_length(self):
        response = self.request(
            "POST", "/formtest.html",
            [("Content-Type", "multipart/form-data; boundary=%s" % MULTIPART_BOUNDARY),
             ("Content-Length", "0")])
        self.assertEqual(response.status, 200)
        self.assertNotIn(b"GOT A POST USERNAME", response.body)

    def test_07_chunked_get_is_rejected(self):
        response = self.request(
            "GET", "/index.html",
            [("Transfer-Encoding", "chunked")],
            b"0\r\n\r\n")
        self.assertEqual(response.status, 400)
        self.assertEqual(self.request("GET", "/index.html").status, 200)


class TestOversizedBodies(ServerTestCase):
    server_config = {"Caching": "On", "EnablePost": "On", "MaxPostTransactionSize": 512}

    def test_01_oversized_content_length_without_expect(self):
        response = self.request(
            "POST", "/formtest.html",
            [("Content-Length", "1024")],
            b"")
        self.assertEqual(response.status, 413)

    def test_02_oversized_multipart_body(self):
        body = multipart_body(value="x" * 2048)
        response = self.request(
            "POST", "/formtest.html",
            [("Content-Type", "multipart/form-data; boundary=%s" % MULTIPART_BOUNDARY),
             ("Content-Length", str(len(body)))],
            body)
        self.assertEqual(response.status, 413)
        self.assertEqual(self.request("GET", "/index.html").status, 200)


class TestSlowClients(ServerTestCase):
    server_config = {"Caching": "On", "Timeout": 5}

    def test_01_partial_headers_do_not_block_other_clients(self):
        slow_connections = []
        try:
            for index in range(8):
                sock = socket.create_connection(("127.0.0.1", self.port), timeout=3)
                sock.sendall(b"GET /index.html HTTP/1.1\r\nHost: slow-%d\r\n" % index)
                slow_connections.append(sock)

            started = time.time()
            response = self.request("GET", "/index.html")
            self.assertEqual(response.status, 200)
            self.assertLess(time.time() - started, 3.0)
        finally:
            for sock in slow_connections:
                sock.close()

    def test_02_partial_body_does_not_block_other_clients(self):
        slow_connection = socket.create_connection(("127.0.0.1", self.port), timeout=3)
        try:
            slow_connection.sendall(
                b"POST /formtest.html HTTP/1.1\r\nHost: slow\r\n"
                b"Content-Type: text/plain\r\nContent-Length: 100\r\n\r\n"
                b"0123456789"
            )
            started = time.time()
            response = self.request("GET", "/index.html")
            self.assertEqual(response.status, 200)
            self.assertLess(time.time() - started, 3.0)
        finally:
            slow_connection.close()


if __name__ == "__main__":
    unittest.main(verbosity=2)
