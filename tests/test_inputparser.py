"""Regression tests for the InputParser tokenizer library.

The actual checks live in InputParser_C_Tester/main.c behind --self-test, so
they can also be run directly:

    ./src/InputParser/testinputparser --self-test

They cover zero-copy parsing of read-only strings, NUL termination of the
parser's own copy, and switching between keepcopy=1 and keepcopy=0 without
inheriting ownership of caller memory (the old code could write into a string
literal or free() a stack buffer).
"""

import os
import subprocess
import unittest


REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_BINARY = os.path.join(REPO_ROOT, "src", "InputParser", "testinputparser")


class TestInputParser(unittest.TestCase):
    def test_self_tests(self):
        binary = os.environ.get("INPUTPARSER_TEST_BINARY", DEFAULT_BINARY)
        if not os.path.isfile(binary):
            self.fail(
                "InputParser self-test binary not found at %s "
                "(build the testinputparser target)" % binary
            )

        result = subprocess.run(
            [binary, "--self-test"],
            cwd=REPO_ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=60,
        )

        self.assertEqual(result.returncode, 0, msg=result.stdout)
        self.assertNotIn("FAIL", result.stdout)


if __name__ == "__main__":
    unittest.main()
