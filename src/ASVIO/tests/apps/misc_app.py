#!/usr/bin/env python3
"""Covers the phase-2 components and runtime paths the depth stub does not : Textbox , Number , JSON , numpy/pil
Images , multiple Files , generators , exceptions , per-event concurrency."""

import argparse
import os
import sys
import time

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "python"))
import asvio as gr  # noqa: E402

parser = argparse.ArgumentParser()
parser.add_argument("--port", type=int, default=7862)
args = parser.parse_args()


def echo(text, n):
    return text.upper(), {"n": n, "type": type(n).__name__, "nan": float("nan"), "np": np.float32(1.5), "arr": np.arange(3)}


def invert(array):
    return Image.fromarray(255 - array)


def count(n):
    for i in range(1, n + 1):
        time.sleep(0.2)
        yield "".join(str(k % 10) for k in range(1, i + 1))


def boom():
    raise ValueError("secret internals")


def sizes(paths):
    return [[os.path.basename(p), os.path.getsize(p)] for p in paths]


with gr.Blocks(title="misc") as demo:
    text = gr.Textbox(label="text")
    number = gr.Number(label="n", precision=0)
    text_out = gr.Textbox(label="text out")
    json_out = gr.JSON(label="json out")
    image_in = gr.Image(type="numpy", label="image in")
    image_out = gr.Image(type="pil", format="png", label="image out")
    files_in = gr.File(file_count="multiple", label="files")
    gr.Button("echo").click(echo, [text, number], [text_out, json_out], api_name="echo")
    gr.Button("invert").click(invert, image_in, image_out, api_name="invert")
    gr.Button("count").click(count, number, text_out, api_name="count")
    gr.Button("boom").click(boom, None, text_out, api_name="boom")
    gr.Button("sizes").click(sizes, files_in, json_out, api_name="sizes")

if __name__ == "__main__":
    demo.queue().launch(server_port=args.port)
