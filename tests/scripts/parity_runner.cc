// parity_runner — JSON-lines wrapper around NNetLanguageIdentifier, used by
// tests/test_parity.py to verify bit-compatibility between the original C++
// reference and the Python bindings.
//
// Input  (stdin):  one JSON object per line: {"id": "...", "text": "..."}
// Output (stdout): one JSON object per line:
//   --find-language mode:
//     {"id": "...", "language": "en", "probability": 0.9997,
//      "is_reliable": true, "proportion": 1.0}
//   --find-top-n N mode:
//     {"id": "...", "results": [ {language, probability, is_reliable, proportion}, ... ]}
//
// Intentionally avoids a JSON dependency: hand-written minimal unescape/escape
// that covers ASCII + \uXXXX + UTF-8 pass-through (sufficient for our corpus).

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "nnet_language_identifier.h"

using chrome_lang_id::NNetLanguageIdentifier;

namespace {

// ------------- minimal JSON string unescape -------------
bool HexDigit(char c, int *v) {
    if (c >= '0' && c <= '9') { *v = c - '0'; return true; }
    if (c >= 'a' && c <= 'f') { *v = c - 'a' + 10; return true; }
    if (c >= 'A' && c <= 'F') { *v = c - 'A' + 10; return true; }
    return false;
}

void AppendUtf8(std::string *out, unsigned cp) {
    if (cp < 0x80) {
        out->push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        out->push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out->push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        out->push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out->push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out->push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out->push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out->push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out->push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out->push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

// Unescape a JSON string literal starting at `s[i]` which must point at the
// opening '"'. On success returns true and `i` advanced past the closing '"'.
bool ReadJsonString(const std::string &s, size_t *i, std::string *out) {
    if (*i >= s.size() || s[*i] != '"') return false;
    ++(*i);
    out->clear();
    while (*i < s.size()) {
        char c = s[*i];
        if (c == '"') { ++(*i); return true; }
        if (c == '\\') {
            if (*i + 1 >= s.size()) return false;
            char esc = s[*i + 1];
            *i += 2;
            switch (esc) {
                case '"':  out->push_back('"'); break;
                case '\\': out->push_back('\\'); break;
                case '/':  out->push_back('/'); break;
                case 'b':  out->push_back('\b'); break;
                case 'f':  out->push_back('\f'); break;
                case 'n':  out->push_back('\n'); break;
                case 'r':  out->push_back('\r'); break;
                case 't':  out->push_back('\t'); break;
                case 'u': {
                    if (*i + 4 > s.size()) return false;
                    unsigned cp = 0;
                    for (int k = 0; k < 4; ++k) {
                        int d;
                        if (!HexDigit(s[*i + k], &d)) return false;
                        cp = (cp << 4) | d;
                    }
                    *i += 4;
                    // Handle surrogate pair
                    if (cp >= 0xD800 && cp <= 0xDBFF && *i + 6 <= s.size() &&
                        s[*i] == '\\' && s[*i + 1] == 'u') {
                        unsigned low = 0;
                        for (int k = 0; k < 4; ++k) {
                            int d;
                            if (!HexDigit(s[*i + 2 + k], &d)) return false;
                            low = (low << 4) | d;
                        }
                        if (low >= 0xDC00 && low <= 0xDFFF) {
                            cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                            *i += 6;
                        }
                    }
                    AppendUtf8(out, cp);
                    break;
                }
                default: return false;
            }
        } else {
            out->push_back(c);
            ++(*i);
        }
    }
    return false;
}

// Extract the value of top-level field `"key"` from a one-line JSON object.
// Assumes well-formed input (it's ours — produced by tests/data/corpus.json).
bool FieldValue(const std::string &line, const std::string &key, std::string *out) {
    std::string needle = "\"" + key + "\"";
    size_t p = 0;
    while ((p = line.find(needle, p)) != std::string::npos) {
        size_t q = p + needle.size();
        while (q < line.size() && (line[q] == ' ' || line[q] == '\t')) ++q;
        if (q >= line.size() || line[q] != ':') { ++p; continue; }
        ++q;
        while (q < line.size() && (line[q] == ' ' || line[q] == '\t')) ++q;
        if (q >= line.size() || line[q] != '"') { ++p; continue; }
        return ReadJsonString(line, &q, out);
    }
    return false;
}

// ------------- minimal JSON string escape -------------
void WriteJsonString(std::ostream &os, const std::string &s) {
    os << '"';
    for (char c : s) {
        unsigned char u = static_cast<unsigned char>(c);
        switch (c) {
            case '"':  os << "\\\""; break;
            case '\\': os << "\\\\"; break;
            case '\b': os << "\\b"; break;
            case '\f': os << "\\f"; break;
            case '\n': os << "\\n"; break;
            case '\r': os << "\\r"; break;
            case '\t': os << "\\t"; break;
            default:
                if (u < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", u);
                    os << buf;
                } else {
                    os << c;
                }
        }
    }
    os << '"';
}

void WriteResult(std::ostream &os, const NNetLanguageIdentifier::Result &r) {
    os << "{\"language\":";
    WriteJsonString(os, r.language);
    os << ",\"probability\":" << std::setprecision(9) << r.probability
       << ",\"is_reliable\":" << (r.is_reliable ? "true" : "false")
       << ",\"proportion\":"  << std::setprecision(9) << r.proportion
       << "}";
}

void Usage(const char *prog) {
    std::fprintf(stderr,
        "usage: %s --find-language\n"
        "       %s --find-top-n N\n"
        "\n"
        "Reads JSON lines from stdin: {\"id\":\"...\",\"text\":\"...\"}\n"
        "Writes JSON lines to stdout (see parity_runner.cc header).\n",
        prog, prog);
}

}  // namespace

int main(int argc, char **argv) {
    enum class Mode { kFindLanguage, kFindTopN };
    Mode mode = Mode::kFindLanguage;
    int top_n = 3;

    if (argc < 2) { Usage(argv[0]); return 2; }
    if (std::strcmp(argv[1], "--find-language") == 0) {
        mode = Mode::kFindLanguage;
    } else if (std::strcmp(argv[1], "--find-top-n") == 0) {
        mode = Mode::kFindTopN;
        if (argc < 3) { Usage(argv[0]); return 2; }
        top_n = std::atoi(argv[2]);
        if (top_n <= 0) { Usage(argv[0]); return 2; }
    } else {
        Usage(argv[0]);
        return 2;
    }

    NNetLanguageIdentifier lang_id(/*min_num_bytes=*/0,
                                   /*max_num_bytes=*/1000);

    std::cout << std::fixed;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        std::string id, text;
        if (!FieldValue(line, "id", &id))   { std::fprintf(stderr, "bad line (no id): %s\n", line.c_str()); return 3; }
        if (!FieldValue(line, "text", &text)) { std::fprintf(stderr, "bad line (no text): %s\n", line.c_str()); return 3; }

        if (mode == Mode::kFindLanguage) {
            auto r = lang_id.FindLanguage(text);
            std::cout << "{\"id\":";
            WriteJsonString(std::cout, id);
            std::cout << ",\"language\":";
            WriteJsonString(std::cout, r.language);
            std::cout << ",\"probability\":" << std::setprecision(9) << r.probability
                      << ",\"is_reliable\":" << (r.is_reliable ? "true" : "false")
                      << ",\"proportion\":"  << std::setprecision(9) << r.proportion
                      << "}\n";
        } else {
            auto results = lang_id.FindTopNMostFreqLangs(text, top_n);
            std::cout << "{\"id\":";
            WriteJsonString(std::cout, id);
            std::cout << ",\"results\":[";
            for (size_t k = 0; k < results.size(); ++k) {
                if (k) std::cout << ',';
                WriteResult(std::cout, results[k]);
            }
            std::cout << "]}\n";
        }
    }

    return 0;
}
