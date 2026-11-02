"""Conditional request behavior (If-Modified-Since, If-Unmodified-Since, ETag)."""

import email.utils
import unittest

from .harness import FIXED_MTIME, RANGE_BYTES, ServerTestCase, make_request


class ConditionalAssertions(ServerTestCase):
    def _ims(self, when):
        return [("If-Modified-Since", email.utils.formatdate(when, usegmt=True))]

    def _ius(self, when):
        return [("If-Unmodified-Since", email.utils.formatdate(when, usegmt=True))]

    def test_01_last_modified_is_present(self):
        response = self.request("GET", "/range.bin")
        self.assertEqual(response.status, 200)
        self.assertEqual(response.header("last-modified"), email.utils.formatdate(FIXED_MTIME, usegmt=True))

    def test_02_if_modified_since_exact(self):
        response = self.request("GET", "/range.bin", self._ims(FIXED_MTIME))
        self.assertEqual(response.status, 304)
        self.assertEqual(response.body, b"")
        self.assertIsNone(response.header("content-length"))
        self.assertEqual(response.header("last-modified"), email.utils.formatdate(FIXED_MTIME, usegmt=True))

    def test_03_if_modified_since_older(self):
        response = self.request("GET", "/range.bin", self._ims(FIXED_MTIME - 100))
        self.assertEqual(response.status, 200)

    def test_04_if_modified_since_newer(self):
        response = self.request("GET", "/range.bin", self._ims(FIXED_MTIME + 100))
        self.assertEqual(response.status, 304)

    def test_05_if_modified_since_garbage(self):
        response = self.request("GET", "/range.bin", [("If-Modified-Since", "not a date")])
        self.assertEqual(response.status, 200)

    def test_06_head_if_modified_since(self):
        response = self.request("HEAD", "/range.bin", self._ims(FIXED_MTIME))
        self.assertEqual(response.status, 304)
        self.assertEqual(response.body, b"")

    def test_07_if_unmodified_since_older(self):
        response = self.request("GET", "/range.bin", self._ius(FIXED_MTIME - 100))
        self.assertEqual(response.status, 412)

    def test_08_if_unmodified_since_exact_and_newer(self):
        for when in (FIXED_MTIME, FIXED_MTIME + 100):
            with self.subTest(when=when):
                self.assertEqual(self.request("GET", "/range.bin", self._ius(when)).status, 200)

    def test_09_etag_takes_precedence_over_ims(self):
        first = self.request("GET", "/range.bin")
        etag = first.header("etag")

        # A non-matching If-None-Match suppresses If-Modified-Since. This works
        # even on the disk-served profile, which does not emit an ETag at all.
        response = self.request(
            "GET", "/range.bin",
            [("If-None-Match", '"definitely-not-the-etag"')] + self._ims(FIXED_MTIME))
        self.assertEqual(response.status, 200)

        if etag is not None:
            response = self.request("GET", "/range.bin", [("If-None-Match", etag)])
            self.assertEqual(response.status, 304)

    def test_10_304_then_normal_get_on_same_connection(self):
        connection = self.connect()
        try:
            connection.send(make_request(
                "GET", "/range.bin",
                self._ims(FIXED_MTIME) + [("Connection", "keep-alive")],
                port=self.port))
            first = connection.read_response("GET")
            self.assertEqual(first.status, 304)

            connection.send(make_request("GET", "/range.bin", [("Connection", "close")], port=self.port))
            second = connection.read_response("GET")
            self.assertEqual(second.status, 200)
            self.assertEqual(second.body, RANGE_BYTES)
        finally:
            connection.close()


class TestConditionalsCached(ConditionalAssertions):
    server_config = {"Caching": "On"}


class TestConditionalsDisk(ConditionalAssertions):
    server_config = {"Caching": "Off"}


if __name__ == "__main__":
    unittest.main(verbosity=2)
