#!/usr/bin/env python3
"""Phase 2 checks : the stub apps run as real processes and are driven through asvio.client and raw HTTP.

Run from anywhere : python3 src/ASVIO/tests/test_interface.py
"""

import glob
import json
import os
import shutil
import signal
import socket
import subprocess
import sys
import tempfile
import time
import unittest
import urllib.request

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "python"))
from asvio.client import AppError, Client, handle_file  # noqa: E402


def free_port():
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


class App:
    """Runs tests/apps/<name>.py on a free port until stop()."""

    def __init__(self, name, env=None):
        self.port = free_port()
        self.process = subprocess.Popen([sys.executable, os.path.join(HERE, "apps", name), "--port", str(self.port)],
                                        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                                        env=dict(os.environ, **(env or {})))
        line = self.process.stdout.readline()
        if "Running on" not in line:
            self.process.kill()
            raise RuntimeError("app did not start: %r %s" % (line, self.process.stderr.read()))
        self.url = line.split()[-1]
        self.process.stdout.readline()   # the AmmarServer log line

    def get(self, path):
        with urllib.request.urlopen(self.url + path, timeout=30) as r:
            return json.loads(r.read())

    def stop(self, signum=signal.SIGINT):
        self.process.send_signal(signum)
        try:
            return self.process.wait(10)
        except subprocess.TimeoutExpired:
            self.process.kill()
            return None
        finally:
            self.process.stdout.close()
            self.process.stderr.close()


class DepthStubTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.work = tempfile.mkdtemp(prefix="asvio_depth_test_")
        cls.server_temp = os.path.join(cls.work, "server_temp")
        cls.app = App("depth_stub.py", {"GRADIO_TEMP_DIR": cls.server_temp})
        cls.client = Client(cls.app.url, download_files=os.path.join(cls.work, "downloads"), verbose=False)

    @classmethod
    def tearDownClass(cls):
        cls.rc = cls.app.stop()
        shutil.rmtree(cls.work, ignore_errors=True)

    def test_16bit_png_round_trip_is_byte_identical(self):
        image = os.path.join(self.work, "photo.png")
        Image.fromarray(np.random.default_rng(1).integers(0, 255, (240, 320, 3), dtype=np.uint8)).save(image)
        result = self.client.predict(handle_file(image))
        self.assertTrue(result.endswith(".png") and os.path.isfile(result), result)
        written = max(glob.glob(os.path.join(self.server_temp, "da3_depth_*.png")), key=os.path.getmtime)
        with open(result, "rb") as a, open(written, "rb") as b:
            self.assertEqual(a.read(), b.read())
        with open(result, "rb") as f:
            header = f.read(26)
        self.assertEqual((header[24], header[25]), (16, 0))   # IHDR : 16-bit grayscale
        depth = np.asarray(Image.open(result))
        self.assertEqual((depth.shape, int(depth.min()), int(depth.max())), ((240, 320), 0, 65535))
        # the upload landed in GRADIO_TEMP_DIR under its own name , like gradio
        self.assertTrue(glob.glob(os.path.join(self.server_temp, "*", "photo.png")))

    def test_large_upload(self):
        image = os.path.join(self.work, "big.png")
        Image.fromarray(np.random.default_rng(2).integers(0, 255, (1500, 2000, 3), dtype=np.uint8)).save(image)
        self.assertGreater(os.path.getsize(image), 8 * 1024 * 1024)
        depth = np.asarray(Image.open(self.client.predict(handle_file(image))))
        self.assertEqual(depth.shape, (1500, 2000))

    def test_gr_error_reaches_client(self):
        with self.assertRaises(AppError) as caught:
            self.client.predict(None)
        self.assertEqual(str(caught.exception), "No image provided.")

    def test_unknown_api(self):
        with self.assertRaises(AppError):
            self.client.predict(None, api_name="/nope")

    def test_view_api_and_page(self):
        self.assertEqual(self.client.view_api(print_info=False),
                         ["client.predict(image_path, api_name='/predict') -> file(16-bit depth PNG)"])
        with urllib.request.urlopen(self.app.url + "/") as r:
            self.assertIn(b"asvio.js", r.read())


class MiscAppTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.work = tempfile.mkdtemp(prefix="asvio_misc_test_")
        cls.app = App("misc_app.py")
        cls.client = Client(cls.app.url, download_files=cls.work, verbose=False)

    @classmethod
    def tearDownClass(cls):
        cls.app.stop()
        shutil.rmtree(cls.work, ignore_errors=True)

    def test_text_number_json(self):
        text, info = self.client.predict("héllo <b>", 2.6, api_name="/echo")
        self.assertEqual(text, "HÉLLO <B>")
        self.assertEqual(info, {"n": 3, "type": "int", "nan": None, "np": 1.5, "arr": [0, 1, 2]})

    def test_numpy_in_pil_png_out(self):
        array = np.random.default_rng(3).integers(0, 255, (50, 60, 3), dtype=np.uint8)
        path = os.path.join(self.work, "in.jpg")
        Image.fromarray(array).save(path, quality=95)
        loaded = np.asarray(Image.open(path).convert("RGB"))
        result = self.client.predict(handle_file(path), api_name="/invert")
        self.assertTrue(result.endswith(".png"), result)
        self.assertTrue(np.array_equal(np.asarray(Image.open(result)), 255 - loaded))

    def test_multiple_files(self):
        paths = []
        for i, size in enumerate((0, 10, 70000)):
            paths.append(os.path.join(self.work, "f%d.bin" % i))
            with open(paths[-1], "wb") as f:
                f.write(os.urandom(size))
        self.assertEqual(self.client.predict([handle_file(p) for p in paths], api_name="/sizes"),
                         [["f0.bin", 0], ["f1.bin", 10], ["f2.bin", 70000]])

    def test_generator_streams_every_yield(self):
        job = self.client.submit(5, api_name="/count")
        seen, seq = [], 0
        while True:
            reply = self.app.get("/asvio/poll.json?job=%s&seq=%d" % (job, seq))
            seq = reply["seq"]
            if reply["data"]:
                seen.append(reply["data"]["outputs"][0])
            if reply["status"] == "done":
                break
        self.assertEqual(seen[-1], "12345")
        self.assertGreaterEqual(len(set(seen)), 4)   # most yields seen one by one , not just the end

    def test_cancel_running_generator(self):
        job = self.client.submit(50, api_name="/count")
        time.sleep(0.7)
        self.app.get("/asvio/cancel.json?job=" + job)
        started = time.time()
        reply = {"status": ""}
        seq = 0
        while reply["status"] != "cancelled":
            reply = self.app.get("/asvio/poll.json?job=%s&seq=%d" % (job, seq))
            seq = reply["seq"]
        self.assertLess(time.time() - started, 2)
        self.assertLess(len(reply["data"]["outputs"][0]), 10)

    def test_one_job_per_event_at_a_time(self):
        first = self.client.submit(3, api_name="/count")
        second = self.client.submit(3, api_name="/count")
        time.sleep(0.3)
        self.assertEqual(self.app.get("/asvio/poll.json?job=%s&seq=0&wait=0" % second)["status"], "queued")
        self.assertEqual(self.client.result(first), "123")
        self.assertEqual(self.client.result(second), "123")

    def test_exception_text_hidden_without_show_error(self):
        with self.assertRaises(AppError) as caught:
            self.client.predict(api_name="/boom")
        self.assertNotIn("secret", str(caught.exception))
        self.assertIn("show_error=True", str(caught.exception))


class ShutdownTest(unittest.TestCase):

    def check_exit_cleans_up(self, signum):
        webroots = lambda: set(glob.glob(os.path.join(tempfile.gettempdir(), "asvio_www_*")))
        before = webroots()
        app = App("misc_app.py")
        own = webroots() - before
        self.assertEqual(len(own), 1)
        self.assertEqual(app.stop(signum), 0)
        self.assertFalse(own & webroots())

    def test_ctrl_c_exits_and_cleans_up(self):
        self.check_exit_cleans_up(signal.SIGINT)

    def test_sigterm_exits_and_cleans_up(self):
        self.check_exit_cleans_up(signal.SIGTERM)


if __name__ == "__main__":
    unittest.main(verbosity=2)
