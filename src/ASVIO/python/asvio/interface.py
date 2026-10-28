"""Interface : the one-function UI , built from Blocks the way gradio.Interface lays it out"""

from .blocks import Blocks, Column, Row, warn_ignored
from .components import Button, ClearButton, Markdown, Textbox, from_shortcut


def _as_components(components):
    if components is None:
        return []
    if not isinstance(components, (list, tuple)):
        components = [components]
    return [from_shortcut(c) for c in components]


class Interface(Blocks):

    def __init__(self, fn, inputs, outputs, examples=None, title=None, description=None, article=None,
                 api_name="predict", concurrency_limit="default", submit_btn="Submit", clear_btn="Clear",
                 flagging_mode=None, allow_flagging=None, delete_cache=None, theme=None, css=None, **kwargs):
        warn_ignored("Interface", kwargs)
        if examples is not None:
            warn_ignored("Interface", {"examples": examples})
        super().__init__(title=title or "ASVIO", theme=theme, css=css, delete_cache=delete_cache)
        self.fn = fn
        self.input_components = _as_components(inputs)
        self.output_components = _as_components(outputs)

        with self:
            if title:
                Markdown("# " + title)
            if description:
                Markdown(description)
            with Row():
                with Column():
                    for component in self.input_components:
                        component.render()
                    with Row():
                        ClearButton(self.input_components + self.output_components, value=clear_btn)
                        submit = Button(submit_btn, variant="primary")
                with Column():
                    for component in self.output_components:
                        component.render()
            if article:
                Markdown(article)

        # gradio also runs the Interface when Enter is pressed in a single line Textbox input
        triggers = [(submit, "click")] + [(c, "submit") for c in self.input_components if isinstance(c, Textbox) and c.lines == 1]
        self._add_dependency(triggers, fn, self.input_components, self.output_components, api_name, concurrency_limit, None)
