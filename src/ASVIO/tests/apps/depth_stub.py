#!/usr/bin/env python3
"""depthanythingv3/server.py with the model replaced by a stub : same gradio usage , same output encoding.
The "depth" is a 16-bit gradient keyed to the input image's size and mean colour , so tests can recompute it."""

import argparse
import os
import sys
import tempfile

import numpy as np
from PIL import Image

if "GRADIO_TEMP_DIR" not in os.environ:
    os.environ["GRADIO_TEMP_DIR"] = tempfile.mkdtemp(prefix="da3_server_")

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "python"))
import asvio as gr  # noqa: E402

parser = argparse.ArgumentParser()
parser.add_argument("--ip", type=str, default="127.0.0.1")
parser.add_argument("--port", type=int, default=7861)
args = parser.parse_args()


def fake_depth(image_path):
    rgb = np.asarray(Image.open(image_path).convert("RGB"), dtype=np.float32)
    h, w = rgb.shape[:2]
    depth = np.add.outer(np.arange(h, dtype=np.float32), np.arange(w, dtype=np.float32)) + rgb.mean()
    dmin, dmax = float(depth.min()), float(depth.max())
    if dmax - dmin < 1e-6:
        dmax = dmin + 1.0
    return (((depth - dmin) / (dmax - dmin)) * 65535.0).astype(np.uint16)


def predict_depth(image_path):
    if image_path is None:
        raise gr.Error("No image provided.")
    depth_uint16 = fake_depth(image_path)
    out_fd, out_path = tempfile.mkstemp(suffix=".png", prefix="da3_depth_", dir=os.environ.get("GRADIO_TEMP_DIR"))
    os.close(out_fd)
    Image.fromarray(depth_uint16).save(out_path)   # the real server uses cv2.imwrite , both write a 16-bit PNG
    return out_path


demo = gr.Interface(
    fn=predict_depth,
    inputs=gr.Image(type="filepath", label="Input image"),
    outputs=gr.File(label="16-bit depth PNG"),
    title="Depth Anything V3",
    description="Upload an image to get a 16-bit grayscale depth map (PNG).",
    flagging_mode="never",
    delete_cache=(600, 600),
)

if __name__ == "__main__":
    demo.launch(server_name=args.ip, server_port=args.port, share=False)
