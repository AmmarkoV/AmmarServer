"""Components : each one gives the frontend its config , turns a wire value into what fn receives ( preprocess ) ,
and turns what fn returns into a wire value ( postprocess ). File values cross the wire as
{"__asvio_file__": ref , "name": original name} , see PLAN.md section 3."""

import mimetypes
import os
import warnings
from pathlib import Path

from .blocks import Block, warn_ignored

FILE_KEY = "__asvio_file__"


class Error(Exception):
    """Raise this in fn to show message to the user ( the browser , or the client's exception )."""

    def __init__(self, message="Error raised.", duration=None, visible=True, title="Error"):
        super().__init__(message)
        self.message = message


class NamedString(str):
    """A path that also has .name , like gradio's : File inputs used to be file objects , and code still calls f.name"""

    def __init__(self, *args):
        super().__init__()
        self.name = str(self) if args else ""


def update(**props):
    """gr.update(value=... , visible=... , ...) : return it from fn to change some properties of an output."""
    props["__type__"] = "update"
    return props


class Component(Block):
    kind = "component"

    def __init__(self, value=None, label=None, show_label=True, interactive=None, visible=True, elem_id=None,
                 elem_classes=None, render=True, scale=None, min_width=None, info=None, **kwargs):
        warn_ignored(type(self).__name__, kwargs)
        self.value = value
        self.label = label
        self.show_label = show_label
        self.interactive = interactive
        self.info = info
        super().__init__(visible=visible, elem_id=elem_id, render=render)

    def config(self):
        props = self.layout_props()
        props.update(label=self.label, show_label=self.show_label, info=self.info, value=self.initial_value())
        return props

    def initial_value(self):
        return self.value

    def preprocess(self, value, files):
        return value

    def postprocess(self, value, files):
        return value

    def _listen(self, event, fn, inputs, outputs, api_name, concurrency_limit, cancels, kwargs):
        warn_ignored(event, {k: v for k, v in kwargs.items() if k not in ("queue", "show_progress", "trigger_mode", "show_api")})
        if self.root is None:
            raise RuntimeError("asvio: %s.%s() on a component that is not inside a Blocks" % (type(self).__name__, event))
        return self.root._add_dependency([(self, event)], fn, inputs, outputs, api_name, concurrency_limit, cancels)

    def click(self, fn=None, inputs=None, outputs=None, api_name=None, concurrency_limit="default", cancels=None, **kwargs):
        return self._listen("click", fn, inputs, outputs, api_name, concurrency_limit, cancels, kwargs)

    def submit(self, fn=None, inputs=None, outputs=None, api_name=None, concurrency_limit="default", cancels=None, **kwargs):
        return self._listen("submit", fn, inputs, outputs, api_name, concurrency_limit, cancels, kwargs)

    def change(self, fn=None, inputs=None, outputs=None, api_name=None, concurrency_limit="default", cancels=None, **kwargs):
        return self._listen("change", fn, inputs, outputs, api_name, concurrency_limit, cancels, kwargs)


class Textbox(Component):
    kind = "textbox"

    def __init__(self, value="", placeholder=None, lines=1, max_lines=20, type="text", **kwargs):
        self.placeholder = placeholder
        self.lines = lines
        self.max_lines = max_lines
        self.type = type
        super().__init__(value=value, **kwargs)

    def config(self):
        props = super().config()
        props.update(placeholder=self.placeholder, lines=self.lines, max_lines=self.max_lines, type=self.type)
        return props

    def preprocess(self, value, files):
        return "" if value is None else str(value)

    def postprocess(self, value, files):
        return None if value is None else str(value)


class Number(Component):
    kind = "number"

    def __init__(self, value=None, precision=None, minimum=None, maximum=None, step=1, **kwargs):
        self.precision = precision
        self.minimum = minimum
        self.maximum = maximum
        self.step = step
        super().__init__(value=value, **kwargs)

    def config(self):
        props = super().config()
        props.update(precision=self.precision, minimum=self.minimum, maximum=self.maximum, step=self.step)
        return props

    def _round(self, value):
        if value is None:
            return None
        if self.precision is None:
            return float(value)
        if self.precision == 0:
            return int(round(float(value)))
        return round(float(value), self.precision)

    def preprocess(self, value, files):
        value = self._round(value)
        if value is not None and self.minimum is not None and value < self.minimum:
            raise ValueError("%s is below the minimum %s" % (value, self.minimum))
        if value is not None and self.maximum is not None and value > self.maximum:
            raise ValueError("%s is above the maximum %s" % (value, self.maximum))
        return value

    def postprocess(self, value, files):
        return self._round(value)


class JSON(Component):
    kind = "json"


class Markdown(Component):
    kind = "markdown"

    def __init__(self, value="", **kwargs):
        super().__init__(value=value, **kwargs)

    def postprocess(self, value, files):
        return None if value is None else str(value)


class HTML(Markdown):
    kind = "html"


class Button(Component):
    kind = "button"

    def __init__(self, value="Run", variant="secondary", size=None, icon=None, link=None, **kwargs):
        self.variant = variant
        super().__init__(value=value, **kwargs)

    def config(self):
        props = super().config()
        props.update(variant=self.variant)
        return props


class ClearButton(Button):
    """Resets its components in the browser , no server round trip."""
    kind = "clearbutton"

    def __init__(self, components=None, value="Clear", **kwargs):
        self.components = components if isinstance(components, (list, tuple)) else ([components] if components else [])
        super().__init__(value=value, **kwargs)

    def config(self):
        props = super().config()
        props.update(components=[c._id for c in self.components])
        return props


class File(Component):
    kind = "file"

    def __init__(self, value=None, file_count="single", file_types=None, type="filepath", height=None, **kwargs):
        if file_count not in ("single", "multiple"):
            raise ValueError("asvio: File(file_count=%r) is not supported" % file_count)
        self.file_count = file_count
        self.file_types = file_types
        self.type = type
        super().__init__(value=value, **kwargs)

    def config(self):
        props = super().config()
        props.update(file_count=self.file_count, file_types=self.file_types)
        return props

    def initial_value(self):
        return None   # a default file would have to be published first , not needed by any target server yet

    def _valid_type(self, path):
        """gradio_client.utils.is_valid_file : ".ext" matches the extension , "image" etc. the mimetype , "file" anything"""
        mime = mimetypes.guess_type(path)[0]
        for file_type in self.file_types:
            if file_type == "file":
                return True
            if file_type.startswith("."):
                if file_type.lstrip(".").lower() == Path(path).suffix.lstrip(".").lower():
                    return True
            elif mime is not None and mime.startswith(file_type + "/"):
                return True
        return False

    def _one(self, value, files):
        path = files.resolve(value)
        if self.type == "filepath" and self.file_types and not self._valid_type(path):
            raise Error("Invalid file type. Please upload a file that is one of these formats: %s" % self.file_types)
        if self.type == "binary":
            with open(path, "rb") as f:
                return f.read()
        return NamedString(path)

    def preprocess(self, value, files):
        if value is None:
            return None
        if self.file_count == "multiple":
            return [self._one(v, files) for v in (value if isinstance(value, list) else [value])]
        return self._one(value[0] if isinstance(value, list) else value, files)

    def postprocess(self, value, files):
        if value is None:
            return None
        if isinstance(value, (list, tuple)):
            return [files.publish_path(v) for v in value]
        return files.publish_path(value)


class Files(File):
    def __init__(self, value=None, file_count="multiple", **kwargs):
        super().__init__(value=value, file_count=file_count, **kwargs)


class Model3D(File):
    """A .glb / .gltf / .obj / .ply / .stl / .splat file , previewed in the browser ( frontend/model3d.js ) above its download."""
    kind = "model3d"

    def __init__(self, value=None, clear_color=None, camera_position=None, zoom_speed=None, pan_speed=None,
                 display_mode=None, height=None, **kwargs):
        super().__init__(value=value, file_count="single", type="filepath", **kwargs)


class Image(Component):
    kind = "image"

    def __init__(self, value=None, type="numpy", image_mode="RGB", format="webp", height=None, width=None,
                 sources=None, **kwargs):
        if type not in ("numpy", "pil", "filepath"):
            raise ValueError("asvio: Image(type=%r) is not supported" % type)
        self.type = type
        self.image_mode = image_mode
        self.format = format
        self.height = height
        self.width = width
        super().__init__(value=value, **kwargs)

    def config(self):
        props = super().config()
        props.update(height=self.height, width=self.width)
        return props

    def initial_value(self):
        return None

    def preprocess(self, value, files):
        # same steps as gradio 5 Image.preprocess
        if value is None:
            return None
        path = files.resolve(value)
        from PIL import Image as PILImage, ImageOps
        image = PILImage.open(path)
        if self.type == "filepath" and self.image_mode in (None, image.mode):
            return path   # hand over the upload as it is
        if image.getexif().get(274, 1) != 1:   # 274 = EXIF orientation , 1 = upright
            try:
                image = ImageOps.exif_transpose(image)
            except Exception:
                pass
        if image.format != "GIF" and self.image_mode is not None:
            image = image.convert(self.image_mode)
        if self.type == "pil":
            return image
        if self.type == "numpy":
            import numpy
            return numpy.array(image)
        # filepath , converted : save it next to the upload in the upload's own format
        stem, suffix = os.path.splitext(path)
        converted_path = stem + "_" + self.image_mode + (suffix or ".png")
        image.save(converted_path)
        return converted_path

    def postprocess(self, value, files):
        if value is None:
            return None
        if isinstance(value, (str, Path)):
            return files.publish_path(value)
        from PIL import Image as PILImage
        if not isinstance(value, PILImage.Image):
            value = PILImage.fromarray(value)
        image_format = (self.format or "png").lower()
        if image_format == "jpg":
            image_format = "jpeg"
        if image_format == "jpeg" and value.mode not in ("RGB", "L"):
            value = value.convert("RGB")
        return files.publish_writer(lambda path: value.save(path, format=image_format.upper()),
                                    "." + ("jpg" if image_format == "jpeg" else image_format), "image." + image_format)


class State(Component):
    """A per-session Python value ( per browser tab , per asvio.client Client ) , never sent over the wire."""
    kind = "state"

    def __init__(self, value=None, render=True, time_to_live=None, delete_callback=None, **kwargs):
        super().__init__(value=value, render=render, **kwargs)

    def config(self):
        return {"visible": False}


class Slider(Component):
    kind = "slider"

    def __init__(self, minimum=0, maximum=100, value=None, step=None, **kwargs):
        self.minimum = minimum
        self.maximum = maximum
        self.step = step
        super().__init__(value=minimum if value is None else value, **kwargs)

    def config(self):
        props = super().config()
        props.update(minimum=self.minimum, maximum=self.maximum, step=self.step)
        return props

    def preprocess(self, value, files):
        return value


class Checkbox(Component):
    kind = "checkbox"

    def __init__(self, value=False, **kwargs):
        super().__init__(value=value, **kwargs)

    def preprocess(self, value, files):
        return bool(value)


class Dropdown(Component):
    kind = "dropdown"

    def __init__(self, choices=None, value=None, type="value", multiselect=None, allow_custom_value=False, **kwargs):
        # choices are values , or ( label , value ) pairs
        self.choices = [tuple(c) if isinstance(c, (list, tuple)) else (str(c), c) for c in (choices or [])]
        self.type = type
        self.multiselect = bool(multiselect)
        self.allow_custom_value = allow_custom_value
        if value is None and not self.multiselect and self.choices:
            value = self.choices[0][1]
        super().__init__(value=value, **kwargs)

    def config(self):
        props = super().config()
        props.update(choices=[list(c) for c in self.choices], multiselect=self.multiselect)
        return props

    def preprocess(self, value, files):
        values = [c[1] for c in self.choices]
        chosen = value if isinstance(value, list) else ([] if value is None else [value])
        for v in chosen:
            if v not in values and not self.allow_custom_value:
                raise Error("Value: %r is not in the list of choices: %s" % (v, values))
        if self.type == "index":
            indices = [values.index(v) if v in values else None for v in chosen]
            return indices if self.multiselect else (indices[0] if indices else None)
        return value


class Chatbot(Component):
    """Messages ( [{"role": , "content": }] ) or tuples ( [[user , bot]] ) , passed through as the app gives them.
    Inline data: images ( e.g. an <img src="data:image/png;base64,..."> in a message ) are moved into published
    files , so the whole chat stays small enough to resend on every streamed token."""
    kind = "chatbot"

    def __init__(self, value=None, type=None, height=None, **kwargs):
        super().__init__(value=value if value is not None else [], **kwargs)

    def postprocess(self, value, files):
        if value is None:
            return []
        return files.extract_inline_images(value)


class Gallery(Component):
    kind = "gallery"

    def __init__(self, value=None, columns=2, rows=None, height=None, preview=None, object_fit=None, allow_preview=True,
                 type="filepath", format="webp", **kwargs):
        self.columns = columns[0] if isinstance(columns, (list, tuple)) else columns
        self.height = height
        self.type = type
        self.format = format
        super().__init__(value=value, **kwargs)

    def config(self):
        props = super().config()
        props.update(columns=self.columns, height=self.height)
        return props

    def initial_value(self):
        return None

    def postprocess(self, value, files):
        if value is None:
            return None
        items = []
        for item in value:
            caption = None
            if isinstance(item, (list, tuple)):
                item, caption = item[0], (item[1] if len(item) > 1 else None)
            if isinstance(item, (str, Path)):
                items.append([files.publish_path(item), caption])
            else:
                items.append([Image(format=self.format, render=False).postprocess(item, files), caption])
        return items

    def preprocess(self, value, files):
        if value is None:
            return None
        return [(files.resolve(item), caption) for item, caption in value]


class Examples(Block):
    """Rows of example inputs ; clicking one fills the inputs in. Example files are published once , at launch."""
    kind = "examples"

    def __init__(self, examples, inputs, outputs=None, fn=None, cache_examples=None, examples_per_page=10, label="Examples",
                 elem_id=None, run_on_click=False, render=True, **kwargs):
        warn_ignored("Examples", kwargs)
        self.inputs = inputs if isinstance(inputs, (list, tuple)) else [inputs]
        self.examples = [row if isinstance(row, (list, tuple)) else [row] for row in examples]
        self.label = label
        self.interactive = False
        self.rows = None
        super().__init__(elem_id=elem_id, render=render)

    def prepare(self, files):
        """Publish the example values the way each input component publishes an output."""
        self.rows = []
        for row in self.examples:
            wire = []
            for component, value in zip(self.inputs, row):
                try:
                    wire.append(None if value is None else component.postprocess(value, files))
                except Exception as e:
                    warnings.warn("asvio: example %r for %s is skipped: %s" % (value, component.label or component.kind, e))
                    wire.append(None)
            self.rows.append(wire)

    def config(self):
        return {"label": self.label, "inputs": [c._id for c in self.inputs], "rows": self.rows or []}


_SHORTCUTS = {"text": Textbox, "textbox": Textbox, "number": Number, "json": JSON, "markdown": Markdown,
              "html": HTML, "image": Image, "file": File, "files": Files, "model3d": Model3D}


def from_shortcut(component):
    """Interface accepts "text" , "image" ... for a default component , like gradio."""
    if isinstance(component, str):
        if component.lower() not in _SHORTCUTS:
            raise ValueError("asvio: unknown component shortcut %r" % component)
        return _SHORTCUTS[component.lower()](render=False)
    return component
