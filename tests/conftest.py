"""Pytest fixtures for the C++↔Python parity harness.

The C++ reference binary is `tests/scripts/parity_runner.cc` built via
`cmake -DCLD3_BUILD_TOOLS=ON`. The path is discovered from the
``CLD3_PARITY_BINARY`` environment variable, falling back to a few standard
build directories. Parity tests are skipped (not failed) if the binary is
missing so that local dev without CMake still runs the other tests.
"""

from __future__ import annotations

import json
import os
import shutil
import subprocess
from pathlib import Path
from typing import Any

import pytest


TESTS_DIR = Path(__file__).resolve().parent
DATA_DIR = TESTS_DIR / "data"
REPO_ROOT = TESTS_DIR.parent


def _find_parity_binary() -> Path | None:
    env = os.environ.get("CLD3_PARITY_BINARY")
    if env:
        p = Path(env)
        return p if p.exists() else None

    names = ["parity_runner", "parity_runner.exe"]
    candidate_dirs = [
        REPO_ROOT / "build",
        REPO_ROOT / "build" / "Release",
        REPO_ROOT / "build" / "Debug",
    ]
    # Also scan build/{wheel_tag}/ created by scikit-build-core.
    build_dir = REPO_ROOT / "build"
    if build_dir.is_dir():
        candidate_dirs.extend(
            sub for sub in build_dir.iterdir() if sub.is_dir()
        )

    for d in candidate_dirs:
        for n in names:
            p = d / n
            if p.exists() and os.access(p, os.X_OK):
                return p.resolve()

    on_path = shutil.which("parity_runner")
    return Path(on_path) if on_path else None


@pytest.fixture(scope="session")
def parity_binary() -> Path:
    p = _find_parity_binary()
    if p is None:
        pytest.skip(
            "parity_runner binary not found. "
            "Build with `cmake -S . -B build -DCLD3_BUILD_TOOLS=ON && cmake --build build` "
            "or set CLD3_PARITY_BINARY."
        )
    return p


@pytest.fixture(scope="session")
def corpus() -> list[dict[str, Any]]:
    with open(DATA_DIR / "corpus.json", encoding="utf-8") as f:
        return json.load(f)


def _run_cpp(binary: Path, args: list[str], samples: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """Feed `samples` as JSON lines to the C++ binary; parse stdout as JSON lines."""
    stdin_text = "\n".join(
        json.dumps({"id": s["id"], "text": s["text"]}, ensure_ascii=False)
        for s in samples
    ) + "\n"
    result = subprocess.run(
        [str(binary), *args],
        input=stdin_text,
        capture_output=True,
        text=True,
        encoding="utf-8",
        check=True,
    )
    return [json.loads(line) for line in result.stdout.splitlines() if line.strip()]


@pytest.fixture(scope="session")
def cpp_find_language(parity_binary: Path, corpus: list[dict[str, Any]]) -> dict[str, dict[str, Any]]:
    rows = _run_cpp(parity_binary, ["--find-language"], corpus)
    return {row["id"]: row for row in rows}


@pytest.fixture(scope="session")
def cpp_find_top_n(parity_binary: Path, corpus: list[dict[str, Any]]) -> dict[str, dict[str, Any]]:
    rows = _run_cpp(parity_binary, ["--find-top-n", "3"], corpus)
    return {row["id"]: row for row in rows}


@pytest.fixture(scope="session")
def py_detector():
    import gcld3

    return gcld3.NNetLanguageIdentifier(min_num_bytes=0, max_num_bytes=1000)
