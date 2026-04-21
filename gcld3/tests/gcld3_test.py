"""Smoke tests for gcld3 — used by cibuildwheel CIBW_TEST_COMMAND."""

import pytest

import gcld3


def test_module_version():
    assert isinstance(gcld3.__version__, str)
    assert len(gcld3.__version__) > 0


def test_lang_identification_english():
    detector = gcld3.NNetLanguageIdentifier(min_num_bytes=0, max_num_bytes=1000)
    result = detector.FindLanguage(text="This text is written in English.")
    assert result.language == "en"
    assert result.is_reliable
    assert result.proportion > 0.99
    assert result.probability > 0.90


def test_empty_string():
    detector = gcld3.NNetLanguageIdentifier(min_num_bytes=10, max_num_bytes=1000)
    result = detector.FindLanguage(text="")
    assert result.language == "und"
    assert not result.is_reliable
    assert result.proportion == 0.0
    assert result.probability == 0.0


def test_top_n_mixed_en_bg():
    detector = gcld3.NNetLanguageIdentifier(min_num_bytes=0, max_num_bytes=1000)
    sample = "This piece of text is in English. Този текст е на Български."
    results = detector.FindTopNMostFreqLangs(text=sample, num_langs=2)
    assert len(results) == 2
    assert results[0].language == "bg"
    assert results[0].is_reliable
    assert results[0].proportion < 0.75
    assert results[0].probability > 0.90
    assert results[1].language == "en"
    assert results[1].is_reliable
    assert results[1].proportion < 0.75
    assert results[1].probability > 0.90


def test_result_repr():
    detector = gcld3.NNetLanguageIdentifier(min_num_bytes=0, max_num_bytes=1000)
    result = detector.FindLanguage(text="Hello world")
    r = repr(result)
    assert "Result" in r
    assert "language=" in r


def test_static_constants():
    assert gcld3.NNetLanguageIdentifier.kUnknown == "und"
    assert isinstance(gcld3.NNetLanguageIdentifier.kMinNumBytesToConsider, int)
    assert isinstance(gcld3.NNetLanguageIdentifier.kMaxNumBytesToConsider, int)
    assert isinstance(gcld3.NNetLanguageIdentifier.kReliabilityThreshold, float)


if __name__ == "__main__":
    raise SystemExit(pytest.main([__file__, "-v"]))
