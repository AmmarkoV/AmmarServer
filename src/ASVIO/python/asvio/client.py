"""asvio.client : call an ASVIO app from Python , with the same shape as gradio_client

    from asvio.client import Client, handle_file
    client = Client("http://127.0.0.1:7860", download_files="/tmp/out")
    depth_path = client.predict(handle_file("image.jpg"))                 # api_name="/predict"
    embeddings = client.predict([handle_file(p) for p in paths], api_name="/embed")

File outputs come back as local paths ( downloaded into download_files ) , several outputs as a tuple.
Standard library only.
"""

import json
import os
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request
import uuid

FILE_KEY = "__asvio_file__"
UPDATE_KEY = "__asvio_update__"


class AppError(Exception):
    """The app raised an error ( gr.Error text , or the generic message when show_error is off )."""


class FileRef:
    def __init__(self, path):
        self.path = os.fspath(path)

    def __repr__(self):
        return "handle_file(%r)" % self.path


def handle_file(path):
    """Mark a local path as a file to upload."""
    if not os.path.isfile(path):
        raise ValueError("handle_file: %s is not a file" % path)
    return FileRef(path)


class Client:

    def __init__(self, src, download_files=None, verbose=True, timeout=None, **kwargs):
        self.src = src.rstrip("/")
        if "://" not in self.src:
            self.src = "http://" + self.src
        self.download_files = download_files or os.path.join(tempfile.gettempdir(), "asvio_client")
        self.timeout = timeout       # seconds a single predict may take , None waits forever
        self.session = uuid.uuid4().hex
        self.submitted = {}   # job id -> api name , so result(job) knows which outputs it gets
        self.config = self._get_json("/config.json")
        if verbose:
            print("Loaded as API: %s ✔" % self.src)

    # ---------------------------------------------------------------- http
    def _open(self, path, body=None, content_type=None, timeout=60):
        request = urllib.request.Request(self.src + path, data=body, headers={"Content-Type": content_type} if content_type else {})
        return urllib.request.urlopen(request, timeout=timeout)

    def _get_json(self, path, timeout=60):
        try:
            with self._open(path, timeout=timeout) as response:
                return json.loads(response.read())
        except urllib.error.HTTPError as e:
            body = e.read()
            try:
                message = json.loads(body).get("error", body)
            except ValueError:
                message = body
            raise AppError("%s : HTTP %d %s" % (path, e.code, message))

    # ---------------------------------------------------------------- api
    def _encode(self, value, files):
        """Arguments → the data JSON , each FileRef becomes a multipart part."""
        if isinstance(value, FileRef):
            if os.path.getsize(value.path) == 0:
                # an empty multipart part makes AmmarServer drop it and every part after it
                return {FILE_KEY: "empty", "name": os.path.basename(value.path)}
            field = "file%d" % len(files)
            files.append((field, value.path))
            return {FILE_KEY: field, "name": os.path.basename(value.path)}
        if isinstance(value, (list, tuple)):
            return [self._encode(v, files) for v in value]
        if isinstance(value, dict) and value.get("meta", {}).get("_type") == "gradio.FileData":
            return self._encode(handle_file(value["path"]), files)
        return value

    def _multipart(self, fields, files):
        boundary = uuid.uuid4().hex
        parts = []
        for name, value in fields.items():
            parts.append(("--%s\r\nContent-Disposition: form-data; name=\"%s\"\r\n\r\n" % (boundary, name)).encode() + value.encode() + b"\r\n")
        for field, path in files:
            filename = os.path.basename(path).replace('"', "_")
            with open(path, "rb") as f:
                content = f.read()
            parts.append(("--%s\r\nContent-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n"
                          "Content-Type: application/octet-stream\r\n\r\n" % (boundary, field, filename)).encode("utf-8") + content + b"\r\n")
        return b"".join(parts) + ("--%s--\r\n" % boundary).encode(), "multipart/form-data; boundary=" + boundary

    def _download(self, value):
        """Wire outputs → Python values , downloading every file."""
        if isinstance(value, dict) and FILE_KEY not in value:
            return {k: self._download(v) for k, v in value.items()}
        if isinstance(value, dict) and FILE_KEY in value:
            directory = os.path.join(self.download_files, uuid.uuid4().hex)
            os.makedirs(directory, exist_ok=True)
            name = os.path.basename(value.get("name") or "file") or "file"
            path = os.path.join(directory, name)
            with self._open(value[FILE_KEY], timeout=300) as response, open(path, "wb") as f:
                while True:
                    chunk = response.read(1 << 20)
                    if not chunk:
                        break
                    f.write(chunk)
            return path
        if isinstance(value, list):
            return [self._download(v) for v in value]
        return value

    def _dependency(self, api_name):
        for dependency in self.config["dependencies"]:
            if dependency["api_name"] == api_name:
                return dependency
        raise AppError("There is no api_name %s ( see view_api() )" % api_name)

    def _is_state(self, component_id):
        return self.config["components"][str(component_id)]["type"] == "state"

    def _arguments(self, dependency, args, kwargs):
        """Positional and keyword arguments -> one value per input , like gradio_client : State inputs are not
        parameters ( the server keeps them per session ) , missing parameters take their defaults."""
        parameters = dependency["parameters"]
        if len(args) > len(parameters):
            raise TypeError("%s takes %d arguments , %d given" % (dependency["api_name"], len(parameters), len(args)))
        names = [p["name"] for p in parameters]
        unknown = set(kwargs) - set(names)
        if unknown:
            raise TypeError("%s has no parameter %s ( it has %s )" % (dependency["api_name"], ", ".join(sorted(unknown)), ", ".join(names)))
        values = {}
        for index, parameter in enumerate(parameters):
            if index < len(args):
                values[parameter["component"]] = args[index]
            elif parameter["name"] in kwargs:
                values[parameter["component"]] = kwargs[parameter["name"]]
            elif parameter["has_default"]:
                values[parameter["component"]] = parameter["default"]
            else:
                raise TypeError("%s needs a value for %s" % (dependency["api_name"], parameter["name"]))
        return [values.get(component_id) for component_id in dependency["inputs"]]

    def submit(self, *args, api_name="/predict", **kwargs):
        """Start a job and return its id."""
        if not api_name.startswith("/"):
            api_name = "/" + api_name
        dependency = self._dependency(api_name)
        files = []
        data = json.dumps([self._encode(a, files) for a in self._arguments(dependency, args, kwargs)])
        # never an empty multipart field : AmmarServer's parser drops it and every field after it
        body, content_type = self._multipart({"api": api_name, "session": self.session, "data": data}, files)
        try:
            with self._open("/asvio/run.json", body, content_type) as response:
                job = json.loads(response.read())["job"]
                self.submitted[job] = api_name
                return job
        except urllib.error.HTTPError as e:
            raise AppError("%s : HTTP %d %s" % (api_name, e.code, e.read()[:500]))

    def result(self, job):
        """Wait for a job and return its outputs , like predict."""
        api_name = self.submitted.pop(job, "/predict")
        started, seq = time.time(), 0
        while True:
            reply = self._get_json("/asvio/poll.json?job=%s&seq=%d" % (job, seq), timeout=60)
            seq = reply["seq"]
            if reply["status"] in ("done", "error", "cancelled"):
                break
            if self.timeout is not None and time.time() - started > self.timeout:
                self._get_json("/asvio/cancel.json?job=" + job)
                raise TimeoutError("job %s took longer than %ss" % (job, self.timeout))
        data = reply.get("data") or {}
        if reply["status"] == "error":
            raise AppError(data.get("error", "error"))
        if reply["status"] == "cancelled":
            raise AppError("cancelled")
        outputs = data.get("outputs")
        if outputs is None:
            return None
        dependency = self._dependency(api_name)
        # like gradio_client : State outputs are left out , updates come back as {"__type__": "update" , ...}
        outputs = [self._download(dict(v[UPDATE_KEY], __type__="update") if isinstance(v, dict) and UPDATE_KEY in v else v)
                   for v, component_id in zip(outputs, dependency["outputs"]) if not self._is_state(component_id)]
        return outputs[0] if len(outputs) == 1 else tuple(outputs)

    def predict(self, *args, api_name="/predict", **kwargs):
        if not api_name.startswith("/"):
            api_name = "/" + api_name
        return self.result(self.submit(*args, api_name=api_name, **kwargs))

    def view_api(self, print_info=True):
        components = self.config["components"]
        lines = []
        for dependency in self.config["dependencies"]:
            if not dependency["api_name"]:
                continue
            describe = lambda ids: ", ".join("%s(%s)" % (components[str(i)]["type"], components[str(i)]["props"].get("label") or "") for i in ids if not self._is_state(i))
            parameters = ", ".join("%s%s" % (p["name"], "=%r" % (p["default"],) if p["has_default"] else "") for p in dependency["parameters"])
            lines.append("client.predict(%sapi_name=%r) -> %s" % (parameters + ", " if parameters else "", dependency["api_name"], describe(dependency["outputs"])))
        if print_info:
            print("\n".join(lines))
        return lines
