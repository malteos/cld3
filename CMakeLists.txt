cmake_minimum_required(VERSION 3.26)

project(cld3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option(CLD3_FETCH_PROTOBUF "Fetch and statically link protobuf via FetchContent" ON)
option(CLD3_BUILD_TOOLS    "Build CLI tools: language_identifier_main + parity_runner" OFF)
option(CLD3_BUILD_PYTHON   "Build the pybind11 Python extension" ON)

# ---------------------------------------------------------------------------
# Global compiler hygiene — applies to every target, including the protobuf
# library we fetch. Must be set BEFORE FetchContent_MakeAvailable(protobuf)
# so protobuf's sources inherit them.
# ---------------------------------------------------------------------------
if(MSVC)
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        _SCL_SECURE_NO_WARNINGS
        _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
        _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    add_compile_options(/bigobj /utf-8)
    # Select a consistent MSVC runtime across our library and protobuf.
    if(POLICY CMP0091)
        cmake_policy(SET CMP0091 NEW)
    endif()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

# ---------------------------------------------------------------------------
# Protobuf
# ---------------------------------------------------------------------------
if(CLD3_FETCH_PROTOBUF)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchProtobuf.cmake)
else()
    find_package(Protobuf REQUIRED)
    if(NOT DEFINED PROTOBUF_PROTOC_EXECUTABLE)
        set(PROTOBUF_PROTOC_EXECUTABLE ${Protobuf_PROTOC_EXECUTABLE})
    endif()
endif()

# ---------------------------------------------------------------------------
# Generate .pb.{cc,h} into ${CMAKE_BINARY_DIR}/cld_3/protos/ so the existing
# '#include "cld_3/protos/sentence.pb.h"' style keeps working unchanged.
# ---------------------------------------------------------------------------
include(${CMAKE_CURRENT_SOURCE_DIR}/misc/myprotobuf.cmake)
# Tell the helper to add each .proto file's directory as a proto include path
# so the output files drop the leading `src/` segment and land at
# `${CMAKE_BINARY_DIR}/cld_3/protos/<name>.pb.{cc,h}` (matching the
# `#include "cld_3/protos/..."` paths used throughout the C++ sources).
set(PROTOBUF_GENERATE_CPP_APPEND_PATH ON)
my_protobuf_generate_cpp(
    cld_3/protos
    PROTO_SRCS PROTO_HDRS
    src/feature_extractor.proto
    src/sentence.proto
    src/task_spec.proto
)

# ---------------------------------------------------------------------------
# cld3_core static library — all the C++ sources needed for inference.
# ---------------------------------------------------------------------------
set(CLD3_CORE_SRCS
    src/base.cc
    src/embedding_feature_extractor.cc
    src/embedding_network.cc
    src/feature_extractor.cc
    src/feature_types.cc
    src/fml_parser.cc
    src/lang_id_nn_params.cc
    src/language_identifier_features.cc
    src/nnet_language_identifier.cc
    src/registry.cc
    src/relevant_script_feature.cc
    src/sentence_features.cc
    src/task_context.cc
    src/task_context_params.cc
    src/unicodetext.cc
    src/utils.cc
    src/workspace.cc
    src/script_span/fixunicodevalue.cc
    src/script_span/generated_entities.cc
    src/script_span/generated_ulscript.cc
    src/script_span/getonescriptspan.cc
    src/script_span/offsetmap.cc
    src/script_span/text_processing.cc
    src/script_span/utf8statetable.cc
)

add_library(cld3_core STATIC ${CLD3_CORE_SRCS} ${PROTO_SRCS} ${PROTO_HDRS})

target_include_directories(cld3_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(cld3_core PUBLIC protobuf::libprotobuf-lite)

# Per-compiler flags. Scoped to cld3_core only; don't pollute dependents.
if(MSVC)
    target_compile_options(cld3_core PRIVATE
        /wd4267  # size_t -> int narrowing (legacy code)
        /wd4244  # conversion narrowing
        /wd4996  # deprecated POSIX names
        /wd4018  # signed/unsigned mismatch
        /wd4146  # unary minus on unsigned
    )
else()
    target_compile_options(cld3_core PRIVATE
        -Wno-narrowing
        -Wno-deprecated-declarations
        -fvisibility=hidden
        -fvisibility-inlines-hidden
    )
endif()

# ---------------------------------------------------------------------------
# Python extension (pybind11)
# ---------------------------------------------------------------------------
if(CLD3_BUILD_PYTHON)
    # scikit-build-core sets Python_EXECUTABLE and locates pybind11 via its
    # build-system.requires; plain cmake runs also work if pybind11 is installed.
    find_package(Python 3.10 COMPONENTS Interpreter Development.Module REQUIRED)
    find_package(pybind11 CONFIG REQUIRED)

    pybind11_add_module(pybind_ext MODULE
        gcld3/pybind_ext.cc
    )
    target_link_libraries(pybind_ext PRIVATE cld3_core)
    target_compile_definitions(pybind_ext PRIVATE
        CLD3_MODULE_VERSION="3.1.0"
    )

    # Install the extension into the gcld3/ package directory inside the wheel.
    install(TARGETS pybind_ext
        LIBRARY DESTINATION gcld3
        RUNTIME DESTINATION gcld3
    )
endif()

# ---------------------------------------------------------------------------
# Optional CLI tools (only for local development / CI parity testing)
# ---------------------------------------------------------------------------
if(CLD3_BUILD_TOOLS)
    add_executable(language_identifier_main src/language_identifier_main.cc)
    target_link_libraries(language_identifier_main PRIVATE cld3_core)

    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/scripts/parity_runner.cc)
        add_executable(parity_runner tests/scripts/parity_runner.cc)
        target_link_libraries(parity_runner PRIVATE cld3_core)
    endif()
endif()
