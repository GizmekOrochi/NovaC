#!/usr/bin/env python3
"""Compile and run the C++ examples used by the NovaC Sphinx documentation."""

from __future__ import annotations

import concurrent.futures
import os
from pathlib import Path
import shutil
import subprocess
import sys

CONFIG = Path(__file__).resolve().parent
ROOT = CONFIG.parent.parent
BUILD = CONFIG / "_build" / "example-check"
OBJ = BUILD / "obj"
BIN = BUILD / "bin"

CXX = os.environ.get("CXX", "g++")
CXXFLAGS = ["-std=c++20", "-Wall", "-Wextra", "-O0"]
INCLUDES = [f"-I{ROOT / 'src'}", f"-I{ROOT / 'src' / 'include'}"]

EXAMPLES = {
    "minimal.cpp": "42\n",
    "calculator.cpp": "1 + 2 * 3 -> 7\n10 >= 5 -> true\ntrue && !false -> true\n",
    "custom_language.cpp": "factorial(5) =\n120\nsum =\n10\nreturned: 130\n",
    "ir_builder.cpp": "entry\n  const\n  const\n  add\n  return\n",
}


def run(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, check=True, text=True, capture_output=True)


def object_path(source: Path) -> Path:
    relative = source.relative_to(ROOT)
    return OBJ / ("__".join(relative.parts) + ".o")


def compile_core(source: Path) -> Path:
    output = object_path(source)
    output.parent.mkdir(parents=True, exist_ok=True)
    run([CXX, *CXXFLAGS, *INCLUDES, "-c", str(source), "-o", str(output)])
    return output


def main() -> int:
    if shutil.which(CXX) is None:
        print(f"error: compiler '{CXX}' was not found", file=sys.stderr)
        return 1

    shutil.rmtree(BUILD, ignore_errors=True)
    OBJ.mkdir(parents=True)
    BIN.mkdir(parents=True)

    core_sources = sorted((ROOT / "src" / "core").rglob("*.cpp"))
    workers = min(8, max(1, os.cpu_count() or 1))

    try:
        with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as pool:
            objects = list(pool.map(compile_core, core_sources))

        for filename, expected in EXAMPLES.items():
            source = CONFIG / "examples" / filename
            executable = BIN / source.stem
            run([CXX, *CXXFLAGS, *INCLUDES, str(source), *map(str, objects), "-o", str(executable)])
            result = run([str(executable)])
            if result.stdout != expected:
                print(f"error: unexpected output for {filename}", file=sys.stderr)
                print("expected:", repr(expected), file=sys.stderr)
                print("actual:  ", repr(result.stdout), file=sys.stderr)
                return 1
            print(f"[NovaC docs] example OK: {filename}")
    except subprocess.CalledProcessError as error:
        print(error.stderr, file=sys.stderr)
        return error.returncode or 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
