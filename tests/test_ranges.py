"""Byte-range and HEAD behavior (download resume)."""

import unittest

from .harness import RANGE_BYTES, ServerTestCase, make_request


class RangeAssertions(ServerTestCase):
    def assert_full_get(self, response):
        self.assertEqual(response.status, 200)
        self.assertEqual(response.body, RANGE_BYTES)
        self.assertEqual(response.header("content-length"), str(len(RANGE_BYTES)))
        self.assertEqual(response.header("accept-ranges"), "bytes")

    def assert_range(self, response, start, end, expected_body):
        self.assertEqual(response.status, 206, response.header("content-range"))
        self.assertEqual(response.header("content-range"), "bytes %d-%d/%d" % (start, end, len(RANGE_BYTES)))
        self.assertEqual(response.header("content-length"), str(len(expected_body)))
        self.assertEqual(response.body, expected_body)

    def test_01_full_get(self):
        self.assert_full_get(self.request("GET", "/range.bin"))

    def test_02_closed_range(self):
        self.assert_range(self.request("GET", "/range.bin", [("Range", "bytes=10-19")]), 10, 19, RANGE_BYTES[10:20])

    def test_03_open_ended_range(self):
        self.assert_range(self.request("GET", "/range.bin", [("Range", "bytes=10-")]), 10, 35, RANGE_BYTES[10:])

    def test_04_suffix_range(self):
        self.assert_range(self.request("GET", "/range.bin", [("Range", "bytes=-10")]), 26, 35, RANGE_BYTES[26:])

    def test_05_oversized_suffix_is_whole_file(self):
        self.assert_range(self.request("GET", "/range.bin", [("Range", "bytes=-999")]), 0, 35, RANGE_BYTES)

    def test_06_single_byte(self):
        self.assert_range(self.request("GET", "/range.bin", [("Range", "bytes=0-0")]), 0, 0, RANGE_BYTES[:1])

    def test_07_end_past_eof_is_clamped(self):
        self.assert_range(self.request("GET", "/range.bin", [("Range", "bytes=30-100")]), 30, 35, RANGE_BYTES[30:])

    def test_08_unsatisfiable_start(self):
        for spec in ("bytes=36-", "bytes=36-40", "bytes=-0"):
            with self.subTest(spec=spec):
                response = self.request("GET", "/range.bin", [("Range", spec)])
                self.assertEqual(response.status, 416)
                self.assertEqual(response.header("content-range"), "bytes */36")
                self.assertIn(b"416", response.body)

    def test_09_invalid_ranges_are_ignored(self):
        for spec in ("bytes=20-10", "bytes=0-1,3-4", "items=0-1", "bytes=x-y", "bytes="):
            with self.subTest(spec=spec):
                self.assert_full_get(self.request("GET", "/range.bin", [("Range", spec)]))

    def test_10_empty_file(self):
        response = self.request("GET", "/empty.bin")
        self.assertEqual(response.status, 200)
        self.assertEqual(response.header("content-length"), "0")
        self.assertEqual(response.body, b"")

        response = self.request("GET", "/empty.bin", [("Range", "bytes=0-")])
        self.assertEqual(response.status, 416)
        self.assertEqual(response.header("content-range"), "bytes */0")

    def test_11_missing_file_with_range(self):
        response = self.request("GET", "/missing.bin", [("Range", "bytes=0-")])
        self.assertEqual(response.status, 404)
        self.assertIsNone(response.header("content-range"))

    def test_12_head_with_range(self):
        response = self.request("HEAD", "/range.bin", [("Range", "bytes=10-19")])
        self.assertEqual(response.status, 206)
        self.assertEqual(response.header("content-range"), "bytes 10-19/36")
        self.assertEqual(response.header("content-length"), "10")
        self.assertEqual(response.body, b"")

    def test_13_head_full(self):
        response = self.request("HEAD", "/range.bin")
        self.assertEqual(response.status, 200)
        self.assertEqual(response.header("content-length"), "36")
        self.assertEqual(response.body, b"")

    def test_14_keepalive_after_range(self):
        connection = self.connect()
        try:
            connection.send(make_request("GET", "/range.bin", [("Range", "bytes=0-0")],
                                         host="auto", port=self.port))
            first = connection.read_response("GET")
            self.assertEqual(first.status, 206)
            self.assertEqual(first.body, RANGE_BYTES[:1])

            connection.send(make_request("GET", "/range.bin", [("Connection", "close")],
                                         host="auto", port=self.port))
            second = connection.read_response("GET")
            self.assertEqual(second.status, 200)
            self.assertEqual(second.body, RANGE_BYTES)
        finally:
            connection.close()

    def test_15_range_keeps_last_modified(self):
        response = self.request("GET", "/range.bin", [("Range", "bytes=0-0")])
        self.assertEqual(response.status, 206)
        self.assertIsNotNone(response.header("last-modified"))


class TestRangesCached(RangeAssertions):
    server_config = {"Caching": "On"}


class TestRangesDisk(RangeAssertions):
    server_config = {"Caching": "Off"}


if __name__ == "__main__":
    unittest.main(verbosity=2)
