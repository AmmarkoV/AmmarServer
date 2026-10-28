#!/usr/bin/env python3
"""Phase 1 checks for libasvio.so : this test plays the Python worker and talks HTTP to the real server.

Run from anywhere : python3 src/ASVIO/tests/test_bridge.py
"""

import json
import os
import shutil
import socket
import sys
import tempfile
import threading
import time
import unittest
import urllib.error
import urllib.request
import uuid

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "python"))
from asvio import _bridge  # noqa: E402

MAX_RESPONSE_MB = 1
BASE = None
WEBROOT = None


def free_port():
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


def setUpModule():
    global BASE, WEBROOT
    WEBROOT = tempfile.mkdtemp(prefix="asvio_test_")
    os.makedirs(os.path.join(WEBROOT, "asvio_out"))
    with open(os.path.join(WEBROOT, "index.html"), "w") as f:
        f.write("<html>asvio</html>")
    port = free_port()
    assert _bridge.start("ASVIOTest", "127.0.0.1", port, WEBROOT, 16, MAX_RESPONSE_MB)
    BASE = "http://127.0.0.1:%d" % port
    time.sleep(0.3)


def tearDownModule():
    _bridge.stop()
    shutil.rmtree(WEBROOT, ignore_errors=True)


def request(path, body=None, content_type=None, timeout=30):
    req = urllib.request.Request(BASE + path, data=body, headers={"Content-Type": content_type} if content_type else {})
    try:
        with urllib.request.urlopen(req, timeout=timeout) as r:
            return r.status, r.headers.get("Content-Type"), r.read()
    except urllib.error.HTTPError as e:
        return e.code, e.headers.get("Content-Type"), e.read()


def multipart(fields, files=()):
    boundary = uuid.uuid4().hex
    out = b""
    for name, value in fields.items():
        out += ("--%s\r\nContent-Disposition: form-data; name=\"%s\"\r\n\r\n%s\r\n" % (boundary, name, value)).encode()
    for name, filename, data in files:
        out += ("--%s\r\nContent-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n"
                "Content-Type: application/octet-stream\r\n\r\n" % (boundary, name, filename)).encode() + data + b"\r\n"
    return out + ("--%s--\r\n" % boundary).encode(), "multipart/form-data; boundary=" + boundary


def run(api="/predict", session="s1", data="[1]", files=()):
    fields = {"api": api, "data": data}
    if session:
        fields["session"] = session
    body, ctype = multipart(fields, files)
    return request("/asvio/run.json", body, ctype)


def submit(**kwargs):
    status, _, body = run(**kwargs)
    assert status == 200, (status, body)
    return json.loads(body)["job"]


def poll(job_id, seq=0, wait=5):
    status, ctype, body = request("/asvio/poll.json?job=%s&seq=%d&wait=%d" % (job_id, seq, wait))
    return status, json.loads(body)


class BridgeTest(unittest.TestCase):

    def setUp(self):
        # every test starts with nothing queued
        while _bridge.next_job(50) is not None:
            pass

    def take(self, job_id):
        job = _bridge.next_job(2000)
        self.assertIsNotNone(job)
        self.assertEqual(job.id, job_id)
        return job

    def test_static_and_json_types(self):
        status, ctype, body = request("/index.html")
        self.assertEqual((status, body), (200, b"<html>asvio</html>"))
        status, ctype, body = run()
        self.assertEqual(status, 200)
        self.assertEqual(ctype, "application/json")

    def test_streamed_updates_then_finish(self):
        job_id = submit(api="/predict", session="abc", data='["hi", 2]')
        job = self.take(job_id)
        self.assertEqual((job.api, job.session, job.data, job.files), ("/predict", "abc", b'["hi", 2]', []))

        self.assertTrue(_bridge.update(job_id, "running", '{"outputs":["a"]}'))
        status, reply = poll(job_id, 0)
        self.assertEqual((status, reply["seq"], reply["status"], reply["data"]), (200, 1, "running", {"outputs": ["a"]}))

        # a poll past the latest seq must wait for the next change
        threading.Timer(0.4, lambda: _bridge.finish(job_id, "done", '{"outputs":["ab"]}')).start()
        started = time.time()
        status, reply = poll(job_id, 1)
        self.assertGreaterEqual(time.time() - started, 0.35)
        self.assertEqual((reply["seq"], reply["status"], reply["data"]), (2, "done", {"outputs": ["ab"]}))

        # finished : no more updates , polls answer at once
        self.assertFalse(_bridge.update(job_id, "running", '{"outputs":[]}'))
        self.assertFalse(_bridge.finish(job_id, "done", '{"outputs":[]}'))
        started = time.time()
        status, reply = poll(job_id, 2)
        self.assertLess(time.time() - started, 0.5)
        self.assertEqual(reply["data"], {"outputs": ["ab"]})

    def test_poll_times_out_unchanged(self):
        job_id = submit()
        started = time.time()
        status, reply = poll(job_id, 0, wait=1)
        self.assertGreaterEqual(time.time() - started, 0.9)
        self.assertEqual((status, reply["seq"], reply["status"], reply["data"]), (200, 0, "queued", None))
        _bridge.finish(self.take(job_id).id, "done", "{}")

    def test_cancel_queued(self):
        job_id = submit()
        self.assertEqual(request("/asvio/cancel.json?job=" + job_id)[0], 200)
        status, reply = poll(job_id, 0)
        self.assertEqual((reply["status"], reply["seq"]), ("cancelled", 1))
        self.assertIsNone(_bridge.next_job(200))

    def test_cancel_running(self):
        job_id = submit()
        self.take(job_id)
        self.assertFalse(_bridge.cancelled(job_id))
        self.assertEqual(request("/asvio/cancel.json?job=" + job_id)[0], 200)
        self.assertTrue(_bridge.cancelled(job_id))
        self.assertTrue(_bridge.finish(job_id, "cancelled"))
        self.assertEqual(poll(job_id, 0)[1]["status"], "cancelled")

    def test_upload_8mb_intact(self):
        big, small = os.urandom(8 * 1024 * 1024), b"\x00\x01\x02 tiny"
        job_id = submit(data='[{"__file__":"file0"},{"__file__":"file1"}]',
                        files=[("file0", "big.png", big), ("file1", "small name.bin", small)])
        job = self.take(job_id)
        self.assertEqual([(f, n) for f, n, _ in job.files], [("file0", "big.png"), ("file1", "small name.bin")])
        self.assertEqual(job.files[0][2], big)
        self.assertEqual(job.files[1][2], small)
        _bridge.finish(job_id, "done", "{}")

    def test_file_route(self):
        png = b"\x89PNG\r\n\x1a\n" + os.urandom(1000)
        with open(os.path.join(WEBROOT, "asvio_out", "abc_1.png"), "wb") as f:
            f.write(png)
        status, ctype, body = request("/asvio/file.json?id=abc_1.png")
        self.assertEqual((status, ctype, body), (200, "image/png", png))
        os.remove(os.path.join(WEBROOT, "asvio_out", "abc_1.png"))
        self.assertEqual(request("/asvio/file.json?id=abc_1.png")[0], 404)
        for bad in ("..%2Findex.html", ".hidden", "a%2Fb.png", ""):
            self.assertEqual(request("/asvio/file.json?id=" + bad)[0], 400, bad)

    def test_rejected_requests(self):
        self.assertEqual(request("/asvio/poll.json?job=" + "0" * 32)[0], 404)
        self.assertEqual(request("/asvio/cancel.json?job=" + "0" * 32)[0], 404)
        self.assertEqual(request("/asvio/poll.json")[0], 400)
        body, ctype = multipart({"api": "/predict"})
        self.assertEqual(request("/asvio/run.json", body, ctype)[0], 400)
        self.assertEqual(run(api="/pre dict")[0], 400)
        self.assertEqual(run(session="a\"b")[0], 400)
        self.assertIsNone(_bridge.next_job(100))

    def test_output_larger_than_poll_buffer(self):
        job_id = submit()
        self.take(job_id)
        _bridge.finish(job_id, "done", json.dumps({"outputs": ["x" * (MAX_RESPONSE_MB * 1024 * 1024)]}))
        status, reply = poll(job_id, 0)
        self.assertEqual(status, 500)
        self.assertIn("files", reply["error"])

    def test_20_concurrent_jobs(self):
        results = {}

        def client(i):
            job_id = submit(session="c%d" % i, data="[%d]" % i)
            seq = 0
            while True:
                status, reply = poll(job_id, seq)
                seq = reply["seq"]
                if reply["status"] == "done":
                    results[i] = reply["data"]["outputs"][0]
                    return

        def worker():
            for _ in range(20):
                job = _bridge.next_job(10000)
                value = json.loads(job.data)[0]
                _bridge.update(job.id, "running", json.dumps({"outputs": [-1]}))
                _bridge.finish(job.id, "done", json.dumps({"outputs": [value * 10]}))

        threads = [threading.Thread(target=client, args=(i,)) for i in range(20)] + [threading.Thread(target=worker)]
        for t in threads:
            t.start()
        for t in threads:
            t.join(60)
        self.assertEqual(results, {i: i * 10 for i in range(20)})


if __name__ == "__main__":
    unittest.main(verbosity=2)
