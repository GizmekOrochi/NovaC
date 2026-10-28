#!/usr/bin/env python3
"""Regression gate: parked Compilation must not leak into NovaC's core or install.

Run from any directory. --library checks the actual (fresh) static archive,
--installed checks an installed include root after staging / packaging.
"""
import argparse
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
PARKED_SOURCES = ROOT / "src/core/novac/assets/compilation"
PARKED_HEADERS = ROOT / "src/include/novac/assets/compilation"
BACKEND_SOURCES = PARKED_SOURCES / "backend"
BACKEND_HEADERS = PARKED_HEADERS / "backend"
LEGACY_DIRS = (
    "src/core/novac/engine/backend",
    "src/core/novac/engine/compilation",
    "src/include/novac/engine/backend",
    "src/include/novac/engine/compilation",
    "src/core/novac/assets/backend",
    "src/include/novac/assets/backend",
)
BAD_LINK_SYMBOLS = re.compile(
    r"novac::(?:compilation::|backend::|controllers::CompilationController(?:\b|::))"
)
BAD_CORE_DEPENDENCIES = re.compile(
    r"novac/(?:engine/(?:compilation|backend)|assets/(?:compilation|backend))/"
    r"|novac::(?:compilation::|backend::|controllers::CompilationController\b)"
)


def check(condition, message, errors):
    if not condition:
        errors.append(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library", type=Path, help="compiled libNovaC.a to inspect")
    parser.add_argument("--installed", type=Path, help="installed include directory to inspect")
    args = parser.parse_args()
    errors = []

    for relative in LEGACY_DIRS:
        check(not (ROOT / relative).exists(), f"legacy directory has returned: {relative}", errors)
    for relative in (
        "Compilation.cpp", "CompilationController.cpp"
    ):
        check((PARKED_SOURCES / relative).is_file(),
              f"missing parked compilation source: {relative}", errors)
    for relative in (
        "Compilation.hpp", "CompilationController.hpp"
    ):
        check((PARKED_HEADERS / relative).is_file(),
              f"missing parked compilation header: {relative}", errors)

    # HIR, MIR and the generic IR are shared interpretation/analysis primitives.
    # They MUST live in Engine and be packaged with the core library.
    engine_ir_sources = ROOT / "src/core/novac/engine/transformation"
    engine_ir_headers = ROOT / "src/include/novac/engine/transformation"
    for required in ("IR.hpp", "GenericIR.hpp"):
        check((engine_ir_headers / required).is_file(),
              f"missing Engine IR header: {required}", errors)
    check((engine_ir_sources / "IR.cpp").is_file(),
          "missing Engine IR implementation", errors)
    # Prevent two implementations of the same novac::ir symbols in the project.
    check(not (PARKED_HEADERS / "ir").exists(),
          "duplicate IR headers in parked compilation asset", errors)
    check(not (PARKED_SOURCES / "ir").exists(),
          "duplicate IR implementation in parked compilation asset", errors)

    check((BACKEND_SOURCES / "Backend.cpp").is_file(),
          "missing parked backend source: Backend.cpp", errors)
    check((BACKEND_HEADERS / "Backend.hpp").is_file(),
          "missing parked backend header: Backend.hpp", errors)

    # IR in engine/transformation is intentional. Check only dependencies on
    # the parked compiler/backend; never reject IR or generic lowering helpers.
    for location in (ROOT / "src/core/novac", ROOT / "src/include"):
        for file in location.rglob("*"):
            if file.suffix not in (".cpp", ".hpp", ".h"):
                continue
            if any(parked in file.parents for parked in (PARKED_SOURCES, PARKED_HEADERS)):
                continue
            if BAD_CORE_DEPENDENCIES.search(file.read_text(encoding="utf-8")):
                errors.append(f"core depends on parked compilation: {file.relative_to(ROOT)}")

    if args.library:
        library = args.library.resolve()
        check(library.is_file(), f"missing static library: {library}", errors)
        if library.is_file():
            try:
                # Check both defined and undefined symbols: even an unresolved
                # dependency would make the compiled engine incorrectly coupled.
                result = subprocess.run(["nm", "-C", str(library)],
                                        capture_output=True, text=True, check=True)
            except (OSError, subprocess.CalledProcessError) as exc:
                errors.append(f"cannot examine library with nm: {exc}")
            else:
                check("novac::ir::HIRBuilder::" in result.stdout and
                      "novac::ir::MIRBuilder::" in result.stdout,
                      "Engine IR builders missing from core archive", errors)
                for line in result.stdout.splitlines():
                    if BAD_LINK_SYMBOLS.search(line):
                        errors.append(f"Compilation symbol leaked into core archive: {line.strip()}")
                        if len(errors) >= 15:
                            break
                try:
                    members = subprocess.run(["ar", "t", str(library)],
                                             capture_output=True, text=True, check=True).stdout
                except (OSError, subprocess.CalledProcessError) as exc:
                    errors.append(f"cannot list archive members: {exc}")
                else:
                    for member in members.splitlines():
                        if member in ("Compilation.o", "CompilationController.o", "Backend.o") or re.search(r"(?:^|/)(?:backend|compilation)/", member, re.I):
                            errors.append(f"Compilation object leaked into core archive: {member}")

    if args.installed:
        includedir = args.installed.resolve()
        check((includedir / "NovaC.hpp").is_file(),
              f"NovaC.hpp absent from installed headers: {includedir}", errors)
        for relative in (
            "novac/engine/backend", "novac/engine/compilation",
            "novac/assets/compilation", "novac/assets/backend",
        ):
            check(not (includedir / relative).exists(),
                  f"parked/legacy headers installed: {relative}", errors)

    if errors:
        print("[NovaC compilation isolation] FAILED:", file=sys.stderr)
        for error in errors:
            print(" - " + error, file=sys.stderr)
        return 1
    print("[NovaC compilation isolation] OK: engine, archive and requested install checks are isolated.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
