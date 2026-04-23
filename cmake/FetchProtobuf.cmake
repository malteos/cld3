# Fetch and statically build protobuf v3.21.12 (last pre-Abseil release).
#
# This keeps the build self-contained: no system protoc / libprotobuf needed
# on any platform, and the resulting wheel links protobuf-lite statically so
# there is nothing extra for auditwheel / delocate / delvewheel to bundle.

include(FetchContent)

set(_PROTOBUF_VERSION "3.21.12")

set(protobuf_BUILD_TESTS            OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_CONFORMANCE      OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_EXAMPLES         OFF CACHE BOOL "" FORCE)
set(protobuf_INSTALL                OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_SHARED_LIBS      OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_PROTOC_BINARIES  ON  CACHE BOOL "" FORCE)
set(protobuf_BUILD_LIBPROTOC        OFF CACHE BOOL "" FORCE)
set(protobuf_WITH_ZLIB              OFF CACHE BOOL "" FORCE)
set(protobuf_MSVC_STATIC_RUNTIME    OFF CACHE BOOL "" FORCE)
set(protobuf_DISABLE_RTTI           OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS               OFF)

FetchContent_Declare(
    protobuf
    GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
    GIT_TAG        v${_PROTOBUF_VERSION}
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(protobuf)

# The official protobuf CMake exports:
#   - protobuf::libprotobuf       (full library)
#   - protobuf::libprotobuf-lite  (lite runtime, what cld3 actually needs)
#   - protobuf::protoc            (compiler binary target)
# Older/newer versions sometimes miss aliases; add fallbacks.

if(NOT TARGET protobuf::libprotobuf-lite AND TARGET libprotobuf-lite)
    add_library(protobuf::libprotobuf-lite ALIAS libprotobuf-lite)
endif()

if(NOT TARGET protobuf::protoc AND TARGET protoc)
    add_executable(protobuf::protoc ALIAS protoc)
endif()

set(PROTOBUF_PROTOC_EXECUTABLE "$<TARGET_FILE:protobuf::protoc>"
    CACHE INTERNAL "Path to the protoc executable (generator expression)")

message(STATUS "CLD3: fetched protobuf v${_PROTOBUF_VERSION} (static, lite runtime)")
