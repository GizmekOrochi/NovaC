#!/usr/bin/env python3
"""Compile and run NovaC documentation examples against the installed package."""

from __future__ import annotations

import os
from pathlib import Path
import shutil
import subprocess
import sys

CONFIG = Path(__file__).resolve().parent
ROOT = CONFIG.parent.parent
BUILD = CONFIG / "_build" / "example-check"
STAGE = BUILD / "stage"
BIN = BUILD / "bin"

CXX = os.environ.get("CXX", "g++")
CXXFLAGS = ["-std=c++20", "-Wall", "-Wextra", "-O0"]

EXAMPLES = {
    "minimal.cpp": "42\n",
    "calculator.cpp": "1 + 2 * 3 -> 7\n10 >= 5 -> true\ntrue && !false -> true\n",
    "custom_language.cpp": "factorial(5) =\n120\nsum =\n10\nreturned: 130\n",
    "ir_builder.cpp": "entry\n  const\n  const\n  add\n  return\n",
}


def run(command: list[str], *, cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, cwd=cwd, check=True, text=True, capture_output=True)


def main() -> int:
    if shutil.which(CXX) is None:
        print(f"error: compiler '{CXX}' was not found", file=sys.stderr)
        return 1

    shutil.rmtree(BUILD, ignore_errors=True)
    BIN.mkdir(parents=True)

    jobs = str(min(8, max(1, os.cpu_count() or 1)))

    try:
        run([
            "make", f"-j{jobs}", "install", f"CXX={CXX}",
            f"DESTDIR={STAGE}", "PREFIX=/usr",
        ], cwd=ROOT)

        include_dir = STAGE / "usr" / "include"
        library = STAGE / "usr" / "lib" / "libNovaC.a"
        umbrella = include_dir / "NovaC.hpp"

        if not umbrella.is_file():
            print(f"error: installed umbrella header is missing: {umbrella}", file=sys.stderr)
            return 1
        if not library.is_file():
            print(f"error: installed static library is missing: {library}", file=sys.stderr)
            return 1

        for filename, expected in EXAMPLES.items():
            source = CONFIG / "examples" / filename
            executable = BIN / source.stem
            run([
                CXX, *CXXFLAGS, f"-I{include_dir}", str(source), str(library),
                "-o", str(executable),
            ])
            result = run([str(executable)])
            if result.stdout != expected:
                print(f"error: unexpected output for {filename}", file=sys.stderr)
                print("expected:", repr(expected), file=sys.stderr)
                print("actual:  ", repr(result.stdout), file=sys.stderr)
                return 1
            print(f"[NovaC docs] installed-package example OK: {filename}")
    except subprocess.CalledProcessError as error:
        if error.stdout:
            print(error.stdout, file=sys.stderr)
        if error.stderr:
            print(error.stderr, file=sys.stderr)
        return error.returncode or 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
