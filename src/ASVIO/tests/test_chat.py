#!/usr/bin/env python3
"""Phase 4 checks : the deepseekvlm / qwenvlm shaped chat stub ( tests/apps/chat_app.py ) driven the way the deepseek
client drives it ( keyword arguments , State kept per session ) plus raw polling for streaming and cancel.

Run from anywhere : python3 src/ASVIO/tests/test_chat.py
"""

import json
import os
import subprocess
import sys
import tempfile
import shutil
import time
import unittest
import urllib.error
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "python"))
sys.path.insert(0, HERE)
from asvio.client import AppError, Client, handle_file  # noqa: E402
from test_interface import App, free_port  # noqa: E402

EXAMPLE = os.path.join(HERE, "apps", "chat_example.png")
ARGS = dict(chatbot=[], temperature=0.3, top_p=0.5, max_length_tokens=99, repetition_penalty=1.1,
            max_context_length_tokens=4096, greek_translation=False)


class ChatApp(App):
    def __init__(self, *flags):
        self.port = free_port()
        self.process = subprocess.Popen([sys.executable, os.path.join(HERE, "apps", "chat_app.py"), "--port", str(self.port)] + list(flags),
                                        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        line = self.process.stdout.readline()
        if "Running on" not in line:
            self.process.kill()
            raise RuntimeError("app did not start: %r %s" % (line, self.process.stderr.read()))
        self.url = line.split()[-1]
        self.process.stdout.readline()

    def fetch(self, path):
        try:
            with urllib.request.urlopen(self.url + path, timeout=30) as r:
                return r.status, r.headers.get("Content-Type"), r.read()
        except urllib.error.HTTPError as e:
            return e.code, None, b""


def answer(text, images=1, model="model-a"):
    return "You said %s with %d image%s (top_p=0.5, t=0.3, tokens=99, greek=False, %s)" % (text, images, "" if images == 1 else "s", model)


class TuplesChatTest(unittest.TestCase):
    """deepseekvlm : [[user , bot]] turns"""

    @classmethod
    def setUpClass(cls):
        cls.work = tempfile.mkdtemp(prefix="asvio_chat_test_")
        cls.app = ChatApp("--format", "tuples")

    @classmethod
    def tearDownClass(cls):
        cls.app.stop()
        shutil.rmtree(cls.work, ignore_errors=True)

    def client(self):
        return Client(self.app.url, download_files=self.work, verbose=False)

    def ask(self, client, text, images=(EXAMPLE,), **extra):
        # exactly the deepseek client.py sequence
        client.predict(api_name="/reset_state")
        client.predict(input_images=[handle_file(i) for i in images] or None, input_text=text, api_name="/transfer_input")
        return client.predict(**dict(ARGS, **extra), api_name="/predict")

    def test_deepseek_client_sequence(self):
        chatbot, status = self.ask(self.client(), "hello")
        self.assertEqual(chatbot[0][1], answer("hello"))   # deepseek client.py reads result[0][0][1]
        self.assertEqual(status, "Generate: Success")

    def test_transfer_input_returns_updates(self):
        client = self.client()
        self.assertEqual(client.predict(input_images=None, input_text="x", api_name="/transfer_input"),
                         ({"value": "", "__type__": "update"}, {"value": None, "__type__": "update"}, {"visible": True, "__type__": "update"}))

    def test_inline_image_becomes_a_published_png(self):
        chatbot, _ = self.ask(self.client(), "look")
        user = chatbot[0][0]
        self.assertNotIn("data:", user)
        url = user.split('src="')[1].split('"')[0]
        self.assertRegex(url, r"^/asvio/file.json\?id=inline_[0-9a-f]{24}\.png$")
        status, ctype, body = self.app.fetch(url)
        self.assertEqual((status, ctype, body[:8]), (200, "image/png", b"\x89PNG\r\n\x1a\n"))

    def test_state_is_per_session(self):
        a, b = self.client(), self.client()
        a.predict(input_images=None, input_text="from a", api_name="/transfer_input")
        b.predict(input_images=None, input_text="from b", api_name="/transfer_input")
        self.assertEqual(a.predict(**ARGS, api_name="/predict")[0][0][1], answer("from a", 0))
        self.assertEqual(b.predict(**ARGS, api_name="/predict")[0][0][1], answer("from b", 0))

    def test_tokens_stream(self):
        client = self.client()
        client.predict(input_images=None, input_text="stream me", api_name="/transfer_input")
        job = client.submit(**ARGS, api_name="/predict")
        seen, seq = [], 0
        while True:
            reply = json.loads(self.app.fetch("/asvio/poll.json?job=%s&seq=%d" % (job, seq))[2])
            seq = reply["seq"]
            if reply["data"] and reply["data"]["outputs"]:
                seen.append(reply["data"]["outputs"][0][0][1])
            if reply["status"] == "done":
                break
        self.assertGreater(len(set(seen)), 5)
        self.assertEqual(seen[-1], answer("stream me", 0))
        self.assertTrue(all(answer("stream me", 0).startswith(s) for s in seen))

    def test_stop_cancels_a_running_predict(self):
        client = self.client()
        client.predict(input_images=None, input_text="slow", api_name="/transfer_input")
        job = client.submit(**ARGS, api_name="/predict")
        time.sleep(1.2)
        self.app.fetch("/asvio/cancel.json?job=" + job)
        started, seq, reply = time.time(), 0, {"status": ""}
        while reply["status"] not in ("cancelled", "done"):
            reply = json.loads(self.app.fetch("/asvio/poll.json?job=%s&seq=%d" % (job, seq))[2])
            seq = reply["seq"]
        self.assertEqual(reply["status"], "cancelled")
        self.assertLess(time.time() - started, 1.5)

    def test_gradio_error_from_a_helper_library(self):
        with self.assertRaises(AppError) as caught:
            self.ask(self.client(), "explode", images=())
        self.assertEqual(str(caught.exception), "Failed to generate text: the model fell over")

    def test_dropdown_rejects_an_unknown_choice(self):
        with self.assertRaises(AppError) as caught:
            self.ask(self.client(), "hi", images=(), model_select_dropdown="nope")
        self.assertEqual(str(caught.exception), "Value: 'nope' is not in the list of choices: ['model-a', 'model-b']")

    def test_omitted_parameter_takes_its_default_and_bad_names_are_refused(self):
        chatbot, _ = self.ask(self.client(), "defaults", images=())
        self.assertTrue(chatbot[0][1].endswith("model-a)"))
        with self.assertRaises(TypeError):
            self.client().predict(chatbot=[], no_such_parameter=1, api_name="/predict")

    def test_reset_state_four_values_for_three_outputs(self):
        self.assertEqual(self.client().predict(api_name="/reset_state"), ([], None))

    def test_delete_last_conversation(self):
        self.assertEqual(self.client().predict(chatbot=[["a", "b"], ["c", "d"]], api_name="/delete_last_conversation"),
                         ([["a", "b"]], "Delete Done"))

    def test_api_names_like_gradio(self):
        names = [d["api_name"] for d in self.client().config["dependencies"]]
        self.assertEqual(names, ["/preview_images", "/transfer_input", "/predict", "/transfer_input_1", "/predict_1",
                                 "/reset_state", "/reset_textbox", "/retry", "/delete_last_conversation", "/lambda"])

    def test_examples_and_favicon(self):
        config = self.client().config
        examples = [c for c in config["components"].values() if c["type"] == "examples"][0]["props"]
        first, missing = examples["rows"]
        self.assertRegex(first[0][0]["__asvio_file__"], r"id=example_[0-9a-f]{32}\.png$")
        self.assertEqual((first[1], first[2]), ("<b>example</b> html", "Is the worker wearing gloves?"))
        self.assertIsNone(missing[0])
        self.assertEqual(config["title"], "VLM - Platform")
        page = self.app.fetch("/")[2].decode()
        self.assertIn('<link rel="icon" href="favicon.png">', page)
        self.assertEqual(self.app.fetch("/favicon.png")[1], "image/png")


class MessagesChatTest(unittest.TestCase):
    """qwenvlm : {"role": , "content": } messages"""

    @classmethod
    def setUpClass(cls):
        cls.app = ChatApp("--format", "messages")

    @classmethod
    def tearDownClass(cls):
        cls.app.stop()

    def test_messages_round_trip(self):
        client = Client(self.app.url, verbose=False)
        client.predict(input_images=None, input_text="hi", api_name="/transfer_input")
        chatbot, status = client.predict(**ARGS, api_name="/predict")
        self.assertEqual(chatbot, [{"role": "user", "content": "hi"}, {"role": "assistant", "content": answer("hi", 0)}])
        # the Chatbot value goes back in as an input unchanged
        self.assertEqual(client.predict(chatbot=chatbot, api_name="/delete_last_conversation"), ([], "Delete Done"))


class ReaperTest(unittest.TestCase):

    def test_delete_cache_keeps_examples(self):
        app = ChatApp("--reap")
        try:
            client = Client(app.url, verbose=False)
            client.predict(input_images=[handle_file(EXAMPLE)], input_text="x", api_name="/transfer_input")
            chatbot, _ = client.predict(**ARGS, api_name="/predict")
            inline = chatbot[0][0].split('src="')[1].split('"')[0]
            examples = [c for c in client.config["components"].values() if c["type"] == "examples"][0]["props"]
            example = examples["rows"][0][0][0]["__asvio_file__"]
            self.assertEqual(app.fetch(inline)[0], 200)
            time.sleep(3.5)
            self.assertEqual(app.fetch(inline)[0], 404)    # reaped by delete_cache=(1, 1)
            self.assertEqual(app.fetch(example)[0], 200)   # examples are published once and never reaped
        finally:
            app.stop()


if __name__ == "__main__":
    unittest.main(verbosity=2)
