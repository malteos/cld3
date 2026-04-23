"""Compact Language Detector v3 (CLD3) — Python bindings.

Distributed on PyPI as ``cld3-py``. The import name remains ``gcld3`` so that
code written against the original Google package keeps working unchanged::

    import gcld3
    detector = gcld3.NNetLanguageIdentifier(min_num_bytes=0, max_num_bytes=1000)
    result = detector.FindLanguage(text="Hello, world!")
"""

from .pybind_ext import NNetLanguageIdentifier, Result
from .pybind_ext import __version__ as _ext_version

__version__ = _ext_version

__all__ = [
    "NNetLanguageIdentifier",
    "Result",
    "__version__",
]
