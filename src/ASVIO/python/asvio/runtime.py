"""Runtime : everything launch() starts. It prepares the web root , starts the C bridge , takes jobs off it and runs them
on per-event thread pools , and reaps old files."""

import base64
import copy
import hashlib
import io
import json
import math
import mimetypes
import os
import re
import shutil
import signal
import sys
import tempfile
import threading
import time
import traceback
import uuid
import warnings
from concurrent.futures import ThreadPoolExecutor

from . import _bridge
from .blocks import Block
from .components import FILE_KEY, Error

UPDATE_KEY = "__asvio_update__"

FRONTEND = os.path.join(os.path.dirname(os.path.abspath(__file__)), "frontend")
MAX_UPLOAD_MB = int(os.environ.get("ASVIO_MAX_UPLOAD_MB", "64"))
MAX_RESPONSE_MB = int(os.environ.get("ASVIO_MAX_RESPONSE_MB", "2"))
EXTENSION = re.compile(r"^\.[A-Za-z0-9]{1,10}$")   # kept on published files : AmmarServer picks the Content-Type by it
DATA_URL = re.compile(r"data:(image/[A-Za-z0-9.+-]+);base64,([A-Za-z0-9+/=]+)")
SESSION_LIFETIME = 3600   # seconds a session's State values live after its last request
GENERIC_ERROR = ("The upstream ASVIO app has raised an exception but has not enabled verbose error reporting. "
                 "To enable , set show_error=True in launch().")


def _terminate(signum, frame):
    raise KeyboardInterrupt


def _safe_name(name, default="file"):
    name = re.sub(r"[^A-Za-z0-9._ -]", "_", os.path.basename(name or "")).strip(" .")
    return name[:200] or default


def _json_ready(value):
    """numpy scalars/arrays , paths , and non-finite floats ( NaN is not JSON ) made JSON safe."""
    if isinstance(value, float):
        return value if math.isfinite(value) else None
    if isinstance(value, dict):
        return {str(k): _json_ready(v) for k, v in value.items()}
    if isinstance(value, (list, tuple)):
        return [_json_ready(v) for v in value]
    if value is None or isinstance(value, (str, int, bool)):
        return value
    if hasattr(value, "tolist"):
        return _json_ready(value.tolist())
    if isinstance(value, os.PathLike):
        return os.fspath(value)
    return str(value)


class Files:
    """What preprocess/postprocess use for files : the uploads of one job , and the output directory."""

    def __init__(self, runtime, uploads, prefix=""):
        self.runtime = runtime
        self.uploads = uploads   # {"file0": (filename , bytes)}
        self.prefix = prefix     # "example_" files are published once at launch and never reaped

    def resolve(self, value):
        """A wire file value → a local path."""
        ref = value.get(FILE_KEY) if isinstance(value, dict) else None
        if not isinstance(ref, str):
            raise Error("Expected a file , got %r" % (value,))
        if ref.startswith("out:"):
            # a file this server already published ( e.g. an output fed back as an input )
            name = ref[4:]
            path = os.path.join(self.runtime.outputs, name)
            if _safe_name(name) != name or not os.path.isfile(path):
                raise Error("That file is no longer available , please upload it again.")
            return path
        if ref == "empty":
            # 0 byte files are never sent as multipart parts : AmmarServer's parser drops an empty part and every part after it
            filename, content = value.get("name"), b""
        elif ref in self.uploads:
            filename, content = self.uploads[ref]
        else:
            raise Error("Missing upload %r" % ref)
        directory = os.path.join(self.runtime.temp, uuid.uuid4().hex)
        os.makedirs(directory)
        path = os.path.join(directory, _safe_name(filename))
        with open(path, "wb") as f:
            f.write(content)
        return path

    def _reference(self, name, original):
        path = os.path.join(self.runtime.outputs, name)
        return {FILE_KEY: "/asvio/file.json?id=" + name, "name": original, "size": os.path.getsize(path)}

    def publish_path(self, path):
        """Hard link ( or copy ) a file fn returned into the outputs , so its own clean up cannot break the download."""
        path = os.fspath(path)
        if not os.path.isfile(path):
            raise Error("Output file %s does not exist" % path)
        original = os.path.basename(path)
        extension = os.path.splitext(original)[1].lower()
        name = self.prefix + uuid.uuid4().hex + (extension if EXTENSION.match(extension) else "")
        target = os.path.join(self.runtime.outputs, name)
        try:
            os.link(path, target)
        except OSError:
            shutil.copyfile(path, target)
        return self._reference(name, original)

    def publish_writer(self, write, extension, original):
        name = self.prefix + uuid.uuid4().hex + extension
        write(os.path.join(self.runtime.outputs, name))
        return self._reference(name, original)

    def _publish_data_url(self, match):
        """data:image/png;base64,... -> a published file , named after its content so every streamed token reuses it"""
        mime, encoded = match.group(1), match.group(2)
        data = base64.b64decode(encoded)
        extension = (mimetypes.guess_extension(mime) or ".bin").lower()
        name = "inline_" + hashlib.sha1(data).hexdigest()[:24] + (extension if EXTENSION.match(extension) else ".bin")
        path = os.path.join(self.runtime.outputs, name)
        if not os.path.exists(path):
            with open(path + ".part", "wb") as f:
                f.write(data)
            os.replace(path + ".part", path)
        return "/asvio/file.json?id=" + name

    def extract_inline_images(self, value):
        if isinstance(value, str):
            return DATA_URL.sub(self._publish_data_url, value) if "data:" in value else value
        if isinstance(value, dict):
            return {k: self.extract_inline_images(v) for k, v in value.items()}
        if isinstance(value, (list, tuple)):
            return [self.extract_inline_images(v) for v in value]
        return value


class Runtime:

    def __init__(self, blocks, show_error):
        self.blocks = blocks
        self.show_error = show_error
        self.running = False
        self.lock = threading.Lock()
        self.pending = 0
        self.executors = {}
        self.api = {d.api_name: d for d in blocks._dependencies if d.api_name}
        self.own_temp = False
        self.sessions = {}   # session id -> {"used": time , "values": {State component id: value}}

    # ------------------------------------------------------------------ setup
    def _prepare_directories(self, favicon_path):
        self.webroot = tempfile.mkdtemp(prefix="asvio_www_")
        for name in os.listdir(FRONTEND):
            shutil.copyfile(os.path.join(FRONTEND, name), os.path.join(self.webroot, name))
        if favicon_path:
            icon = "favicon" + os.path.splitext(favicon_path)[1].lower()
            shutil.copyfile(favicon_path, os.path.join(self.webroot, icon))
            index = os.path.join(self.webroot, "index.html")
            with open(index) as f:
                page = f.read().replace('<link rel="icon" href="favicon.svg" type="image/svg+xml">', '<link rel="icon" href="%s">' % icon)
            with open(index, "w") as f:
                f.write(page)
        self.outputs = os.path.join(self.webroot, "asvio_out")
        os.makedirs(self.outputs)
        examples = Files(self, {}, prefix="example_")
        for block in self.blocks._blocks.values():
            if hasattr(block, "prepare"):
                block.prepare(examples)
        with open(os.path.join(self.webroot, "config.json"), "w") as f:
            json.dump(_json_ready(self.blocks.config()), f)
        for route, (kind, source) in self.blocks._extra_files.items():
            target = os.path.join(self.webroot, route)
            if not route or ".." in route.split("/") or os.path.exists(target) or route.split("/")[0] in ("asvio", "asvio_out"):
                shutil.rmtree(self.webroot, ignore_errors=True)
                raise ValueError("asvio: /%s cannot be added , it is empty , escapes the web root , or ASVIO uses it" % route)
            os.makedirs(os.path.dirname(target), exist_ok=True)
            if kind == "file":
                shutil.copyfile(source, target)
            else:
                with open(target, "w") as f:
                    json.dump(_json_ready(source()), f)
        # Uploads go where gradio puts them , so the scripts' own GRADIO_TEMP_DIR reapers keep working.
        # The web root must NOT be in there : those reapers delete anything older than a few minutes.
        self.temp = os.environ.get("ASVIO_TEMP_DIR") or os.environ.get("GRADIO_TEMP_DIR")
        if self.temp:
            os.makedirs(self.temp, exist_ok=True)
        else:
            self.temp = tempfile.mkdtemp(prefix="asvio_tmp_")
            self.own_temp = True

    def _quiet_server_output(self):
        """AmmarServer prints every request on stdout/stderr from C. Point fds 1 and 2 at a log file , and keep
        Python's sys.stdout / sys.stderr on the terminal , so print() and tracebacks still show there."""
        self.log = os.environ.get("ASVIO_SERVER_LOG") or os.path.join(tempfile.gettempdir(), "asvio_%d_server.log" % os.getpid())
        sys.stdout.flush()
        sys.stderr.flush()
        terminal_out, terminal_err = os.dup(1), os.dup(2)
        log = os.open(self.log, os.O_WRONLY | os.O_CREAT | os.O_APPEND, 0o600)
        os.dup2(log, 1)
        os.dup2(log, 2)
        os.close(log)
        sys.stdout = io.TextIOWrapper(os.fdopen(terminal_out, "wb", 0), write_through=True, line_buffering=True)
        sys.stderr = io.TextIOWrapper(os.fdopen(terminal_err, "wb", 0), write_through=True, line_buffering=True)

    def start(self, server_name, server_port, favicon_path=None):
        host = server_name or os.environ.get("GRADIO_SERVER_NAME") or "127.0.0.1"
        explicit = server_port is not None or "GRADIO_SERVER_PORT" in os.environ
        port = int(server_port or os.environ.get("GRADIO_SERVER_PORT") or 7860)
        self._prepare_directories(favicon_path)
        if os.environ.get("ASVIO_SERVER_LOG") != "-":
            self._quiet_server_output()
        for candidate in ([port] if explicit else range(port, port + 100)):
            if _bridge.start("ASVIO", host, candidate, self.webroot, MAX_UPLOAD_MB, MAX_RESPONSE_MB):
                port = candidate
                break
        else:
            shutil.rmtree(self.webroot, ignore_errors=True)
            raise OSError("asvio: cannot start a server on %s:%s" % (host, port))
        self.running = True
        threading.Thread(target=self._dispatch, name="asvio-dispatch", daemon=True).start()
        if self.blocks.delete_cache:
            threading.Thread(target=self._reap, args=self.blocks.delete_cache, name="asvio-reaper", daemon=True).start()
        self.url = "http://%s:%d" % ("127.0.0.1" if host == "0.0.0.0" else host, port)
        print("* Running on local URL:  %s" % self.url)
        if getattr(self, "log", None):
            print("* AmmarServer log: %s" % self.log)

    def block(self):
        if threading.current_thread() is threading.main_thread():
            # kill / systemd / timeout send SIGTERM : clean up like Ctrl+C instead of leaving the web root behind
            signal.signal(signal.SIGTERM, _terminate)
        try:
            while self.running and _bridge.running():
                time.sleep(0.5)
        except KeyboardInterrupt:
            print("Keyboard interruption in main thread... closing server.")
        finally:
            self.stop()

    def stop(self):
        if not self.running:
            return
        self.running = False
        _bridge.stop()
        for executor in self.executors.values():
            executor.shutdown(wait=False, cancel_futures=True)
        shutil.rmtree(self.webroot, ignore_errors=True)
        if self.own_temp:
            shutil.rmtree(self.temp, ignore_errors=True)

    # ------------------------------------------------------------------ jobs
    def _executor(self, dependency):
        if dependency.index not in self.executors:
            limit = dependency.concurrency_limit
            if limit == "default":
                limit = self.blocks._default_concurrency_limit
            self.executors[dependency.index] = ThreadPoolExecutor(max_workers=limit or 64, thread_name_prefix="asvio-%d" % dependency.index)
        return self.executors[dependency.index]

    def _dispatch(self):
        forgotten = time.time()
        while self.running:
            job = _bridge.next_job(500)
            if time.time() - forgotten > 60:
                self._forget_idle_sessions()
                forgotten = time.time()
            if job is None:
                continue
            dependency = self.api.get(job.api)
            if dependency is None:
                _bridge.finish(job.id, "error", json.dumps({"error": "There is no api_name %s" % job.api}))
                continue
            with self.lock:
                if self.blocks._max_size is not None and self.pending >= self.blocks._max_size:
                    _bridge.finish(job.id, "error", json.dumps({"error": "Queue is full , please try again later."}))
                    continue
                self.pending += 1
            self._executor(dependency).submit(self._run, dependency, job)

    # ------------------------------------------------------------------ sessions
    def _session(self, session_id):
        with self.lock:
            session = self.sessions.setdefault(session_id, {"used": 0, "values": {}})
            session["used"] = time.time()
            return session["values"]

    def _forget_idle_sessions(self):
        cutoff = time.time() - SESSION_LIFETIME
        with self.lock:
            for session_id in [s for s, v in self.sessions.items() if v["used"] < cutoff]:
                del self.sessions[session_id]

    def _state(self, values, component):
        if component._id not in values:
            values[component._id] = copy.deepcopy(component.value)
        return values[component._id]

    def _output(self, component, value, files, values):
        """One output value -> wire. State is kept in the session instead , updates become {"__asvio_update__": props}."""
        if isinstance(value, Block):
            value = dict(value.constructor_args, __type__="update")   # e.g. gr.Button(visible=True) returned from fn
        if isinstance(value, dict) and value.get("__type__") == "update":
            props = {k: v for k, v in value.items() if k != "__type__" and (v is not None or k == "value") and k not in ("render", "elem_id", "elem_classes")}
            if component.kind == "state":
                if "value" in props:
                    values[component._id] = props["value"]
                return None
            if "value" in props:
                props["value"] = component.postprocess(props["value"], files)
            return {UPDATE_KEY: props}
        if component.kind == "state":
            values[component._id] = value
            return None
        return component.postprocess(value, files)

    def _payload(self, dependency, result, files, session):
        outputs = dependency.outputs
        if len(outputs) == 0:
            values = []
        elif len(outputs) == 1:
            values = [result]
        else:
            # gradio's validate_outputs : too few values is an exception ( hidden unless show_error ) , too many a warning
            values = list(result) if isinstance(result, (list, tuple)) else [result]
            if len(values) < len(outputs):
                raise ValueError("A function (%s) didn't return enough output values (needed: %d, returned: %d)."
                                 % (dependency.api_name, len(outputs), len(values)))
            if len(values) > len(outputs):
                warnings.warn("A function (%s) returned too many output values (needed: %d, returned: %d). "
                              "Ignoring the extra values." % (dependency.api_name, len(outputs), len(values)))
                values = values[:len(outputs)]
        wire = [self._output(component, value, files, session) for component, value in zip(outputs, values)]
        return json.dumps({"outputs": _json_ready(wire)})

    def _run(self, dependency, job):
        try:
            _bridge.update(job.id, "running")
            if _bridge.cancelled(job.id):
                _bridge.finish(job.id, "cancelled")
                return
            files = Files(self, {field: (filename, content) for field, filename, content in job.files})
            data = json.loads(job.data)
            if not isinstance(data, list) or len(data) != len(dependency.inputs):
                raise Error("%s takes %d inputs , got %s" % (dependency.api_name, len(dependency.inputs), job.data[:200]))
            session = self._session(job.session)
            args = [self._state(session, component) if component.kind == "state" else component.preprocess(value, files)
                    for component, value in zip(dependency.inputs, data)]
            result = dependency.fn(*args)
            if hasattr(result, "__next__") and hasattr(result, "close"):
                payload = json.dumps({"outputs": None})
                for value in result:
                    if _bridge.cancelled(job.id):
                        result.close()
                        _bridge.finish(job.id, "cancelled")
                        return
                    payload = self._payload(dependency, value, files, session)
                    _bridge.update(job.id, "running", payload)
                _bridge.finish(job.id, "done", payload)
            else:
                _bridge.finish(job.id, "done", self._payload(dependency, result, files, session))
        except Error as e:
            _bridge.finish(job.id, "error", json.dumps({"error": str(e.message)}))
        except Exception as e:
            if type(e).__name__ == "Error" and type(e).__module__.startswith("gradio"):
                # a real gradio.Error , e.g. raised by a helper library written for gradio ( deepseek_vl2's wrap_gen_fn )
                _bridge.finish(job.id, "error", json.dumps({"error": str(getattr(e, "message", e))}))
                return
            traceback.print_exc()
            _bridge.finish(job.id, "error", json.dumps({"error": "%s: %s" % (type(e).__name__, e) if self.show_error else GENERIC_ERROR}))
        finally:
            with self.lock:
                self.pending -= 1

    # ------------------------------------------------------------------ files
    def _reap(self, every, age):
        """delete_cache=(every , age) : every `every` seconds delete files older than `age` seconds."""
        while self.running:
            time.sleep(every)
            cutoff = time.time() - age
            for directory in (self.outputs, self.temp):
                for root, _, names in os.walk(directory):
                    for name in names:
                        if name.startswith("example_"):
                            continue
                        path = os.path.join(root, name)
                        try:
                            if os.path.getmtime(path) < cutoff:
                                os.remove(path)
                        except OSError:
                            pass
