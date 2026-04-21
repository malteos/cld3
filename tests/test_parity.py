"""C++ ↔ Python parity test.

For every sample in `tests/data/corpus.json`, runs the original C++
reference (`parity_runner`, linked directly against `cld3_core`) and the
Python binding with identical `NNetLanguageIdentifier(0, 1000)` parameters,
then asserts:

- ``language`` is identical
- ``is_reliable`` is identical
- ``|probability_cpp − probability_py| < 1e-6``
- ``|proportion_cpp  − proportion_py|  < 1e-6``

This proves the Python wheel produces the same predictions as the reference.
The tolerance absorbs SIMD-reduction-order FP drift; a real regression
(wrong softmax output, flipped sign, off-by-one feature) would blow past it.
"""

from __future__ import annotations

import pytest


FLOAT_EPS = 1e-6


def _assert_result_match(cid: str, cpp: dict, py_lang: str, py_prob: float,
                         py_reliable: bool, py_proportion: float) -> None:
    assert cpp["language"] == py_lang, (
        f"{cid}: language mismatch cpp={cpp['language']!r} py={py_lang!r}"
    )
    assert cpp["is_reliable"] == py_reliable, (
        f"{cid}: is_reliable mismatch cpp={cpp['is_reliable']} py={py_reliable}"
    )
    assert abs(cpp["probability"] - py_prob) < FLOAT_EPS, (
        f"{cid}: probability drift cpp={cpp['probability']:.9f} "
        f"py={py_prob:.9f} diff={cpp['probability'] - py_prob:.2e}"
    )
    assert abs(cpp["proportion"] - py_proportion) < FLOAT_EPS, (
        f"{cid}: proportion drift cpp={cpp['proportion']:.9f} "
        f"py={py_proportion:.9f} diff={cpp['proportion'] - py_proportion:.2e}"
    )


def test_find_language_parity(corpus, cpp_find_language, py_detector):
    for sample in corpus:
        cid = sample["id"]
        assert cid in cpp_find_language, f"C++ runner skipped {cid}"
        cpp = cpp_find_language[cid]
        py = py_detector.FindLanguage(text=sample["text"])
        _assert_result_match(
            cid, cpp, py.language, py.probability, py.is_reliable, py.proportion
        )


def test_find_top_n_parity(corpus, cpp_find_top_n, py_detector):
    for sample in corpus:
        cid = sample["id"]
        assert cid in cpp_find_top_n, f"C++ runner skipped {cid}"
        cpp_results = cpp_find_top_n[cid]["results"]
        py_results = py_detector.FindTopNMostFreqLangs(text=sample["text"], num_langs=3)
        assert len(cpp_results) == len(py_results), (
            f"{cid}: top-N length mismatch cpp={len(cpp_results)} py={len(py_results)}"
        )
        for rank, (cpp, py) in enumerate(zip(cpp_results, py_results)):
            _assert_result_match(
                f"{cid}[rank={rank}]", cpp, py.language, py.probability,
                py.is_reliable, py.proportion,
            )


@pytest.mark.parametrize("sample_id,expected", [
    ("en_1", "en"),
    ("de_1", "de"),
    ("fr_1", "fr"),
    ("es_1", "es"),
    ("it_1", "it"),
    ("ja_1", "ja"),
    ("zh_1", "zh"),
    ("ko_1", "ko"),
    ("ru_1", "ru"),
    ("ar_1", "ar"),
])
def test_expected_language_sanity(corpus, py_detector, sample_id, expected):
    """Sanity check: the Python binding itself predicts the expected language
    for the canonical samples. Guards against the parity test passing only
    because *both* sides are broken in the same way."""
    sample = next(s for s in corpus if s["id"] == sample_id)
    result = py_detector.FindLanguage(text=sample["text"])
    assert result.language == expected, (
        f"{sample_id}: expected {expected!r}, got {result.language!r} "
        f"(probability={result.probability:.4f})"
    )
    assert result.is_reliable, f"{sample_id}: prediction marked unreliable"
