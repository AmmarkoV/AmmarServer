#!/usr/bin/env python3
"""Phase 3 stubs , one per server shape : --app dinov3 ( Blocks , multiple typed Files , JSON ) , sam3 ( Interface with
pil/png Images , article , add_route , add_static ) , sam3d ( two Images , Numbers , Model3D ) , collide ( a bad route )."""

import argparse
import os
import sys
import tempfile

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "python"))
import asvio as gr  # noqa: E402

parser = argparse.ArgumentParser()
parser.add_argument("--app", required=True)
parser.add_argument("--port", type=int, default=7863)
args = parser.parse_args()


def embed(files):
    return [[os.path.basename(f), list(Image.open(f).size)] for f in files or []]


def size(image):
    return list(image.size)


def segment(image, prompt):
    if image is None or not prompt.strip():
        return None
    w, h = image.size
    rgb = np.zeros((h, w, 3), np.uint8)
    rgb[2:6, 3:9] = [len(prompt.split(",")), 255, 255]
    return Image.fromarray(rgb), {"width": w, "height": h, "mode": image.mode}


def reconstruct(image, mask, instance_id, seed):
    if image is None or mask is None:
        return None, None, None
    out = tempfile.mkdtemp(dir=os.environ.get("GRADIO_TEMP_DIR"))
    glb = os.path.join(out, "mesh.glb")
    with open(glb, "wb") as f:
        f.write(b"glTF" + bytes([instance_id % 256]))
    ply = os.path.join(out, "splat.ply")
    with open(ply, "w") as f:
        f.write("ply %s %d\n" % (mask.mode, int((np.asarray(mask.convert("RGBA"))[:, :, 3] > 0).sum())))
    return glb, ply, {"instance": instance_id, "seed": seed, "types": [type(instance_id).__name__, type(seed).__name__]}


if args.app == "dinov3":
    with gr.Blocks(title="DINOv3 descriptors", delete_cache=(600, 600)) as demo:
        gr.Markdown("# DINOv3 descriptors\nUpload images.")
        files_in = gr.File(file_count="multiple", file_types=["image"], label="Images")
        out = gr.JSON(label="Result")
        gr.Button("Embed").click(fn=embed, inputs=files_in, outputs=out, api_name="embed")
        gr.Button("Size").click(fn=size, inputs=gr.Image(type="pil", label="one"), outputs=out, api_name="size")
    demo.queue()
elif args.app == "sam3":
    demo = gr.Interface(fn=segment,
                        inputs=[gr.Image(type="pil", label="Input Image"), gr.Textbox(label="Prompt", placeholder="e.g. person")],
                        outputs=[gr.Image(type="pil", format="png", label="Encoded"), gr.JSON(label="Instances")],
                        title="SAM3 stub", description="- R = ids\n- G/B = mask",
                        article='<p><a href="/client.py" download>download client.py</a> &middot; <a href="/version">/version</a></p>')
    demo.add_route("/version", lambda: {"version": "stub-1", "client_py_available": True})
    demo.add_static("/client.py", __file__)
elif args.app == "sam3d":
    demo = gr.Interface(fn=reconstruct,
                        inputs=[gr.Image(type="pil", label="Image"), gr.Image(type="pil", image_mode=None, format="png", label="Mask"),
                                gr.Number(value=0, precision=0, label="Instance"), gr.Number(value=42, precision=0, label="Seed")],
                        outputs=[gr.Model3D(label="Mesh"), gr.Model3D(label="Splat"), gr.JSON(label="Pose")],
                        title="SAM3D stub")
elif args.app == "collide":
    demo = gr.Interface(fn=size, inputs=gr.Image(type="pil"), outputs="json")
    demo.add_route("/config.json", lambda: {})

demo.launch(server_name="127.0.0.1", server_port=args.port, show_error=True)
