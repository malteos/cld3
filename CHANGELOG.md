# Changelog

All notable changes to `cld3-py` are documented here. The project follows
[Semantic Versioning](https://semver.org/).

## [3.1.0] — unreleased

First release of the modernised fork of Google's `gcld3`. Distributed on PyPI
as **`cld3-py`**. The import name remains `gcld3` so existing downstream code
using `import gcld3` works unchanged.

### Added
- Prebuilt wheels for Python 3.10, 3.11, 3.12, 3.13, 3.14 (incl. free-threaded
  `cp313t` / `cp314t` variants).
- Prebuilt wheels for `manylinux_2_28` (x86_64, aarch64), `musllinux_1_2`
  (x86_64, aarch64), macOS Intel + Apple Silicon, and Windows AMD64.
- Modern build system: `pyproject.toml` + `scikit-build-core` + CMake.
- `FetchContent` pin of protobuf v3.21.12, statically linked — no system
  protobuf / protoc required for either source or wheel builds.
- Parity test harness (`tests/test_parity.py`, `tests/scripts/parity_runner.cc`)
  that compares Python bindings against the original C++ `NNetLanguageIdentifier`
  across ~75 multilingual samples.
- `__version__` attribute and `__repr__` on `Result`.
- GIL released during inference calls; module declared `mod_gil_not_used` for
  free-threaded builds.
- Trusted-Publisher (OIDC) publishing to PyPI + TestPyPI via GitHub Actions.

### Changed
- Bumped minimum Python to 3.10.
- Bumped pybind11 requirement to ≥ 2.13.6.
- Bumped CMake minimum to 3.26 and C++ standard to C++17.
- Dropped `-D_GLIBCXX_USE_CXX11_ABI=0` (incompatible with `manylinux_2_28`).
- Removed the abusive `namespace pybind11 { ... }` wrapper from
  `gcld3/pybind_ext.cc`.

### Removed
- `setup.py` (replaced by `pyproject.toml` + scikit-build-core).
- `requirements.txt` (build deps now live in `pyproject.toml`).
- Old `.github/workflows/main.yml` (replaced by `publish.yml`).
