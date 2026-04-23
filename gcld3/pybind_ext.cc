// Python bindings for chrome_lang_id::NNetLanguageIdentifier.
//
// The Python import path is `gcld3.pybind_ext`; `gcld3/__init__.py` re-exports
// the public names. The API is intentionally source-compatible with the
// original `gcld3` package so existing user code (`import gcld3;
// gcld3.NNetLanguageIdentifier(...)`) keeps working unchanged.

#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

#include "nnet_language_identifier.h"

namespace py = pybind11;

using chrome_lang_id::NNetLanguageIdentifier;

#ifndef CLD3_MODULE_VERSION
#define CLD3_MODULE_VERSION "0.0.0+unknown"
#endif

PYBIND11_MODULE(pybind_ext, m) {
    m.doc() = "Compact Language Detector v3 (CLD3) Python bindings.";
    m.attr("__version__") = CLD3_MODULE_VERSION;

    py::class_<NNetLanguageIdentifier::Result>(m, "Result")
        .def(py::init<>())
        .def_readwrite("language",    &NNetLanguageIdentifier::Result::language)
        .def_readwrite("probability", &NNetLanguageIdentifier::Result::probability)
        .def_readwrite("is_reliable", &NNetLanguageIdentifier::Result::is_reliable)
        .def_readwrite("proportion",  &NNetLanguageIdentifier::Result::proportion)
        .def("__repr__", [](const NNetLanguageIdentifier::Result &r) {
            return "<Result language='" + r.language +
                   "' probability=" + std::to_string(r.probability) +
                   " is_reliable=" + (r.is_reliable ? "True" : "False") +
                   " proportion=" + std::to_string(r.proportion) + ">";
        });

    py::class_<NNetLanguageIdentifier>(m, "NNetLanguageIdentifier")
        .def(py::init<int, int>(),
             py::arg("min_num_bytes"),
             py::arg("max_num_bytes"))
        .def("FindLanguage",
             [](NNetLanguageIdentifier &self, const std::string &text) {
                 py::gil_scoped_release release;
                 return self.FindLanguage(text);
             },
             py::arg("text"),
             "Return the most likely language for `text`.")
        .def("FindTopNMostFreqLangs",
             [](NNetLanguageIdentifier &self, const std::string &text, int num_langs) {
                 py::gil_scoped_release release;
                 return self.FindTopNMostFreqLangs(text, num_langs);
             },
             py::arg("text"), py::arg("num_langs"),
             "Return up to `num_langs` most-frequent languages detected in `text`.")
        .def_property_readonly_static("kUnknown",
                                      [](py::object) {
                                          return std::string(NNetLanguageIdentifier::kUnknown);
                                      })
        .def_readonly_static("kMinNumBytesToConsider",
                             &NNetLanguageIdentifier::kMinNumBytesToConsider)
        .def_readonly_static("kMaxNumBytesToConsider",
                             &NNetLanguageIdentifier::kMaxNumBytesToConsider)
        .def_readonly_static("kMaxNumInputBytesToConsider",
                             &NNetLanguageIdentifier::kMaxNumInputBytesToConsider)
        .def_readonly_static("kReliabilityThreshold",
                             &NNetLanguageIdentifier::kReliabilityThreshold)
        .def_readonly_static("kReliabilityHrBsThreshold",
                             &NNetLanguageIdentifier::kReliabilityHrBsThreshold);
}
