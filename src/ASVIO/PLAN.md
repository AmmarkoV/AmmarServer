# ASVIO — AmmarSerVer I/O

A Gradio-like Python UI library that runs on top of AmmarServer.

**Goal:** the model servers in `~/Documents/Programming/RGBToPoseDetect2D/datasets/*/server.py` run unchanged
except for `import gradio as gr` → `import asvio as gr`. Their `client.py` scripts switch
`from gradio_client import Client, handle_file` → `from asvio.client import Client, handle_file`.

**Non-goal:** wire compatibility with Gradio. ASVIO has its own small HTTP protocol, and `asvio.client` is its client.

---

## 1. What the target servers use

| Server | Gradio API | Notes |
|---|---|---|
| depthanythingv3 | `Interface`, `Image(type="filepath")`, `File`, `Error`, `launch(server_name, server_port, share)`, `delete_cache`, `flagging_mode` | 16-bit PNG output must come back byte-identical |
| dinov3 | `Blocks(title, delete_cache)`, `Markdown`, `File(file_count="multiple", file_types)`, `JSON`, `Button(...).click(fn, inputs, outputs, api_name)`, `queue()` | |
| segmentanything3 | `Interface(article=...)`, `Image(type="pil", format="png")`, `Textbox(placeholder)`, `JSON` | Built on FastAPI `mount_gradio_app` + `/version` + `/client.py` → **gets a ~15 line edit** to `demo.launch()` + `demo.add_route()` / `demo.add_static()` |
| sam3d-obj | SAM3's set + `Number(value, precision)`, `Model3D`, `Image(image_mode=None)` | FastAPI, same edit as SAM3 |
| deepseekvlm, qwenvlm | `Blocks(theme)`, `Row`/`Column(scale, min_width)`/`Tab`, `Chatbot`, `State`, `Files`, `Gallery(columns, height)`, `Slider`, `Dropdown`, `Checkbox`, `HTML(visible)`, `Examples`, `gr.update`, returning `gr.Button(visible=True)` as an update, `.submit`/`.click`/`.change`/`.then`, `cancels=`, generator functions (token streaming), `queue(max_size)`, `themes.Soft` | Client calls `/reset_state`, `/transfer_input`, `/predict` in sequence with shared state |

Clients rely on: `Client(url, download_files=dir, verbose=)`, `predict(*args, api_name="/predict")`, `handle_file(path)`,
file/image outputs returned as **local downloaded paths**, JSON outputs as Python objects, several outputs as a tuple.

## 2. Architecture

```
browser (vanilla JS, no CDN, no build)          asvio.client (stdlib only)
   │ GET  /index.html /asvio.js /asvio.css /config.json   ← AmmarServer static path (files never change at runtime)
   │ POST /asvio/run.json     multipart: api, session, data=JSON, file0..N   → {"job":"<id>"}
   │ GET  /asvio/poll.json?job=J&seq=N   LONG-POLL: blocks until seq>N , job finished , or ~20s
   │ GET  /asvio/cancel.json?job=J
   │ GET  /asvio/file.json?id=<name>     outputs , via AmmServer_DynamicRequestReturnFile
   ▼
libAmmarServer (threaded mode , DIFFERENT_PAGE_FOR_EACH_CLIENT handlers)
   ▼
libasvio.so (src/ASVIO/bridge) : job table + FIFO , one mutex , two condvars
   ▲ asvio_next_job() / asvio_job_update() / asvio_job_finish()   (ctypes drops the GIL while blocked)
Python : dispatcher thread → per-event concurrency → user fn → components pre/postprocess
```

Why it is shaped like this (measured in the phase-0 spike):
- Python never runs on server threads, so there is no GIL inside the server and no ctypes callbacks.
- Handler threads may block for a long time: 30s holds were fine and other clients were unaffected. That is
  what makes long-polling possible , and long-polling is how yields stream without SSE.
- API mode is not used: its callback runs on the single event loop thread and must not block.
- The static path caches forever (it serves rewritten or deleted files from memory), so changing files
  (outputs) only go through `/asvio/file.json` , which uses `ReturnFile` and so never touches the cache.
- Each dynamic response buffer is malloc'd **and memset** to its full size on every request. The poll
  buffer is therefore kept modest (default 2MB , `ASVIO_MAX_RESPONSE_MB`). Big outputs travel as files.
- The multipart parser drops a field with an empty value **and every field after it**. Clients never send
  an empty field: all non-file values go in one `data` JSON array , and file parts are sent only when present.

### Directories at runtime
- **webroot** = its own `mkdtemp("asvio_")`: frontend copy + `config.json` + `asvio_out/` (outputs). It must
  NOT live under `GRADIO_TEMP_DIR` , because the target scripts run reapers there that delete anything older
  than 120s.
- **uploads** go to `GRADIO_TEMP_DIR` (or `ASVIO_TEMP_DIR` , or a mkdtemp), the same place Gradio puts them ,
  so the scripts' own reapers keep working.
- Outputs returned as paths are hard-linked (or copied across filesystems) into `asvio_out/<random>.<ext>`.
  `delete_cache=(every,age)` reaps `asvio_out/` and the uploads.

## 3. Protocol

All responses are JSON (`.json` resources are served as `application/json`).

| Route | Request | Response |
|---|---|---|
| `POST /asvio/run.json` | multipart: `api` (e.g. `/predict`), `session` (client-chosen random id), `data` (JSON array, file slots are `{"__file__":"file0"}`), `file0..N` | `{"job":"<32 hex>"}` , or `{"error":...}` with 400/503 |
| `GET /asvio/poll.json?job=J&seq=N[&wait=S]` | `seq` = last seq seen (0 at start) , `wait` = seconds to hold (default 20 , max 25) | `{"job":J,"seq":k,"status":"queued|running|done|error|cancelled","data":<python payload or null>}` , 404 for an unknown or expired job |
| `GET /asvio/cancel.json?job=J` | | `{"ok":1}` |
| `GET /asvio/file.json?id=<name>` | name is `[A-Za-z0-9._-]` and has no `..` | file bytes with the type from its extension , 404 if gone |

Input file values are `{"__asvio_file__":"file0","name":...}` (a multipart part) , `{"__asvio_file__":"out:<id>"}` (a file
this server already published , e.g. an output fed back in) , or `{"__asvio_file__":"empty","name":...}` for a 0 byte file.
An empty file is never sent as a part , for the same parser reason as empty fields.

The Python payload is either `{"outputs":[...]}` , where file outputs are `{"__file__":"/asvio/file.json?id=x.png","name":"orig.png"}`
and updates are `{"__update__":{...}}`, or `{"error":"message"}` (`gr.Error` text , or the exception text).
Each generator yield is one `asvio_job_update` (seq+1). `status` stays `running` until the final `asvio_job_finish`.

Finished jobs stay in the table for 10 minutes so a slow client can still read them , then they are reaped.

## 4. C bridge API (`bridge/asvio.h`)

```c
int  asvio_start(const char * name,const char * ip,unsigned int port,const char * webroot,
                 unsigned int maxUploadMB,unsigned int maxResponseMB);
int  asvio_running();
void asvio_stop();

struct asvio_job * asvio_next_job(unsigned int timeoutMS);   // 0 on timeout ; input stays valid until asvio_job_finish
int  asvio_job_update(const char * jobID,const char * status,const char * payload,unsigned int payloadLength);
int  asvio_job_finish(const char * jobID,const char * status,const char * payload,unsigned int payloadLength);
int  asvio_job_cancelled(const char * jobID);
```
`struct asvio_job` (id , api , session , data , files[] of {field , filename , content , length}) is mirrored in
`python/asvio/_bridge.py` and has to be kept in sync with it.

libasvio.so links the **static** libAmmarServer.a , so the Python package is self-contained. CMake writes it into
`python/asvio/`.

## 5. Python package (`python/asvio/`)

- `_bridge.py` — ctypes bindings.
- `components.py` — `Component` with `config()` (for the frontend), `preprocess(wire)` → value , `postprocess(value)` → wire.
  `gr.update(**props)` and returning a component instance are both updates.
- `blocks.py` — `Blocks` context manager with layout (`Row`, `Column`, `Tab`) , event listeners (`click`, `submit`,
  `change`, `then`, `cancels`) , `State` per session , `queue()` , `launch()` , `add_route()` , `add_static()`.
- `interface.py` — `Interface` / `TabbedInterface` built on Blocks (api name `/predict`).
- `client.py` — `Client` , `handle_file` , `predict` / `submit` / `result` / `view_api`. Standard library only (urllib) , so
  a `client.py` handed out to other machines needs nothing compiled. `src/AmmClient` was considered and not used: its
  POST sends exactly one file part and no other fields , and its receive buffer has to be sized before the download.
- `frontend/` — `index.html` , `asvio.js` (with a small Markdown renderer of its own : headings , lists , code , bold/italic ,
  links , raw HTML passed through minus scripts and `on*` handlers) , `asvio.css` (light + dark) , `favicon.svg`.

Install into a venv with `pip install -e src/ASVIO/python` (after `make asvio`) , or put `src/ASVIO/python` on `PYTHONPATH`.

`launch()` points fds 1 and 2 at a log file (`/tmp/asvio_<pid>_server.log` , or `ASVIO_SERVER_LOG`) because AmmarServer
prints every request from C , and keeps Python's `sys.stdout` / `sys.stderr` on the terminal , so `print()` and tracebacks
still show. `ASVIO_SERVER_LOG=-` leaves the server output on the terminal. SIGTERM cleans up like Ctrl+C.

Accepted and ignored: `theme`, `flagging_mode`, `show_progress`, `elem_id`, `equal_height`, `show_error`.
`share=True` warns. Unknown kwargs warn once, they do not raise.

Concurrency: like Gradio , one running job per event (`concurrency_limit=1`) unless `queue(default_concurrency_limit=N)`.
Different events may run in parallel. The C side takes every job straight into Python , so concurrency and
`queue(max_size)` live in Python: a job over max_size finishes at once with `{"error":"Queue is full"}`.
The C job table itself holds at most 4096 jobs (queued + running + finished-but-remembered) and answers 503 above that.

## 6. Phases

Each phase ends with its check passing. Real model servers need a GPU and are run by the user. Automated tests
use stub apps that copy each server's Gradio usage with fake model functions.

- [x] **0. Spike** — blocking handlers , uploads , ReturnFile , static cache , Ctrl+C (done 2026-10-28).
  AmmServerlib changes that came out of it: `.json`→`application/json` and `.js`→`application/javascript`
  (StringRecognizer `applicationFiles`) , and SendFile now answers 404 instead of an empty 200 when a
  non-cached file (e.g. from ReturnFile) cannot be streamed from disk.
- [x] **1. C bridge** — `bridge/asvio.c`, CMake target , `python/asvio/_bridge.py` , `tests/test_bridge.py` (ctypes
  plays the Python worker). Check: submit → fetch → streamed updates → finish seen through poll ; long-poll timeout ;
  cancel of a queued job and of a running one ; 8MB upload bytes intact ; file.json 200/404/bad-name ; unknown job 404 ;
  output bigger than the poll buffer → 500 ; 20 concurrent jobs. **10/10 pass** (2026-10-28).
  Found along the way: AmmarServer 400s a dynamic resource whose directory does not exist on disk (but still runs its
  callback) , so `asvio_start` creates `webroot/asvio/`.
- [x] **2. Python core + frontend** — `Interface` , `Image` , `File` , `Textbox` , `Number` , `JSON` , `Markdown` , `HTML` ,
  `Button` , `ClearButton` , `Error` , `launch` , `queue` , `delete_cache` , generators , `asvio.client` , `pyproject.toml`.
  (`Blocks` + `Button.click` came along , since Interface is built on them.) Done 2026-10-28:
  - `tests/test_interface.py` **14/14** : stub depth app round-trips a 16-bit PNG **byte-identically** , 9MB upload ,
    `gr.Error` text reaches the client , exception text hidden without `show_error` , numpy-in / PIL-PNG-out ,
    multiple files (incl. 0 bytes) , every generator yield seen , cancel stops a running generator , one job per event
    at a time , Ctrl+C and SIGTERM exit 0 and remove the web root.
  - Headless Chrome over CDP (scratch script , not in the repo) : upload → Submit → 16-bit PNG link , Clear , error
    toast , text/number/JSON , tokens streamed one by one , image round trip , light + dark , no console errors.
  - The real `depthanythingv3/client.py` (import line changed only , run in a copy) processed a folder against the stub
    server : `uint16` depth maps , 0..65535.
  - **Still open for the user:** the real depthanythingv3 `server.py` on the GPU with `import asvio as gr`.
- [x] **3. Blocks basics + SAM3/SAM3D** — `Blocks` , `Button.click(api_name)` , multi-file `File` with gradio's `file_types`
  check , `Image` preprocess now step for step gradio 5's (EXIF transpose , filepath untouched when already in
  `image_mode`) , `Number(precision)` , `Model3D` , `article` , `add_route` / `add_static` ,
  gradio's output count rule (too few → ValueError , too many → warning + truncate). Done 2026-10-28:
  - `tests/test_blocks.py` **11/11** (dinov3 / sam3 / sam3d shaped stubs in `tests/apps/blocks_app.py`).
  - **The real servers ran:** depthanythingv3 , dinov3 , segmentanything3 , sam3d-obj `server.py` under the
    RGBToPoseDetect2D venv with fake model modules (real torch / numpy / PIL / cv2) , each driven by its own real
    `client.py` (import line changed only) : 22/22 checks (scratch harness , not in the repo). SAM3/SAM3D needed the
    FastAPI→`launch` edit (≈ 25 lines out , 9 in , see below) ; depth and dinov3 only the import line.
  - `add_route(path, fn)` calls fn **once at launch** (AmmarServer serves it as a file) , which fits `/version`.
    Extensionless routes and `.py` go out as `text/html` (unknown extension) ; the article's `<a download>` still downloads.

  The SAM3 / SAM3D edit replaces the FastAPI + `gr.mount_gradio_app` + `uvicorn.run` tail with:
  ```python
  demo.add_route("/version", lambda: {"version": SERVER_VERSION, "source": os.path.realpath(__file__),
                                      "client_py_available": os.path.isfile(CLIENT_PY)})
  if os.path.isfile(CLIENT_PY):
      demo.add_static("/client.py", CLIENT_PY)
  demo.launch(server_name=args.host, server_port=args.port, show_error=True)
  ```
  and drops the `uvicorn` / `fastapi` imports.
- [x] **4. Chat apps** — `State` (per session , expires after 1h idle) , `Chatbot` (messages and tuples , inline `data:`
  images moved into published files so each streamed token stays small) , `.then` , `cancels` , `Examples` (files
  published once at launch as `example_*` , never reaped) , `Tab` / `Tabs` , `Gallery` , `Slider` , `Dropdown` (gradio's
  choice check) , `Checkbox` , `HTML(visible=False)` , `gr.update` and returned components as updates (gradio's
  `constructor_args` rule) , `NamedString` file inputs (`f.name`) , `.change` fired by function updates too ,
  `gr.themes.*` accepted , `launch(favicon_path=)` , a real `gradio.Error` raised by a helper library treated as
  `gr.Error`. `asvio.client` takes keyword arguments named like gradio_client's (fn parameter names , State skipped ,
  component values as defaults) and returns updates as `{"__type__": "update", ...}` without State outputs. Done 2026-10-28:
  - `tests/test_chat.py` **15/15** (deepseek / qwen shaped stub `tests/apps/chat_app.py`).
  - The real `qwenvlm/server_asvio.py` with fake transformers / qwen_vl_utils / argostranslate (its venv's torch) :
    10/10 checks , and in headless Chrome : example row fills Files + Gallery + Textbox , Send clears the inputs and
    streams 13 states into the chat , Enter submits , Stop cancels , the tab holds the sliders , no console errors.
  - The real `deepseekvlm/client_asvio.py` captioned a folder through the deepseek-shaped stub. The real deepseek
    server needs `deepseek_vl2` (`~/Documents/Programming/DeepSeek-VL2`) and was not run ; its `server_asvio.py`
    is the import line only , and its helpers (checked against that source) behave as the stub's.
  - Not done: `TabbedInterface` (no target server uses it).
- [x] **Model3D viewer** (`frontend/model3d.js` , WebGL2 , no libraries) : `.glb` / `.gltf` with embedded buffers (node
  transforms , `COLOR_0` , base colour) , `.ply` ascii / binary (faces as a lit mesh , otherwise points ; gaussian splat
  plys coloured from `f_dc_*`) , `.obj` (vertex colours) , `.stl` , `.splat` as points. Drag rotates , wheel zooms ,
  shift- or right-drag pans , double click resets. Inputs preview their upload too. Splats are drawn as coloured points ,
  not as gaussians. Checked in headless Chrome (2026-10-28) with one model per format : each draws (its triangle / point
  count shown , coloured pixels on screen) , dragging changes the view , an unsupported file says so , no console errors.

`*_asvio.py` files in RGBToPoseDetect2D/datasets (the gradio originals are untouched):
`depthanythingv3` , `dinov3` , `segmentanything3` , `sam3d-obj` , `deepseekvlm` (server + client) , `qwenvlm` (server).
SAM3 / SAM3D's `server_asvio.py` also serve `/asvio_client.py` and their `client_asvio.py` falls back to importing it ,
so a client downloaded from the page runs with nothing installed but its own dependencies.

## 7. Known limitations

- Streaming arrives through long-poll , so every yield costs one HTTP round trip. That is fine for tokens ,
  but it is not a socket.
- Range requests (206) still send their header before the file is checked , so a missing file on that path
  still gets an empty body.
- `gr.mount_gradio_app` / FastAPI integration is not provided. SAM3 / SAM3D have `server_asvio.py` variants instead.
- Published outputs had lost their extension (so went out as `text/html`) until phase 4 ; fixed , they keep it now.
- **Fixed in AmmServerlib (2026-10-28):** a `DIFFERENT_PAGE_FOR_EACH_CLIENT` callback that used `ReturnFile` leaked its
  per-request buffer (`cache/file_caching.c` "Please free content here..") , measured at 2.02KB per `/asvio/file.json`
  download. It is freed there now , and growth went to 0.00KB/request.
- AmmarServer's 404 status line carries the server-side path (`404 Not Found /tmp/asvio_www_.../404.html`) , so the
  web root location is visible to clients. Left as is , since it is library behaviour.
- Every multipart POST logs "Server failed to parse POST data". That comes from the closing boundary part being
  flagged as empty , and the fields themselves arrive intact. It is log noise only.
