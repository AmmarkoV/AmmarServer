"""Application-facing features the vanilla ammarserver binary has no handler
for: cookies, sessions, GET+POST fields in one handler, and API mode on both
the epoll layer and io_uring.  Driven through tests/fixtures/testservice.c."""

import unittest

from tests.harness import (
    MULTIPART_BOUNDARY,
    TESTSERVICE_BINARY,
    VALGRIND_LOG_DIR,
    RawConnection,
    ServerTestCase,
    make_request,
    multipart_body,
)


class TestCookiesAndFields(ServerTestCase):
    server_binary = TESTSERVICE_BINARY

    def test_01_no_cookie(self):
        response = self.request("GET", "/cookie.html")
        self.assertEqual(response.status, 200)
        self.assertEqual(response.body, b"flavor=none")

    def test_02_cookie_is_read(self):
        response = self.request("GET", "/cookie.html", [("Cookie", "a=1; flavor=mint; z=26")])
        self.assertEqual(response.body, b"flavor=mint")

    def test_03_cookie_name_is_matched_exactly(self):
        response = self.request("GET", "/cookie.html", [("Cookie", "flavors=many; xflavor=no")])
        self.assertEqual(response.body, b"flavor=none")

    def test_04_valueless_cookie_at_end_of_header(self):
        response = self.request("GET", "/cookie.html", [("Cookie", "flavor=mint; bare")])
        self.assertEqual(response.body, b"flavor=mint")

    def test_05_set_cookie(self):
        response = self.request("GET", "/cookie.html?set=vanilla")
        self.assertEqual(response.status, 200)
        cookies = response.header_all("Set-Cookie")
        self.assertTrue(any(cookie.startswith("flavor=vanilla") for cookie in cookies), cookies)

    def test_06_many_cookies(self):
        cookie = "; ".join("c%u=%u" % (i, i) for i in range(200)) + "; flavor=last"
        response = self.request("GET", "/cookie.html", [("Cookie", cookie)])
        self.assertIn(response.status, (200, 400, 413))

    def test_07_get_and_post_fields(self):
        body = multipart_body(field="b", value="posted value")
        response = self.request(
            "POST", "/echo.html?a=from+query",
            [("Content-Type", "multipart/form-data; boundary=%s" % MULTIPART_BOUNDARY),
             ("Content-Length", str(len(body)))],
            body,
        )
        self.assertEqual(response.status, 200)
        self.assertIn(b"b=[posted value]", response.body)
        self.assertIn(b"a=[from", response.body)

    def test_08_empty_and_bare_query_fields(self):
        # "?a&b" style fields without a value used to be stored with a null
        # value pointer and crash the server ; more fields than the parser has
        # room for used to be written past the end of its array.
        cases = {
            "/echo.html?a=": b"a=[]",
            "/echo.html?a": b"a=[]",
            "/echo.html?": b"a=[]",
            "/echo.html?&&a=1&": b"a=[1]",
            "/echo.html?x&a=2&y": b"a=[2]",
            "/echo.html?a&x=1": b"a=[]",
            "/echo.html?=5&a=3": b"a=[3]",
            "/echo.html?a=4&" + "&".join("f%u=%u" % (i, i) for i in range(150)): b"a=[4]",
            "/echo.html?" + "&".join("f%u" % i for i in range(150)) + "&a=5": b"a=[]",
        }
        for target, expected in cases.items():
            response = self.request("GET", target)
            self.assertEqual(response.status, 200, target[:40])
            self.assertIn(expected, response.body, target[:40])
        # a static file with an empty query is still that file
        self.assertEqual(self.request("GET", "/text.txt?").status, 200)

    def test_09_declared_content_type(self):
        # AmmServer_SetResourceHandlerContentType : an extension-less resource
        # can say what it generates , error statuses included
        for target, status in (("/typed", 200), ("/typed?status=400", 400)):
            response = self.request("GET", target)
            self.assertEqual(response.status, status, target)
            self.assertEqual(response.header("Content-Type"), "application/json", target)
            self.assertEqual(response.body, b'{"typed":true}', target)
        head = self.request("HEAD", "/typed")
        self.assertEqual(head.header("Content-Type"), "application/json")
        # a type with a line break was refused , so nothing is injected and the
        # extension still decides
        response = self.request("GET", "/echo.html")
        self.assertEqual(response.header("Content-Type"), "text/html")
        self.assertIsNone(response.header("X-Injected"))

    def test_10_dynamic_resource_in_a_folder_not_on_disk(self):
        # a registered name needs no folder on disk ...
        response = self.request("GET", "/virtual/folder/echo?a=nested")
        self.assertEqual(response.status, 200)
        self.assertIn(b"a=[nested]", response.body)
        # ... but other names in that folder are still not served
        self.assertNotEqual(self.request("GET", "/virtual/folder/other").status, 200)


class TestSessions(ServerTestCase):
    server_binary = TESTSERVICE_BINARY

    @staticmethod
    def _session_cookie(response):
        for cookie in response.header_all("Set-Cookie"):
            return cookie.split(";")[0]
        return None

    def test_01_session_counts_visits(self):
        first = self.request("GET", "/session.html")
        self.assertEqual(first.body, b"visits=1")
        cookie = self._session_cookie(first)
        self.assertIsNotNone(cookie, first.headers)

        second = self.request("GET", "/session.html", [("Cookie", cookie)])
        self.assertEqual(second.body, b"visits=2")
        third = self.request("GET", "/session.html", [("Cookie", cookie)])
        self.assertEqual(third.body, b"visits=3")

    def test_02_sessions_are_separate(self):
        one = self.request("GET", "/session.html")
        two = self.request("GET", "/session.html")
        self.assertEqual(one.body, b"visits=1")
        self.assertEqual(two.body, b"visits=1")
        self.assertNotEqual(self._session_cookie(one), self._session_cookie(two))

    def test_03_forged_session_token_starts_fresh(self):
        name = self._session_cookie(self.request("GET", "/session.html")).split("=")[0]
        response = self.request("GET", "/session.html", [("Cookie", "%s=%s" % (name, "A" * 64))])
        self.assertEqual(response.body, b"visits=1")


class ApiTests:
    """Shared by the epoll and io_uring transports."""

    def test_01_path_capture(self):
        response = self.request("GET", "/item/42")
        self.assertEqual(response.status, 200)
        self.assertEqual(response.body, b'{"item":42}')

    def test_02_unknown_path(self):
        response = self.request("GET", "/nothing/here")
        self.assertEqual(response.status, 404)

    def test_03_bad_capture_is_not_a_match(self):
        for target in ("/item/", "/item/-1", "/item/1.5", "/item/99999999999999999999999"):
            self.assertEqual(self.request("GET", target).status, 404, target)

    def test_04_query_header_and_body(self):
        response = self.request(
            "POST", "/echo?x=1&y=two",
            [("X-Test", 'quote"d'), ("Content-Length", "5")],
            b"hello",
        )
        self.assertEqual(response.status, 200)
        self.assertEqual(response.body, b'{"query":"x=1&y=two","header":"quote\\"d","body":"hello"}')

    def test_05_keepalive_pipeline(self):
        connection = RawConnection(self.port)
        try:
            payload = b"".join(
                make_request("GET", "/item/%u" % number, port=self.port) for number in range(1, 6)
            )
            connection.send(payload)
            for number in range(1, 6):
                response = connection.read_response("GET")
                self.assertEqual(response.body, b'{"item":%u}' % number)
        finally:
            connection.close()

    def test_06_response_larger_than_the_buffer(self):
        body = b"A" * 70000
        response = self.request("POST", "/echo", [("Content-Length", str(len(body)))], body)
        self.assertIn(response.status, (200, 400, 413, 500))

    def test_07_garbage_request(self):
        connection = RawConnection(self.port)
        try:
            connection.send(b"\x00\x01\x02 not http at all\r\n\r\n")
            try:
                connection.read_response("GET")
            except (ConnectionError, ValueError, OSError):
                pass
        finally:
            connection.close()
        self.assertEqual(self.request("GET", "/item/7").body, b'{"item":7}')


class TestApiEpoll(ApiTests, ServerTestCase):
    server_binary = TESTSERVICE_BINARY
    server_first_args = ("-api", "0")


@unittest.skipIf(VALGRIND_LOG_DIR, "valgrind cannot follow io_uring provided buffers ( run_sanitizers.sh covers this )")
class TestApiUring(ApiTests, ServerTestCase):
    server_binary = TESTSERVICE_BINARY
    server_first_args = ("-api", "1")


if __name__ == "__main__":
    unittest.main(verbosity=2)
