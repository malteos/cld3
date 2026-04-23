# Compact Language Detector v3 (CLD3) — Python bindings

[![PyPI](https://img.shields.io/pypi/v/cld3-py.svg)](https://pypi.org/project/cld3-py/)
[![Python Versions](https://img.shields.io/pypi/pyversions/cld3-py.svg)](https://pypi.org/project/cld3-py/)

Modernised Python bindings for Google's CLD3 neural language detector, distributed on PyPI as
[`cld3-py`](https://pypi.org/project/cld3-py/). Prebuilt wheels are published for:

| Platform                | Architectures                   |
| ----------------------- | ------------------------------- |
| Linux (manylinux_2_28)  | `x86_64`, `aarch64`             |
| Linux (musllinux_1_2)   | `x86_64`, `aarch64`             |
| macOS                   | `x86_64` (Intel), `arm64` (Apple Silicon) |
| Windows                 | `AMD64`                         |

Supported Python versions: **3.10, 3.11, 3.12, 3.13, 3.14** (including free-threaded `cp313t` / `cp314t` builds).

## Install

```shell
pip install cld3-py
```

## Quick start

The import name is `gcld3`, so code written against the original Google `gcld3` package works unchanged:

```python
import gcld3

detector = gcld3.NNetLanguageIdentifier(min_num_bytes=0, max_num_bytes=1000)

result = detector.FindLanguage(text="This text is written in English.")
print(result.language)     # "en"
print(result.probability)  # ~0.999
print(result.is_reliable)  # True
print(result.proportion)   # 1.0

# Multiple languages in the same document:
sample = "This piece of text is in English. Този текст е на Български."
for r in detector.FindTopNMostFreqLangs(text=sample, num_langs=2):
    print(r.language, r.probability, r.proportion)
```

### Drop-in replacement for `gcld3`

If you are migrating from the old PyPI `gcld3` package:

```shell
pip uninstall gcld3
pip install cld3-py
# No code changes required — `import gcld3` still works.
```

## Table of contents

* [Model](#model)
* [Supported Languages](#supported-languages)
* [Building from source](#building-from-source)
* [Verification / parity](#verification--parity)
* [Bugs and Feature Requests](#bugs-and-feature-requests)
* [Credits](#credits)

### Model

CLD3 is a neural network model for language identification. This package
 contains the inference code and a trained model. The inference code
 extracts character ngrams from the input text and computes the fraction
 of times each of them appears. For example, as shown in the figure below,
 if the input text is "banana", then one of the extracted trigrams is "ana"
 and the corresponding fraction is 2/4. The ngrams are hashed down to an id
 within a small range, and each id is represented by a dense embedding vector
 estimated during training.

The model averages the embeddings corresponding to each ngram type according
 to the fractions, and the averaged embeddings are concatenated to produce
 the embedding layer. The remaining components of the network are a hidden
 (Rectified linear) layer and a softmax layer.

To get a language prediction for the input text, we simply perform a forward
 pass through the network.

![Figure](model.png "CLD3")

### Supported Languages

The model outputs BCP-47-style language codes, shown in the table below. For
some languages, output is differentiated by script. Language and script names
from
[Unicode CLDR](https://github.com/unicode-cldr/cldr-localenames-modern/blob/master/main/en).

Output Code | Language Name   | Script Name
----------- | --------------- | ------------------------------------------
af          | Afrikaans       | Latin
am          | Amharic         | Ethiopic
ar          | Arabic          | Arabic
bg          | Bulgarian       | Cyrillic
bg-Latn     | Bulgarian       | Latin
bn          | Bangla          | Bangla
bs          | Bosnian         | Latin
ca          | Catalan         | Latin
ceb         | Cebuano         | Latin
co          | Corsican        | Latin
cs          | Czech           | Latin
cy          | Welsh           | Latin
da          | Danish          | Latin
de          | German          | Latin
el          | Greek           | Greek
el-Latn     | Greek           | Latin
en          | English         | Latin
eo          | Esperanto       | Latin
es          | Spanish         | Latin
et          | Estonian        | Latin
eu          | Basque          | Latin
fa          | Persian         | Arabic
fi          | Finnish         | Latin
fil         | Filipino        | Latin
fr          | French          | Latin
fy          | Western Frisian | Latin
ga          | Irish           | Latin
gd          | Scottish Gaelic | Latin
gl          | Galician        | Latin
gu          | Gujarati        | Gujarati
ha          | Hausa           | Latin
haw         | Hawaiian        | Latin
hi          | Hindi           | Devanagari
hi-Latn     | Hindi           | Latin
hmn         | Hmong           | Latin
hr          | Croatian        | Latin
ht          | Haitian Creole  | Latin
hu          | Hungarian       | Latin
hy          | Armenian        | Armenian
id          | Indonesian      | Latin
ig          | Igbo            | Latin
is          | Icelandic       | Latin
it          | Italian         | Latin
iw          | Hebrew          | Hebrew
ja          | Japanese        | Japanese
ja-Latn     | Japanese        | Latin
jv          | Javanese        | Latin
ka          | Georgian        | Georgian
kk          | Kazakh          | Cyrillic
km          | Khmer           | Khmer
kn          | Kannada         | Kannada
ko          | Korean          | Korean
ku          | Kurdish         | Latin
ky          | Kyrgyz          | Cyrillic
la          | Latin           | Latin
lb          | Luxembourgish   | Latin
lo          | Lao             | Lao
lt          | Lithuanian      | Latin
lv          | Latvian         | Latin
mg          | Malagasy        | Latin
mi          | Maori           | Latin
mk          | Macedonian      | Cyrillic
ml          | Malayalam       | Malayalam
mn          | Mongolian       | Cyrillic
mr          | Marathi         | Devanagari
ms          | Malay           | Latin
mt          | Maltese         | Latin
my          | Burmese         | Myanmar
ne          | Nepali          | Devanagari
nl          | Dutch           | Latin
no          | Norwegian       | Latin
ny          | Nyanja          | Latin
pa          | Punjabi         | Gurmukhi
pl          | Polish          | Latin
ps          | Pashto          | Arabic
pt          | Portuguese      | Latin
ro          | Romanian        | Latin
ru          | Russian         | Cyrillic
ru-Latn     | Russian         | English
sd          | Sindhi          | Arabic
si          | Sinhala         | Sinhala
sk          | Slovak          | Latin
sl          | Slovenian       | Latin
sm          | Samoan          | Latin
sn          | Shona           | Latin
so          | Somali          | Latin
sq          | Albanian        | Latin
sr          | Serbian         | Cyrillic
st          | Southern Sotho  | Latin
su          | Sundanese       | Latin
sv          | Swedish         | Latin
sw          | Swahili         | Latin
ta          | Tamil           | Tamil
te          | Telugu          | Telugu
tg          | Tajik           | Cyrillic
th          | Thai            | Thai
tr          | Turkish         | Latin
uk          | Ukrainian       | Cyrillic
ur          | Urdu            | Arabic
uz          | Uzbek           | Latin
vi          | Vietnamese      | Latin
xh          | Xhosa           | Latin
yi          | Yiddish         | Hebrew
yo          | Yoruba          | Latin
zh          | Chinese         | Han (including Simplified and Traditional)
zh-Latn     | Chinese         | Latin
zu          | Zulu            | Latin

### Building from source

Most users should just `pip install cld3-py`. To build from source (e.g. for an unreleased Python version or
to work on the bindings):

```shell
git clone https://github.com/malteos/cld3
cd cld3
pip install -v .
```

The build uses `scikit-build-core` + CMake. Protobuf v3.21.12 is fetched and statically linked via CMake
`FetchContent`, so no system `protoc` / `libprotobuf` is required.

To additionally build the reference C++ CLI binaries:

```shell
cmake -S . -B build -DCLD3_BUILD_TOOLS=ON
cmake --build build -j
./build/language_identifier_main
```

### Verification / parity

A parity test (`tests/test_parity.py`) verifies that the Python bindings produce the same predictions as the
original C++ reference for ~75 multilingual samples. Run it locally with:

```shell
cmake -S . -B build -DCLD3_BUILD_TOOLS=ON && cmake --build build -j
CLD3_PARITY_BINARY=$PWD/build/parity_runner pytest tests/test_parity.py -v
```

The same test runs in CI after every wheel build.

### Bugs and Feature Requests

Open a [GitHub issue](https://github.com/google/cld3/issues) for this repository to file bugs and feature requests.

### Announcements and Discussion

For announcements regarding major updates as well as general discussion list, please subscribe to:
[cld3-users@googlegroups.com](https://groups.google.com/forum/#!forum/cld3-users)

### Credits

Original authors of the code in this package include (in alphabetical order):

* Alex Salcianu
* Andy Golding
* Anton Bakalov
* Chris Alberti
* Daniel Andor
* David Weiss
* Emily Pitler
* Greg Coppola
* Jason Riesa
* Kuzman Ganchev
* Michael Ringgaard
* Nan Hua
* Ryan McDonald
* Slav Petrov
* Stefan Istrate
* Terry Koo
