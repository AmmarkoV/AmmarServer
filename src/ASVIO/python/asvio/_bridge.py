"""ctypes bindings for libasvio.so , the C side of asvio ( see ../../bridge/asvio.h ).

The structures below mirror struct asvio_file / struct asvio_job in asvio.h , keep the two in sync.
Every call here goes through ctypes.CDLL , which releases the GIL while the C function runs , so a
blocking next_job() does not stall other Python threads.
"""

import ctypes
import os

ASVIO_JOB_ID_SIZE = 33
ASVIO_MAX_FILES = 32


class _File(ctypes.Structure):
    _fields_ = [("field", ctypes.c_char_p),
                ("filename", ctypes.c_char_p),
                ("content", ctypes.c_void_p),
                ("length", ctypes.c_uint)]


class _Job(ctypes.Structure):
    _fields_ = [("id", ctypes.c_char * ASVIO_JOB_ID_SIZE),
                ("api", ctypes.c_char_p),
                ("session", ctypes.c_char_p),
                ("data", ctypes.c_void_p),
                ("dataLength", ctypes.c_uint),
                ("fileCount", ctypes.c_uint),
                ("files", _File * ASVIO_MAX_FILES)]


class Job:
    """A Python copy of one job's input , safe to keep after finish() frees the C side."""

    def __init__(self, job_id, api, session, data, files):
        self.id = job_id
        self.api = api
        self.session = session
        self.data = data      # bytes , the multipart "data" field
        self.files = files    # list of (field, filename, bytes)

    def __repr__(self):
        return "Job(%s %s session=%s files=%d)" % (self.id, self.api, self.session, len(self.files))


_lib = None


def _load():
    global _lib
    if _lib is not None:
        return _lib
    path = os.environ.get("ASVIO_LIB") or os.path.join(os.path.dirname(os.path.abspath(__file__)), "libasvio.so")
    if not os.path.isfile(path):
        raise ImportError("asvio: %s not found , build it with: mkdir -p build && cd build && cmake .. && make asvio "
                          "( from the AmmarServer root ) or point ASVIO_LIB at it" % path)
    lib = ctypes.CDLL(path)
    lib.asvio_start.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_uint, ctypes.c_char_p, ctypes.c_uint, ctypes.c_uint]
    lib.asvio_start.restype = ctypes.c_int
    lib.asvio_running.argtypes = []
    lib.asvio_running.restype = ctypes.c_int
    lib.asvio_stop.argtypes = []
    lib.asvio_stop.restype = None
    lib.asvio_next_job.argtypes = [ctypes.c_uint]
    lib.asvio_next_job.restype = ctypes.POINTER(_Job)
    for name in ("asvio_job_update", "asvio_job_finish"):
        getattr(lib, name).argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_uint]
        getattr(lib, name).restype = ctypes.c_int
    lib.asvio_job_cancelled.argtypes = [ctypes.c_char_p]
    lib.asvio_job_cancelled.restype = ctypes.c_int
    _lib = lib
    return lib


def start(name, ip, port, webroot, max_upload_mb, max_response_mb):
    return bool(_load().asvio_start(name.encode(), ip.encode(), port, webroot.encode(), max_upload_mb, max_response_mb))


def running():
    return bool(_load().asvio_running())


def stop():
    _load().asvio_stop()


def next_job(timeout_ms):
    """The oldest job nobody took yet , or None after timeout_ms."""
    pointer = _load().asvio_next_job(timeout_ms)
    if not pointer:
        return None
    job = pointer.contents
    files = [(f.field.decode("utf-8", "replace"), f.filename.decode("utf-8", "replace"), ctypes.string_at(f.content, f.length))
             for f in job.files[:job.fileCount]]
    return Job(job.id.decode(), job.api.decode(), job.session.decode(), ctypes.string_at(job.data, job.dataLength), files)


def _publish(function, job_id, status, payload):
    data = payload.encode() if isinstance(payload, str) else payload
    return bool(function(job_id.encode(), status.encode(), data, len(data) if data is not None else 0))


def update(job_id, status, payload=None):
    """Publish a new state of a running job ; payload is a JSON string , None keeps the previous one."""
    return _publish(_load().asvio_job_update, job_id, status, payload)


def finish(job_id, status, payload=None):
    """Final state ( "done" , "error" , "cancelled" ) ; frees the job's input on the C side."""
    return _publish(_load().asvio_job_finish, job_id, status, payload)


def cancelled(job_id):
    return bool(_load().asvio_job_cancelled(job_id.encode()))
