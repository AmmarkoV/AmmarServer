#!/usr/bin/env python3
"""Phase 3 checks : the dinov3 / sam3 / sam3d shaped stubs in tests/apps/blocks_app.py , driven through asvio.client.

Run from anywhere : python3 src/ASVIO/tests/test_blocks.py
"""

import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
import urllib.request

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "python"))
sys.path.insert(0, HERE)
from asvio.client import AppError, Client, handle_file  # noqa: E402
from test_interface import App, free_port  # noqa: E402


class BlocksApp(App):
    def __init__(self, app, env=None):
        self.port = free_port()
        self.process = subprocess.Popen([sys.executable, os.path.join(HERE, "apps", "blocks_app.py"), "--app", app, "--port", str(self.port)],
                                        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=dict(os.environ, **(env or {})))
        line = self.process.stdout.readline()
        if "Running on" not in line:
            self.process.kill()
            raise RuntimeError("app did not start: %r %s" % (line, self.process.stderr.read()))
        self.url = line.split()[-1]
        self.process.stdout.readline()


def save(path, array, **kwargs):
    Image.fromarray(array).save(path, **kwargs)
    return path


class Dinov3ShapeTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.work = tempfile.mkdtemp(prefix="asvio_dinov3_test_")
        cls.app = BlocksApp("dinov3")
        cls.client = Client(cls.app.url, download_files=cls.work, verbose=False)

    @classmethod
    def tearDownClass(cls):
        cls.app.stop()
        shutil.rmtree(cls.work, ignore_errors=True)

    def test_multiple_images(self):
        paths = [save(os.path.join(self.work, "a%d.%s" % (i, ext)), np.zeros((10 + i, 20, 3), np.uint8))
                 for i, ext in enumerate(("png", "jpg", "webp"))]
        self.assertEqual(self.client.predict([handle_file(p) for p in paths], api_name="/embed"),
                         [["a0.png", [20, 10]], ["a1.jpg", [20, 11]], ["a2.webp", [20, 12]]])

    def test_file_types_rejects_like_gradio(self):
        text = os.path.join(self.work, "notes.txt")
        with open(text, "w") as f:
            f.write("hello")
        with self.assertRaises(AppError) as caught:
            self.client.predict([handle_file(text)], api_name="/embed")
        self.assertEqual(str(caught.exception), "Invalid file type. Please upload a file that is one of these formats: ['image']")

    def test_exif_orientation_applied_for_pil(self):
        path = os.path.join(self.work, "rotated.jpg")
        image = Image.fromarray(np.zeros((10, 30, 3), np.uint8))
        exif = image.getexif()
        exif[274] = 6   # rotate 90 on display
        image.save(path, exif=exif)
        self.assertEqual(self.client.predict(handle_file(path), api_name="/size"), [10, 30])

    def test_config_layout(self):
        config = self.client.config
        self.assertEqual(config["title"], "DINOv3 descriptors")
        types = [config["components"][str(child["id"])]["type"] for child in config["layout"]["children"]]
        # Button("Size") is created before the Image passed to its click() , so it comes first , as in gradio
        self.assertEqual(types, ["markdown", "file", "json", "button", "button", "image"])
        self.assertEqual([d["api_name"] for d in config["dependencies"]], ["/embed", "/size"])


class Sam3ShapeTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.work = tempfile.mkdtemp(prefix="asvio_sam3_test_")
        cls.app = BlocksApp("sam3")
        cls.client = Client(cls.app.url, download_files=cls.work, verbose=False)

    @classmethod
    def tearDownClass(cls):
        cls.app.stop()
        shutil.rmtree(cls.work, ignore_errors=True)

    def test_png_output_exact_and_json(self):
        image = save(os.path.join(self.work, "in.png"), np.zeros((12, 16, 4), np.uint8))
        mask, info = self.client.predict(handle_file(image), "a, b, c")
        self.assertTrue(mask.endswith(".png"))
        expected = np.zeros((12, 16, 3), np.uint8)
        expected[2:6, 3:9] = [3, 255, 255]
        self.assertTrue(np.array_equal(np.asarray(Image.open(mask)), expected))
        self.assertEqual(info, {"width": 16, "height": 12, "mode": "RGB"})   # RGBA upload converted , image_mode="RGB"

    def test_too_few_outputs_is_an_error(self):
        image = save(os.path.join(self.work, "in2.png"), np.zeros((4, 4, 3), np.uint8))
        with self.assertRaises(AppError) as caught:
            self.client.predict(handle_file(image), "  ")
        self.assertIn("didn't return enough output values (needed: 2, returned: 1)", str(caught.exception))

    def test_add_route_and_add_static(self):
        with urllib.request.urlopen(self.app.url + "/version") as r:
            self.assertEqual(json.loads(r.read()), {"version": "stub-1", "client_py_available": True})
        with urllib.request.urlopen(self.app.url + "/client.py") as r, open(os.path.join(HERE, "apps", "blocks_app.py"), "rb") as f:
            self.assertEqual(r.read(), f.read())

    def test_article_and_description_in_config(self):
        values = [c["props"].get("value") for c in self.client.config["components"].values()]
        self.assertIn("# SAM3 stub", values)
        self.assertIn("- R = ids\n- G/B = mask", values)
        self.assertTrue(any(v and "download client.py" in v for v in values))


class Sam3dShapeTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.work = tempfile.mkdtemp(prefix="asvio_sam3d_test_")
        cls.app = BlocksApp("sam3d", {"GRADIO_TEMP_DIR": os.path.join(cls.work, "server")})
        cls.client = Client(cls.app.url, download_files=os.path.join(cls.work, "dl"), verbose=False)

    @classmethod
    def tearDownClass(cls):
        cls.app.stop()
        shutil.rmtree(cls.work, ignore_errors=True)

    def test_two_images_numbers_model3d(self):
        image = save(os.path.join(self.work, "img.jpg"), np.zeros((20, 30, 3), np.uint8))
        rgba = np.zeros((20, 30, 4), np.uint8)
        rgba[0:5, 0:7, 3] = 255
        mask = save(os.path.join(self.work, "mask.png"), rgba)
        glb, ply, pose = self.client.predict(handle_file(image), handle_file(mask), 2.4, 7)
        self.assertEqual((os.path.basename(glb), os.path.basename(ply)), ("mesh.glb", "splat.ply"))
        with open(glb, "rb") as f:
            self.assertEqual(f.read(), b"glTF\x02")
        with open(ply) as f:
            self.assertEqual(f.read(), "ply RGBA 35\n")   # image_mode=None kept the alpha channel
        self.assertEqual(pose, {"instance": 2, "seed": 7, "types": ["int", "int"]})   # precision=0 -> int

    def test_model3d_in_config(self):
        types = sorted(c["type"] for c in self.client.config["components"].values())
        self.assertEqual(types.count("model3d"), 2)


class RouteCollisionTest(unittest.TestCase):

    def test_route_over_an_asvio_file_fails_at_launch(self):
        result = subprocess.run([sys.executable, os.path.join(HERE, "apps", "blocks_app.py"), "--app", "collide", "--port", str(free_port())],
                                capture_output=True, text=True, timeout=30)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("/config.json cannot be added", result.stderr)


if __name__ == "__main__":
    unittest.main(verbosity=2)
