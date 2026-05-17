cmake_minimum_required(VERSION 3.16)
project(drv_compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Compiler sources ──────────────────────────────────────────────────────────
set(DRV_SOURCES
    compiler/src/lexer.cpp
    compiler/src/parser.cpp
    compiler/src/codegen.cpp
    compiler/src/compiler.cpp
)

# ── drv compiler executable ───────────────────────────────────────────────────
add_executable(drv main.cpp ${DRV_SOURCES})
target_include_directories(drv PRIVATE ${CMAKE_SOURCE_DIR})

# ── Compiler warnings ─────────────────────────────────────────────────────────
target_compile_options(drv PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wno-unused-parameter>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /wd4100>
)

# ── OpenMP (parallel for reduction) ──────────────────────────────────────────
find_package(OpenMP QUIET)
if(OpenMP_CXX_FOUND)
    target_link_libraries(drv PRIVATE OpenMP::OpenMP_CXX)
    target_compile_definitions(drv PRIVATE DRV_OPENMP)
endif()

# ── Filesystem support (GCC < 9) ─────────────────────────────────────────────
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
    target_link_libraries(drv PRIVATE stdc++fs)
endif()

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS drv RUNTIME DESTINATION bin)

message(STATUS "drv compiler — C++${CMAKE_CXX_STANDARD}")
