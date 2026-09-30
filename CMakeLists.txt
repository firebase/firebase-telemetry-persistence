# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.22)
project(firebase-telemetry-persistence VERSION 0.1.0 LANGUAGES CXX)

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif ()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_ASAN "Enable Asan" OFF)
option(ENABLE_FRAME_POINTER "Enable Leaks" OFF)

set(FRAME_POINTER_FLAG "-fomit-frame-pointer")
if (ENABLE_ASAN)
    add_compile_options(-fsanitize=undefined -fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=undefined -fsanitize=address)
endif ()
if (ENABLE_FRAME_POINTER OR ENABLE_ASAN)
    set(FRAME_POINTER_FLAG "-fno-omit-frame-pointer")
endif ()

add_library(firebase-telemetry-persistence SHARED
        src/header.cpp
        src/initialize.cpp
        src/mutable_span_data.cpp
        src/recovered_span_data.cpp
        src/span_data_impl.cpp
)

target_compile_options(firebase-telemetry-persistence PRIVATE
        -Wall -Wextra -Wpedantic -Werror
        -Os
        -fPIC
        -fdata-sections
        -ffast-math
        -ffunction-sections
        -flto
        -fmerge-all-constants
        -fno-asynchronous-unwind-tables
        -fno-common
        -fno-exceptions
        -fno-ident
        -fno-math-errno
        -fno-rtti
        -fno-unroll-loops
        -fno-unwind-tables
        -fvisibility=hidden
        ${FRAME_POINTER_FLAG}
)

target_include_directories(firebase-telemetry-persistence PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if (ANDROID)
    target_sources(firebase-telemetry-persistence PRIVATE
            src/android/android_span_data.cpp
            src/android/span_jclass_cache.cpp
    )

    target_link_options(firebase-telemetry-persistence PRIVATE
            -flto
            -fuse-ld=lld
            -s
            "LINKER:--exclude-libs,ALL"
            "LINKER:--gc-sections"
    )
endif ()

if (NOT ANDROID)
    add_executable(main
            main.cpp
            src/initialize.cpp
            src/span_data_impl.cpp
    )
    target_link_libraries(main PRIVATE firebase-telemetry-persistence)

    include(FetchContent)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip # v1.17.0
            URL_HASH SHA256=9a56a54ae784394ff664cd55e8f4c9a03b503ebf0cb99576321c78ab3d87ca84
    )
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(unit_tests
            src/header.cpp # visible for testing
            src/initialize.cpp # visible for testing
            src/span_data_impl.cpp # visible for testing
            tests/copy_string_tests.cpp
            tests/header_tests.cpp
            tests/initialize_tests.cpp
            tests/managed_mmap_tests.cpp
            tests/memory_layout_tests.cpp
            tests/mutable_span_data_tests.cpp
            tests/page_size_tests.cpp
            tests/raw_span_data_tests.cpp
            tests/recovered_span_data_tests.cpp
            tests/span_data_impl_tests.cpp
            tests/span_tests.cpp
    )
    target_link_libraries(unit_tests PRIVATE firebase-telemetry-persistence GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(unit_tests)
endif ()
