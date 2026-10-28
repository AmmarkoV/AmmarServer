#!/usr/bin/env python3
"""Phase 4 stub : the deepseekvlm / qwenvlm UI , event wiring and helper behaviour with a fake streaming "model".
--format tuples is deepseek ( [[user , bot]] , wrap_gen_fn raising a real gradio.Error , reset_state returning
4 values for 3 outputs ) , --format messages is qwen ( [{"role": , "content": }] ). --reap sets delete_cache=(1, 1)."""

import argparse
import base64
import io
import os
import sys
import time
from functools import wraps

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "python"))
sys.path.insert(0, os.path.join(HERE, "compat"))   # a stand-in "gradio" module , for the gradio.Error a helper library raises
import asvio as gr  # noqa: E402
import gradio  # noqa: E402

parser = argparse.ArgumentParser()
parser.add_argument("--port", type=int, default=7864)
parser.add_argument("--format", choices=("tuples", "messages"), default="tuples")
parser.add_argument("--reap", action="store_true")
args = parser.parse_args()
TUPLES = args.format == "tuples"
EXAMPLE = os.path.join(HERE, "chat_example.png")
if not os.path.exists(EXAMPLE):
    Image.new("RGB", (40, 30), (200, 80, 20)).save(EXAMPLE)


def wrap_gen_fn(gen_fn):
    # deepseek_vl2.serve.app_modules.gradio_utils.wrap_gen_fn : turns failures into a *gradio* Error
    @wraps(gen_fn)
    def wrapped(*a, **kw):
        try:
            yield from gen_fn(*a, **kw)
        except gradio.Error:
            raise
        except Exception as e:
            raise gradio.Error(f"Failed to generate text: {e}") from e
    return wrapped


def pil_to_base64(image):
    buffer = io.BytesIO()
    image.save(buffer, format="PNG")
    return '<img src="data:image/png;base64,%s" alt="user upload" />' % base64.b64encode(buffer.getvalue()).decode()


def preview_images(files):
    if files is None:
        return []
    return [f.name for f in files]


def transfer_input(input_text, input_images):
    return input_text, input_images, gr.update(value=""), gr.update(value=None), gr.Button(visible=True)


@wrap_gen_fn
def predict(text, images, chatbot, history, top_p, temperature, repetition_penalty, max_length_tokens,
            max_context_length_tokens, model_select_dropdown, greek_translation):
    history[:] = []
    if text == "":
        yield chatbot, history, "Empty context."
        return
    if text == "explode":
        raise RuntimeError("the model fell over")
    pil_images = [Image.open(f.name).convert("RGB") for f in (images or [])]
    shown = "".join(pil_to_base64(i) for i in pil_images) + text
    answer = "You said %s with %d image%s (top_p=%s, t=%s, tokens=%s, greek=%s, %s)" % (
        text, len(pil_images), "" if len(pil_images) == 1 else "s", top_p, temperature, max_length_tokens, greek_translation, model_select_dropdown)
    chatbot = chatbot + ([[shown, ""]] if TUPLES else [{"role": "user", "content": shown}, {"role": "assistant", "content": ""}])
    words = answer.split(" ")
    for i in range(len(words)):
        time.sleep(0.05 if text != "slow" else 0.5)
        partial = " ".join(words[:i + 1])
        if TUPLES:
            chatbot[-1][1] = partial
        else:
            chatbot[-1]["content"] = partial
        yield chatbot, history, "Generating..."
    history.extend([text, answer])
    yield chatbot, history, "Generate: Success"


def retry(text, images, chatbot, history, *rest):
    if len(history) == 0:
        yield chatbot, history, "Empty context"
        return
    chatbot = chatbot[:-1] if TUPLES else chatbot[:-2]
    answer, text = history.pop(), history.pop()
    yield from predict(text, images, chatbot, history, *rest)


def reset_state():
    return [], [], None, "Reset Done"   # deepseek_vl2 returns 4 values for 3 outputs ; gradio warns and drops the last


def reset_textbox():
    return gradio_update(value=""), ""


def gradio_update(**kwargs):
    # what a helper library written for gradio returns : gradio.update() is a plain dict
    return dict(kwargs, __type__="update")


def delete_last_conversation(chatbot, history):
    return (chatbot[:-1] if TUPLES else chatbot[:-2]), history[:-2], "Delete Done"


with gr.Blocks(theme=gr.themes.Soft(), delete_cache=(1, 1) if args.reap else None) as demo:
    history = gr.State([])
    input_text = gr.State()
    input_images = gr.State()
    with gr.Row():
        gr.HTML("<h1>Chat stub</h1>")
        status_display = gr.Markdown("Success", elem_id="status_display")
    gr.Markdown("Special tokens: `<image>`")
    with gr.Row(equal_height=True):
        with gr.Column(scale=4):
            with gr.Row():
                chatbot = gr.Chatbot(elem_id="chatbot", height=400, label="Chatbot", show_share_button=True)
            with gr.Row():
                with gr.Column(scale=4):
                    text_box = gr.Textbox(show_label=False, placeholder="Enter text", container=False)
                with gr.Column(min_width=70):
                    submitBtn = gr.Button("Send")
                with gr.Column(min_width=70):
                    cancelBtn = gr.Button("Stop")
            with gr.Row():
                emptyBtn = gr.Button("New Conversation")
                retryBtn = gr.Button("Regenerate")
                delLastBtn = gr.Button("Remove Last Turn")
        with gr.Column():
            upload_images = gr.Files(file_types=["image"], show_label=True, label="Files")
            gallery = gr.Gallery(columns=[3], height="200px", show_label=True, label="Gallery")
            upload_images.change(preview_images, inputs=upload_images, outputs=gallery)
            with gr.Tab(label="Parameter Setting"):
                top_p = gr.Slider(minimum=-0, maximum=1.0, value=0.9, step=0.05, interactive=True, label="Top-p")
                temperature = gr.Slider(minimum=0, maximum=1.0, value=0.1, step=0.1, interactive=True, label="Temperature")
                repetition_penalty = gr.Slider(minimum=0.0, maximum=2.0, value=1.1, step=0.1, interactive=True, label="Repetition penalty")
                max_length_tokens = gr.Slider(minimum=0, maximum=4096, value=2048, step=8, interactive=True, label="Max Generation Tokens")
                max_context_length_tokens = gr.Slider(minimum=0, maximum=8192, value=4096, step=128, interactive=True, label="Max History Tokens")
                model_select_dropdown = gr.Dropdown(label="Select Model", choices=["model-a", "model-b"], multiselect=False, value="model-a", interactive=True)
                greek_translation = gr.Checkbox(label="Ελληνικά", value=True)
                show_images = gr.HTML(visible=False)
    gr.Examples(examples=[[[EXAMPLE], "<b>example</b> html", "Is the worker wearing gloves?"],
                          [[os.path.join(HERE, "missing.jpg")], "", "This example's image is missing"]],
                inputs=[upload_images, show_images, text_box])
    gr.Markdown("A footer description.")

    input_widgets = [input_text, input_images, chatbot, history, top_p, temperature, repetition_penalty,
                     max_length_tokens, max_context_length_tokens, model_select_dropdown, greek_translation]
    output_widgets = [chatbot, history, status_display]
    transfer_input_args = dict(fn=transfer_input, inputs=[text_box, upload_images],
                               outputs=[input_text, input_images, text_box, upload_images, submitBtn], show_progress=True)
    predict_args = dict(fn=predict, inputs=input_widgets, outputs=output_widgets, show_progress=True)
    predict_events = [text_box.submit(**transfer_input_args).then(**predict_args),
                      submitBtn.click(**transfer_input_args).then(**predict_args)]
    emptyBtn.click(reset_state, outputs=output_widgets, show_progress=True)
    emptyBtn.click(fn=reset_textbox, inputs=[], outputs=[text_box, status_display])
    retryBtn.click(fn=retry, inputs=input_widgets, outputs=output_widgets, show_progress=True)
    delLastBtn.click(delete_last_conversation, [chatbot, history], output_widgets, show_progress=True)
    cancelBtn.click(lambda: "Stop Done", [], [status_display], cancels=predict_events)

demo.title = "VLM - Platform"
demo.queue(max_size=10).launch(favicon_path=os.path.join(HERE, "chat_example.png"), inbrowser=False, share=False,
                               server_name="127.0.0.1", server_port=args.port, root_path="")
