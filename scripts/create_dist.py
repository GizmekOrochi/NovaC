#!/usr/bin/env python3
"""Produce a clean, source-only release archive."""
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
import sys

root = Path(__file__).resolve().parent.parent
output = Path(sys.argv[1]).resolve()
prefix = sys.argv[2]
ignore_dirs = {".git", "bin", "lib", "build", "dist", ".venv", "_build", "__pycache__"}
ignore_suffixes = {".o", ".pyc", ".a", ".d"}
output.parent.mkdir(parents=True, exist_ok=True)
with ZipFile(output, "w", compression=ZIP_DEFLATED) as z:
    for path in sorted(root.rglob("*")):
        if not path.is_file() or ignore_dirs.intersection(path.relative_to(root).parts):
            continue
        if path.suffix in ignore_suffixes:
            continue
        z.write(path, arcname=f"{prefix}/{path.relative_to(root).as_posix()}")
print(f"Created {output}")
