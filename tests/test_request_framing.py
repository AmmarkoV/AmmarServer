"""Request-target, Host, chunked-body and Expect: 100-continue behavior."""

import time
import unittest

from .harness import (
    MULTIPART_BOUNDARY,
    RANGE_BYTES,
    ServerTestCase,
    chunk_encode,
    make_request,
    multipart_body,
)


class TestHostAndAbsoluteForm(ServerTestCase):
    server_config = {"Caching": "On"}

    def test_01_http11_requires_host(self):
        response = self.request("GET", "/range.bin", host=None)
        self.assertEqual(response.status, 400)

    def test_02_http10_without_host_is_accepted(self):
        response = self.request("GET", "/range.bin", version="HTTP/1.0", host=None)
        self.assertEqual(response.status, 200)
        self.assertEqual(response.body, RANGE_BYTES)

    def test_03_http11_with_host(self):
        response = self.request("GET", "/range.bin")
        self.assertEqual(response.status, 200)

    def test_04_duplicate_host_is_rejected(self):
        response = self.request("GET", "/range.bin", [("Host", "duplicate.example")])
        self.assertEqual(response.status, 400)

    def test_05_no_host_after_cache_warmup(self):
        # Warm the static cache first, so this also proves the fast path did not
        # answer the Host-less request by itself.
        self.assertEqual(self.request("GET", "/range.bin").status, 200)
        self.assertEqual(self.request("GET", "/range.bin", host=None).status, 400)

    def test_06_absolute_http_form(self):
        target = "http://127.0.0.1:%d/range.bin" % self.port
        response = self.request("GET", target)
        self.assertEqual(response.status, 200)
        self.assertEqual(response.body, RANGE_BYTES)

    def test_07_absolute_https_form(self):
        target = "https://127.0.0.1:%d/range.bin" % self.port
        response = self.request("GET", target)
        self.assertEqual(response.status, 200)
        self.assertEqual(response.body, RANGE_BYTES)

    def test_08_absolute_uppercase_scheme(self):
        target = "HTTP://127.0.0.1:%d/range.bin" % self.port
        response = self.request("GET", target)
        self.assertEqual(response.status, 200)

    def test_09_absolute_with_query(self):
        target = "http://127.0.0.1:%d/range.bin?x=1" % self.port
        response = self.request("GET", target)
        self.assertEqual(response.status, 200)
        self.assertEqual(response.body, RANGE_BYTES)

    def test_10_absolute_no_path_serves_index(self):
        target = "http://127.0.0.1:%d" % self.port
        response = self.request("GET", target)
        self.assertEqual(response.status, 200)
        self.assertIn(b"index fixture", response.body)

    def test_11_absolute_no_path_with_query(self):
        target = "http://127.0.0.1:%d?x=1" % self.port
        response = self.request("GET", target)
        self.assertEqual(response.status, 200)
        self.assertIn(b"index fixture", response.body)

    def test_12_absolute_head(self):
        target = "http://127.0.0.1:%d/range.bin" % self.port
        response = self.request("HEAD", target)
        self.assertEqual(response.status, 200)
        self.assertEqual(response.header("content-length"), "36")
        self.assertEqual(response.body, b"")

    def test_13_absolute_traversal_is_rejected(self):
        target = "http://127.0.0.1:%d/../etc/passwd" % self.port
        response = self.request("GET", target)
        self.assertIn(response.status, (400, 404))


class TestChunkedAndExpect(ServerTestCase):
    server_config = {"Caching": "On", "EnablePost": "On", "MaxPostTransactionSize": 4 * 1024 * 1024}

    def _chunked_post(self, encoded_body, extra_headers=None, close=True):
        headers = [
            ("Content-Type", "multipart/form-data; boundary=%s" % MULTIPART_BOUNDARY),
            ("Transfer-Encoding", "chunked"),
        ]
        headers.extend(extra_headers or [])
        return self.request("POST", "/formtest.html", headers, encoded_body, close=close)

    def assert_echo(self, response, value):
        self.assertEqual(response.status, 200)
        self.assertIn(b"GOT A POST USERNAME", response.body)
        self.assertIn(value.encode("latin-1"), response.body)

    def test_01_chunked_multipart_post(self):
        response = self._chunked_post(chunk_encode(multipart_body(value="chunkeduser"), [5, 7, 11]))
        self.assert_echo(response, "chunkeduser")

    def test_02_chunk_extension(self):
        body = multipart_body(value="extensionuser")
        encoded = ("5;ext=1\r\n" + body[:5].decode("latin-1") + "\r\n").encode("latin-1")
        encoded += chunk_encode(body[5:])
        self.assert_echo(self._chunked_post(encoded), "extensionuser")

    def test_03_chunked_with_trailers(self):
        body = multipart_body(value="traileruser")
        encoded = chunk_encode(body)
        encoded = encoded[:-len(b"0\r\n\r\n")] + b"0\r\nX-Pentest-Trailer: yes\r\n\r\n"
        self.assert_echo(self._chunked_post(encoded), "traileruser")

    def test_04_header_split_across_writes(self):
        connection = self.connect()
        try:
            body = chunk_encode(multipart_body(value="splituser"), [9])
            headers = (
                "POST /formtest.html HTTP/1.1\r\n"
                "Host: 127.0.0.1:%d\r\n"
                "Transfer-Encoding: chunked\r\n" % self.port
            ).encode("latin-1")
            connection.send(headers)
            time.sleep(0.05)
            rest = (
                "Content-Type: multipart/form-data; boundary=%s\r\n"
                "Connection: close\r\n\r\n" % MULTIPART_BOUNDARY
            ).encode("latin-1") + body
            connection.send(rest)
            self.assert_echo(connection.read_response("POST"), "splituser")
        finally:
            connection.close()

    def test_05_chunked_then_second_request_on_same_connection(self):
        connection = self.connect()
        try:
            body = chunk_encode(multipart_body(value="keepaliveuser"))
            connection.send(make_request(
                "POST", "/formtest.html",
                [("Content-Type", "multipart/form-data; boundary=%s" % MULTIPART_BOUNDARY),
                 ("Transfer-Encoding", "chunked")],
                body, port=self.port))
            self.assert_echo(connection.read_response("POST"), "keepaliveuser")

            connection.send(make_request("GET", "/range.bin", [("Connection", "close")], port=self.port))
            response = connection.read_response("GET")
            self.assertEqual(response.status, 200)
            self.assertEqual(response.body, RANGE_BYTES)
        finally:
            connection.close()

    def test_06_expect_continue_chunked(self):
        connection = self.connect()
        try:
            body = chunk_encode(multipart_body(value="expectchunked"))
            connection.send(make_request(
                "POST", "/formtest.html",
                [("Content-Type", "multipart/form-data; boundary=%s" % MULTIPART_BOUNDARY),
                 ("Transfer-Encoding", "chunked"),
                 ("Expect", "100-continue"),
                 ("Connection", "close")],
                b"", port=self.port))
            interim = connection.read_response("POST", skip_interim=False)
            self.assertEqual(interim.status, 100)
            connection.send(body)
            self.assert_echo(connection.read_response("POST"), "expectchunked")
        finally:
            connection.close()

    def test_07_expect_continue_content_length(self):
        connection = self.connect()
        try:
            body = multipart_body(value="expectlength")
            connection.send(make_request(
                "POST", "/formtest.html",
                [("Content-Type", "multipart/form-data; boundary=%s" % MULTIPART_BOUNDARY),
                 ("Content-Length", str(len(body))),
                 ("Expect", "100-continue"),
                 ("Connection", "close")],
                b"", port=self.port))
            interim = connection.read_response("POST", skip_interim=False)
            self.assertEqual(interim.status, 100)
            connection.send(body)
            self.assert_echo(connection.read_response("POST"), "expectlength")
        finally:
            connection.close()

    def test_08_body_sent_with_expect_headers(self):
        body = multipart_body(value="samewrite")
        response = self.request(
            "POST", "/formtest.html",
            [("Content-Type", "multipart/form-data; boundary=%s" % MULTIPART_BOUNDARY),
             ("Content-Length", str(len(body))),
             ("Expect", "100-continue")],
            body)
        self.assert_echo(response, "samewrite")

    def test_09_unknown_expect(self):
        response = self.request("POST", "/formtest.html", [("Expect", "banana"), ("Content-Length", "0")])
        self.assertEqual(response.status, 417)

    def test_10_expect_without_host(self):
        response = self.request(
            "POST", "/formtest.html",
            [("Transfer-Encoding", "chunked"), ("Expect", "100-continue")],
            b"", host=None)
        self.assertEqual(response.status, 400)

    def test_11_chunked_and_content_length_are_rejected(self):
        response = self.request(
            "POST", "/formtest.html",
            [("Transfer-Encoding", "chunked"), ("Content-Length", "5")],
            b"0\r\n\r\n")
        self.assertEqual(response.status, 400)

    def test_12_malformed_chunk_size(self):
        response = self._chunked_post(b"ZZ\r\n")
        self.assertEqual(response.status, 400)

    def test_13_empty_chunked_body(self):
        response = self._chunked_post(b"0\r\n\r\n")
        self.assertEqual(response.status, 200)
        self.assertNotIn(b"GOT A POST USERNAME", response.body)

    def test_14_unsupported_transfer_coding(self):
        response = self.request(
            "POST", "/formtest.html",
            [("Transfer-Encoding", "gzip, chunked")],
            b"0\r\n\r\n")
        self.assertEqual(response.status, 501)

    def test_15_multipart_part_content_length_is_ignored(self):
        # A Content-Length line inside a multipart part header must not override
        # the request's own body length.
        body = multipart_body(
            value="partlength",
            extra_part_headers=['Content-Length: 999'],
        )
        response = self.request(
            "POST", "/formtest.html",
            [("Content-Type", "multipart/form-data; boundary=%s" % MULTIPART_BOUNDARY),
             ("Content-Length", str(len(body)))],
            body)
        self.assert_echo(response, "partlength")


class TestExpectAndSizeLimits(ServerTestCase):
    server_config = {"Caching": "On", "EnablePost": "On", "MaxPostTransactionSize": 512}

    def test_01_expect_oversized_content_length(self):
        connection = self.connect()
        try:
            connection.send(make_request(
                "POST", "/formtest.html",
                [("Content-Type", "text/plain"),
                 ("Content-Length", "1024"),
                 ("Expect", "100-continue"),
                 ("Connection", "close")],
                b"", port=self.port))
            response = connection.read_response("POST", skip_interim=False)
            self.assertEqual(response.status, 413)
        finally:
            connection.close()

    def test_02_oversized_chunk_declaration(self):
        connection = self.connect()
        try:
            connection.send(make_request(
                "POST", "/formtest.html",
                [("Content-Type", "text/plain"),
                 ("Transfer-Encoding", "chunked"),
                 ("Connection", "close")],
                b"400\r\n", port=self.port))
            response = connection.read_response("POST")
            self.assertEqual(response.status, 413)
        finally:
            connection.close()


if __name__ == "__main__":
    unittest.main(verbosity=2)
