"""Stand-in for the real gradio package in tests : only the Error class a helper library ( deepseek_vl2 ) raises."""


class Error(Exception):
    def __init__(self, message="Error raised."):
        super().__init__(message)
        self.message = message
