cmake_minimum_required(VERSION 3.16)
project(dri_compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Compiler sources ──────────────────────────────────────────────────────────
set(DRI_SOURCES
        compiler/src/lexer.cpp
        compiler/src/parser.cpp
        compiler/src/codegen.cpp
        compiler/src/compiler.cpp
)

# ── dri compiler executable ───────────────────────────────────────────────────
# 타겟 이름을 dri로 변경하여 dri.exe가 빌드되도록 설정합니다.
add_executable(dri main.cpp ${DRI_SOURCES})
target_include_directories(dri PRIVATE ${CMAKE_SOURCE_DIR})

# ── Compiler warnings ─────────────────────────────────────────────────────────
target_compile_options(dri PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wno-unused-parameter>
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /wd4100>
)

# ── OpenMP (parallel for reduction) ──────────────────────────────────────────
find_package(OpenMP QUIET)
if(OpenMP_CXX_FOUND)
    target_link_libraries(dri PRIVATE OpenMP::OpenMP_CXX)
    target_compile_definitions(dri PRIVATE DRI_OPENMP) # (선택) 정의명도 DRI_OPENMP로 변경
endif()

# ── Filesystem support (GCC < 9) ─────────────────────────────────────────────
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
    target_link_libraries(dri PRIVATE stdc++fs)
endif()

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS dri RUNTIME DESTINATION bin)

message(STATUS "dri compiler — C++${CMAKE_CXX_STANDARD}")