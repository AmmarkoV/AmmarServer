"""ammServ.conf parsing and the settings it controls."""

import unittest

from .harness import (
    DEFAULT_CONFIG,
    MULTIPART_BOUNDARY,
    RawConnection,
    Server,
    chunk_encode,
    make_request,
    multipart_body,
)


class TestConfiguration(unittest.TestCase):
    def _start(self, config=None, config_files=None, explicit_conf=True):
        server = Server(config=config, config_files=config_files, explicit_conf=explicit_conf)
        self.addCleanup(server.stop)
        return server

    @staticmethod
    def _startup_log(server):
        with open(server.log_path, "rb") as log:
            return log.read().decode("utf-8", "replace")

    @staticmethod
    def _chunked_echo(server, value):
        connection = RawConnection(server.port)
        try:
            body = chunk_encode(multipart_body(value=value))
            connection.send(make_request(
                "POST", "/formtest.html",
                [("Content-Type", "multipart/form-data; boundary=%s" % MULTIPART_BOUNDARY),
                 ("Transfer-Encoding", "chunked"),
                 ("Connection", "close")],
                body, port=server.port))
            return connection.read_response("POST")
        finally:
            connection.close()

    def test_01_enable_post_on(self):
        server = self._start({"EnablePost": "On"})
        response = self._chunked_echo(server, "postenabled")
        self.assertEqual(response.status, 200)
        self.assertIn(b"postenabled", response.body)

    def test_02_enable_post_off(self):
        server = self._start({"EnablePost": "Off"})
        response = self._chunked_echo(server, "postdisabled")
        self.assertNotIn(b"GOT A POST USERNAME", response.body)

    def test_03_max_post_transaction_size(self):
        server = self._start({"EnablePost": "On", "MaxPostTransactionSize": 512})
        response = self._chunked_echo(server, "x" * 2048)
        self.assertEqual(response.status, 413)

    def test_04_comments_tabs_and_unknown_keys(self):
        config = {
            "Timeout": 5,
            "KeepAlive": "On",
            "Caching": "On",
            "EnablePost": "On",
            "MaxPostTransactionSize": 4096,
            "SomeUnknownFutureOption": "whatever",
        }
        server = self._start(config)
        connection = RawConnection(server.port)
        try:
            connection.send(make_request("GET", "/range.bin", [("Connection", "close")], port=server.port))
            response = connection.read_response("GET")
            self.assertEqual(response.status, 200)
        finally:
            connection.close()

    def test_05_keepalive_off_closes(self):
        server = self._start({"KeepAlive": "Off"})
        connection = RawConnection(server.port)
        try:
            connection.send(make_request("GET", "/range.bin", port=server.port))
            response = connection.read_response("GET")
            self.assertEqual(response.status, 200)
            self.assertEqual((response.header("connection") or "").lower(), "close")
            connection.socket.settimeout(2.0)
            self.assertEqual(connection.socket.recv(1), b"")
        finally:
            connection.close()

    def test_06_keepalive_on_allows_two_requests(self):
        server = self._start({"KeepAlive": "On"})
        connection = RawConnection(server.port)
        try:
            for index in range(2):
                headers = [("Connection", "close")] if index == 1 else []
                connection.send(make_request("GET", "/range.bin", headers, port=server.port))
                response = connection.read_response("GET")
                self.assertEqual(response.status, 200)
        finally:
            connection.close()

    def test_07_ammserv_conf_wins_over_legacy(self):
        server = self._start(
            config_files={
                "ammServ.conf": dict(DEFAULT_CONFIG, EnablePost="On"),
                "ammarServer.conf": dict(DEFAULT_CONFIG, EnablePost="Off"),
            },
            explicit_conf=False,
        )
        response = self._chunked_echo(server, "ammservconf")
        self.assertIn(b"GOT A POST USERNAME", response.body)
        self.assertIn(b"ammservconf", response.body)

    def test_08_legacy_ammarserver_conf_is_loaded(self):
        server = self._start(
            config_files={
                "ammarServer.conf": dict(DEFAULT_CONFIG, EnablePost="On"),
            },
            explicit_conf=False,
        )
        response = self._chunked_echo(server, "legacyconf")
        self.assertIn(b"GOT A POST USERNAME", response.body)
        self.assertIn(b"legacyconf", response.body)

    def test_09_listen_is_overridden_by_command_line(self):
        # The command-line port always wins; this asserts the config parser does
        # not break startup when Listen is present too.
        server = self._start({"Listen": 65000})
        connection = RawConnection(server.port)
        try:
            connection.send(make_request("GET", "/range.bin", [("Connection", "close")], port=server.port))
            self.assertEqual(connection.read_response("GET").status, 200)
        finally:
            connection.close()

    def test_10_documented_options_do_not_warn_but_typos_do(self):
        documented = {
            "KeepAliveTimeout": 15,
            "MaxKeepAliveRequests": 100,
            "StartServers": 2,
            "MinSpareThreads": 25,
            "MaxSpareThreads": 75,
            "ThreadLimit": 64,
            "MaxClients": 150,
            "FileCompression": "On",
            "DynamicContentCompression": "On",
            "ReallocToSaveMoreThanThisNumberOfBytesWithCompression": 4096,
            "DefaultType": "text/plain",
            "ServerAdmin": "webmaster@localhost",
            "DocumentRoot": "public_html/",
            "ServerTokens": "OS",
            "ServerSignature": "On",
            "DirectoryListing": "On",
        }
        settings = dict(DEFAULT_CONFIG)
        settings.update(documented)

        server = self._start(config_files={"default.conf": settings}, explicit_conf=False)
        startup_log = self._startup_log(server)
        self.assertNotIn("unknown key", startup_log)

        connection = RawConnection(server.port)
        try:
            connection.send(make_request("GET", "/index.html", [("Connection", "close")], port=server.port))
            self.assertEqual(connection.read_response("GET").status, 200)
        finally:
            connection.close()

        typo_server = self._start({"SomeUnknownFutureOption": "whatever"})
        self.assertIn("unknown key `SomeUnknownFutureOption`", self._startup_log(typo_server))


if __name__ == "__main__":
    unittest.main(verbosity=2)
