"""Blocks : the component tree , layout containers and event wiring , mirroring gradio.Blocks"""

import functools
import os
import inspect
import itertools
import string
import threading
import warnings

_ids = itertools.count(1)
_context = threading.local()   # .stack : the containers currently open with "with"
_warned = set()


def warn_ignored(owner, kwargs):
    """Gradio takes many cosmetic kwargs ASVIO does not implement : accept them , warn once per name."""
    for name in kwargs:
        key = (owner, name)
        if key not in _warned:
            _warned.add(key)
            warnings.warn("asvio: %s(%s=...) is not supported and is ignored" % (owner, name), stacklevel=3)


def _stack():
    if not hasattr(_context, "stack"):
        _context.stack = []
    return _context.stack


class Block:
    """Anything with a place in the layout : components and containers."""

    def __init_subclass__(cls, **kwargs):
        # Remember the arguments of the outermost constructor call , like gradio's constructor_args : a component
        # returned from fn ( e.g. gr.Button(visible=True) ) updates exactly the properties that were passed
        super().__init_subclass__(**kwargs)
        original = cls.__dict__.get("__init__")
        if original is None:
            return
        signature = inspect.signature(original)

        @functools.wraps(original)
        def init(self, *args, **kw):
            if not hasattr(self, "constructor_args"):
                bound = signature.bind_partial(self, *args, **kw).arguments
                bound.pop("self", None)
                bound.update(bound.pop("kwargs", {}))
                self.constructor_args = bound
            original(self, *args, **kw)
        cls.__init__ = init

    def __init__(self, visible=True, elem_id=None, elem_classes=None, render=True):
        self._id = next(_ids)
        self.visible = visible
        self.elem_id = elem_id
        self.parent = None
        self.root = None
        if render and _stack():
            self.render()

    def render(self):
        stack = _stack()
        if not stack:
            raise RuntimeError("asvio: render() needs an open Blocks context")
        if self.parent is not None:
            self.parent.children.remove(self)
        self.parent = stack[-1]
        self.parent.children.append(self)
        self.root = stack[0]
        self.root._blocks[self._id] = self
        return self

    def layout_props(self):
        return {"visible": self.visible, "elem_id": self.elem_id}


class BlockContext(Block):
    """A container opened with "with" ; everything created inside becomes its child."""

    kind = "container"

    def __init__(self, **kwargs):
        self.children = []
        super().__init__(**kwargs)

    def __enter__(self):
        _stack().append(self)
        return self

    def __exit__(self, *exc):
        _stack().pop()

    def layout(self):
        node = {"id": self._id, "type": self.kind, "props": self.layout_props(), "children": []}
        for child in self.children:
            if isinstance(child, BlockContext):
                node["children"].append(child.layout())
            else:
                node["children"].append({"id": child._id})
        return node


class Row(BlockContext):
    kind = "row"

    def __init__(self, equal_height=False, variant="default", visible=True, elem_id=None, elem_classes=None, render=True, **kwargs):
        warn_ignored("Row", kwargs)
        super().__init__(visible=visible, elem_id=elem_id, render=render)


class Column(BlockContext):
    kind = "column"

    def __init__(self, scale=1, min_width=320, variant="default", visible=True, elem_id=None, elem_classes=None, render=True, **kwargs):
        warn_ignored("Column", kwargs)
        self.scale = scale
        self.min_width = min_width
        super().__init__(visible=visible, elem_id=elem_id, render=render)

    def layout_props(self):
        props = super().layout_props()
        props.update(scale=self.scale, min_width=self.min_width)
        return props


class Tabs(BlockContext):
    kind = "tabs"

    def __init__(self, selected=None, visible=True, elem_id=None, elem_classes=None, render=True, **kwargs):
        warn_ignored("Tabs", kwargs)
        super().__init__(visible=visible, elem_id=elem_id, render=render)


class Tab(BlockContext):
    """A tab ; consecutive Tabs ( or the Tabs of one gr.Tabs ) share one tab bar."""
    kind = "tab"

    def __init__(self, label=None, visible=True, interactive=True, id=None, elem_id=None, elem_classes=None, render=True, **kwargs):
        warn_ignored("Tab", kwargs)
        self.label = label
        super().__init__(visible=visible, elem_id=elem_id, render=render)

    def layout_props(self):
        props = super().layout_props()
        props.update(label=self.label)
        return props


TabItem = Tab


class Dependency:
    """One event handler : what triggers it , the fn it runs , its inputs and outputs , and what runs after it."""

    def __init__(self, root, triggers, fn, inputs, outputs, api_name, concurrency_limit, cancels):
        self.root = root
        self.index = len(root._dependencies)
        self.triggers = triggers          # [(block, event name)]
        self.fn = fn
        self.inputs = inputs
        self.outputs = outputs
        self.api_name = api_name          # "/name" , or None when not callable through the API
        self.concurrency_limit = concurrency_limit
        self.cancels = cancels
        self.then_dependencies = []

    def then(self, fn, inputs=None, outputs=None, api_name=None, concurrency_limit="default", cancels=None, **kwargs):
        warn_ignored("then", kwargs)
        dependency = self.root._add_dependency([], fn, inputs, outputs, api_name, concurrency_limit, cancels)
        self.then_dependencies.append(dependency)
        return dependency

    def parameters(self):
        """The API parameters , as gradio names them : State inputs are skipped , each parameter is named after fn's argument
        at the same position ( param_<i> when there is none ) , and defaults to the component's initial value."""
        try:
            arguments = [(name, p.default) for name, p in inspect.signature(self.fn).parameters.items()
                         if p.kind not in (p.VAR_POSITIONAL, p.VAR_KEYWORD)] if self.fn else []
        except (TypeError, ValueError):
            arguments = []
        parameters = []
        for index, block in enumerate(self.inputs):
            if block.kind == "state":
                continue
            name = arguments[index][0] if index < len(arguments) and arguments[index][0] not in ("api_name", "fn_index", "result_callbacks") else "param_%d" % index
            value = block.config().get("value")
            if value is not None:
                parameters.append({"name": name, "component": block._id, "has_default": True, "default": value})
            elif index < len(arguments) and arguments[index][1] is None:
                parameters.append({"name": name, "component": block._id, "has_default": True, "default": None})
            else:
                parameters.append({"name": name, "component": block._id, "has_default": False, "default": None})
        return parameters

    def config(self):
        return {"id": self.index,
                "api_name": self.api_name,
                "targets": [[block._id, event] for block, event in self.triggers],
                "inputs": [block._id for block in self.inputs],
                "outputs": [block._id for block in self.outputs],
                "parameters": self.parameters() if self.api_name else [],
                "then": [d.index for d in self.then_dependencies],
                "cancels": [d.index for d in self.cancels]}


def _as_list(blocks):
    if blocks is None:
        return []
    if isinstance(blocks, (list, tuple)):
        return list(blocks)
    return [blocks]


class Blocks(BlockContext):
    """The root of a UI. Use as "with asvio.Blocks() as demo:" then demo.launch()."""

    kind = "blocks"

    def __init__(self, title="ASVIO", theme=None, css=None, delete_cache=None, analytics_enabled=None, fill_height=False, **kwargs):
        warn_ignored("Blocks", kwargs)
        self.title = title
        self.delete_cache = delete_cache
        self._blocks = {}
        self._dependencies = []
        self._api_names = set()
        self._max_size = None
        self._default_concurrency_limit = 1
        self._runtime = None
        self._extra_files = {}   # url path -> ("file" , source path) or ("json" , fn) , see add_static / add_route
        super().__init__(render=False)
        if _stack():
            # a Blocks opened inside another one is rendered there , like gradio
            self.render()

    def __enter__(self):
        stack = _stack()
        if not stack:
            self.root = self
            self._blocks[self._id] = self
        stack.append(self)
        return self

    def _add_dependency(self, triggers, fn, inputs, outputs, api_name, concurrency_limit, cancels):
        if api_name is False or fn is None:
            api_name = None
        else:
            base = (api_name or getattr(fn, "__name__", None) or type(fn).__name__).strip("/")
            if not api_name:
                # gradio drops punctuation from function names : <lambda> -> lambda
                base = "".join(c for c in base if c not in set(string.punctuation) - {"-", "_"})
            api_name = "/" + base
            suffix = 1
            while api_name in self._api_names:
                api_name = "/%s_%d" % (base, suffix)
                suffix += 1
            self._api_names.add(api_name)
        dependency = Dependency(self, triggers, fn, _as_list(inputs), _as_list(outputs), api_name, concurrency_limit, _as_list(cancels))
        self._dependencies.append(dependency)
        return dependency

    def add_static(self, path, file_path):
        """ASVIO extension , not in gradio : serve file_path at /path ( e.g. "/client.py" ). Copied when launch() runs."""
        self._extra_files[path.strip("/")] = ("file", file_path)
        return self

    def add_route(self, path, fn):
        """ASVIO extension , not in gradio : serve fn()'s result as JSON at /path ( e.g. "/version" ).
        AmmarServer serves it as a file , so fn is called once , when launch() runs."""
        self._extra_files[path.strip("/")] = ("json", fn)
        return self

    def queue(self, max_size=None, default_concurrency_limit=1, **kwargs):
        warn_ignored("queue", kwargs)
        self._max_size = max_size
        self._default_concurrency_limit = default_concurrency_limit
        return self

    def config(self):
        used_as_input = {b._id for d in self._dependencies for b in d.inputs}
        used_as_output = {b._id for d in self._dependencies for b in d.outputs}
        components = {}
        for block_id, block in self._blocks.items():
            if isinstance(block, BlockContext):
                continue
            interactive = block.interactive
            if interactive is None:
                interactive = not (block_id in used_as_output and block_id not in used_as_input)
            components[block_id] = {"type": block.kind, "props": block.config(), "interactive": interactive}
        return {"title": self.title,
                "components": components,
                "layout": self.layout(),
                "dependencies": [d.config() for d in self._dependencies]}

    def launch(self, server_name=None, server_port=None, share=False, prevent_thread_lock=False, show_error=False,
               favicon_path=None, **kwargs):
        from .runtime import Runtime
        warn_ignored("launch", {k: v for k, v in kwargs.items() if k not in ("quiet", "inbrowser", "theme", "debug")})
        if share:
            warnings.warn("asvio: share=True is not supported , serving locally only", stacklevel=2)
        if favicon_path and not os.path.isfile(favicon_path):
            warnings.warn("asvio: favicon_path %s does not exist , keeping the default icon" % favicon_path, stacklevel=2)
            favicon_path = None
        self._runtime = Runtime(self, show_error)
        self._runtime.start(server_name, server_port, favicon_path)
        if not prevent_thread_lock:
            self._runtime.block()
        return self

    def close(self):
        if self._runtime is not None:
            self._runtime.stop()
            self._runtime = None
