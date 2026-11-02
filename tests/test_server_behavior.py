"""General server behavior: static files, MIME types, methods, keep-alive,
concurrency, caching headers and logging."""

import concurrent.futures
import unittest

from .harness import (
    MULTIPART_BOUNDARY,
    RANGE_BYTES,
    RawConnection,
    Server,
    ServerTestCase,
    make_request,
    multipart_body,
)


class TestStaticServing(ServerTestCase):
    server_config = {"Caching": "On"}

    def test_01_index_file_for_root(self):
        response = self.request("GET", "/")
        self.assertEqual(response.status, 200)
        self.assertIn(b"index fixture", response.body)

    def test_02_mime_types(self):
        expected = {
            "/index.html": "text/html",
            "/text.txt": "text/txt",
            "/style.css": "text/css",
            "/image.png": "image/png",
        }
        for path, content_type in expected.items():
            with self.subTest(path=path):
                response = self.request("GET", path)
                self.assertEqual(response.status, 200)
                self.assertEqual(response.header("content-type"), content_type)

    def test_03_directory_listing(self):
        response = self.request("GET", "/subdir/")
        self.assertEqual(response.status, 200)
        self.assertIn(b"child.txt", response.body)

    def test_04_content_length_matches_body(self):
        for path in ("/index.html", "/text.txt", "/image.png", "/empty.bin"):
            with self.subTest(path=path):
                response = self.request("GET", path)
                self.assertEqual(response.status, 200)
                self.assertEqual(int(response.header("content-length")), len(response.body))

    def test_05_etag_is_stable(self):
        first = self.request("GET", "/range.bin")
        second = self.request("GET", "/range.bin")
        self.assertEqual(first.status, 200)
        self.assertEqual(second.status, 200)
        self.assertIsNotNone(first.header("etag"))
        self.assertEqual(first.header("etag"), second.header("etag"))
        self.assertEqual(first.body, second.body)

    def test_06_compression_request_without_compression_support(self):
        # ENABLE_COMPRESSION is 0 in this build; a deflate request must still
        # return the identity representation and must not claim deflate.
        response = self.request("GET", "/range.bin", [("Accept-Encoding", "deflate")])
        self.assertEqual(response.status, 200)
        self.assertIsNone(response.header("content-encoding"))
        self.assertEqual(response.body, RANGE_BYTES)

    def test_07_head_does_not_send_a_body(self):
        response = self.request("HEAD", "/index.html")
        self.assertEqual(response.status, 200)
        self.assertEqual(response.body, b"")
        self.assertIsNotNone(response.header("content-length"))


class TestBasicRegression(ServerTestCase):
    server_config = {"Caching": "On"}

    def test_01_plain_get_and_head(self):
        get_response = self.request("GET", "/index.html")
        self.assertEqual(get_response.status, 200)
        self.assertIn(b"index fixture", get_response.body)

        head_response = self.request("HEAD", "/index.html")
        self.assertEqual(head_response.status, 200)
        self.assertEqual(head_response.body, b"")
        self.assertEqual(head_response.header("content-length"), str(len(get_response.body)))

    def test_02_not_found(self):
        response = self.request("GET", "/does-not-exist.bin")
        self.assertEqual(response.status, 404)
        self.assertIn(b"404", response.body)

    def test_03_content_length_post_still_works(self):
        body = multipart_body(value="normalpost")
        response = self.request(
            "POST", "/formtest.html",
            [("Content-Type", "multipart/form-data; boundary=%s" % MULTIPART_BOUNDARY),
             ("Content-Length", str(len(body)))],
            body)
        self.assertEqual(response.status, 200)
        self.assertIn(b"normalpost", response.body)


class TestMethods(ServerTestCase):
    server_config = {"Caching": "On"}

    def test_01_unsupported_methods_are_not_implemented(self):
        requests = [
            ("PUT", "/range.bin"),
            ("DELETE", "/range.bin"),
            ("TRACE", "/range.bin"),
            ("OPTIONS", "*"),
            ("CONNECT", "127.0.0.1:80"),
            ("PATCH", "/range.bin"),
        ]
        for method, target in requests:
            with self.subTest(method=method):
                response = self.request(method, target)
                self.assertEqual(response.status, 501)

        # The server must still work after all of those
        self.assertEqual(self.request("GET", "/index.html").status, 200)


class TestKeepAlive(ServerTestCase):
    server_config = {"Caching": "On", "KeepAlive": "On"}

    def test_01_http11_keepalive(self):
        connection = self.connect()
        try:
            connection.send(make_request("GET", "/index.html", port=self.port))
            first = connection.read_response("GET")
            self.assertEqual(first.status, 200)
            self.assertEqual((first.header("connection") or "").lower(), "keep-alive")

            connection.send(make_request("GET", "/range.bin", [("Connection", "close")], port=self.port))
            second = connection.read_response("GET")
            self.assertEqual(second.status, 200)
            self.assertEqual(second.body, RANGE_BYTES)
        finally:
            connection.close()

    def test_02_http10_defaults_to_close(self):
        connection = self.connect()
        try:
            connection.send(make_request("GET", "/index.html", version="HTTP/1.0", host=None, port=self.port))
            response = connection.read_response("GET")
            self.assertEqual(response.status, 200)
            self.assertEqual((response.header("connection") or "").lower(), "close")
            connection.socket.settimeout(2.0)
            self.assertEqual(connection.socket.recv(1), b"")
        finally:
            connection.close()

    def test_03_http10_keepalive_opt_in(self):
        connection = self.connect()
        try:
            connection.send(make_request(
                "GET", "/index.html",
                [("Connection", "keep-alive")], version="HTTP/1.0", host=None, port=self.port))
            first = connection.read_response("GET")
            self.assertEqual(first.status, 200)
            self.assertEqual((first.header("connection") or "").lower(), "keep-alive")

            connection.send(make_request("GET", "/range.bin", [("Connection", "close")],
                                         version="HTTP/1.0", host=None, port=self.port))
            second = connection.read_response("GET")
            self.assertEqual(second.status, 200)
        finally:
            connection.close()


class TestConcurrency(ServerTestCase):
    server_config = {"Caching": "On"}

    def test_01_parallel_static_requests(self):
        def fetch(_):
            connection = self.connect()
            try:
                return self.request("GET", "/range.bin", connection=connection)
            finally:
                connection.close()

        with concurrent.futures.ThreadPoolExecutor(max_workers=10) as pool:
            responses = list(pool.map(fetch, range(50)))

        for response in responses:
            self.assertEqual(response.status, 200)
            self.assertEqual(response.body, RANGE_BYTES)


class TestLogging(unittest.TestCase):
    def test_01_access_and_error_logs_are_written(self):
        server = Server(config={"Caching": "On"})
        try:
            connection = RawConnection(server.port)
            connection.send(make_request("GET", "/range.bin", [("Connection", "close")], port=server.port))
            self.assertEqual(connection.read_response("GET").status, 200)
            connection.close()

            connection = RawConnection(server.port)
            connection.send(make_request("GET", "/definitely-missing.bin",
                                         [("Connection", "close")], port=server.port))
            self.assertEqual(connection.read_response("GET").status, 404)
            connection.close()

            # Logs are fully buffered, so stop the server (clean SIGTERM path
            # calls FlushAccessAndErrorLogs) before reading them.
            server.stop(cleanup=False)

            access = server.read_access_logs().decode("latin-1", "replace")
            errors = server.read_error_logs().decode("latin-1", "replace")
            self.assertIn("/range.bin", access)
            self.assertIn(" 200 ", access)
            self.assertIn("404", errors)
        finally:
            server.cleanup()


if __name__ == "__main__":
    unittest.main(verbosity=2)
