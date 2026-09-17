# firebase-telemetry-persistence

Firebase Telemetry Persistence Library

## Project Structure

This library is organized to facilitate local development and cross-platform interop for Android and iOS

- `include/`: Public headers for interop. These define the interface consumed by the SDKs
- `src/`: Implementation of the library, including private headers and source files
- `tests/`: Unit tests designed to run on a developer's machine
- `main.cpp`: A scratchpad for local development and experimentation

Only the core logic in `include/` and `src/` is compiled into the final production library. Development artifacts like
`tests/` and `main.cpp` are used strictly for local verification and are never linked into the final binary

---

## Local Development

Use these instructions to work on the library logic itself on your host machine

### Prerequisites

- A C++17 compliant compiler
- CMake (version 3.24 or higher)

### Building

To build the library, the local scratchpad, and the unit tests:

```shell
cmake -S . -B build
cmake --build build
```

### Running Tests

Unit tests use GoogleTest:

```shell
ctest --test-dir build --output-on-failure
```

Alternatively, you can run the test executable directly:

```shell
./build/unit_tests
```

### Memory Leak Detection

#### ASan (Linux)

Build the project with `ENABLE_ASAN` enabled and run it normally on Linux:

```bash
cmake -S . -B build -DENABLE_ASAN=ON 
cmake --build build

./build/unit_tests
./build/main
```

#### Leaks (macOS)

Build the project with `ENABLE_FRAME_POINTER` enabled, and use `leaks` to run it on macOS:

```bash
cmake -S . -B build -DENABLE_FRAME_POINTER=ON 
cmake --build build

leaks --atExit -- ./build/unit_tests
leaks --atExit -- ./build/main
```

### Running the Scratchpad

Use `main.cpp` for quick testing:

```shell
./build/main
```

---

## Android Integration

Use these instructions to build and link the library for use in an Android application

### Prerequisites

- Android NDK (Version 28.2.13676358 or similar)

### Building for Android

To build the shared library for Android using the NDK toolchain:

```shell
cmake -S . -B android-build \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DCMAKE_TOOLCHAIN_FILE=~/Library/Android/sdk/ndk/28.2.13676358/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-24

cmake --build android-build --target firebase-telemetry-persistence
```

### Integrating into an Android Project

To integrate this library in another Android project, add this to your app's `build.gradle.kts`:

```kotlin
android {
  ...
  externalNativeBuild {
    cmake {
      path = file("firebase-telemetry-persistence/CMakeLists.txt")
      version = "3.22.1"
    }
  }
}
```

Where `firebase-telemetry-persistence/CMakeLists.txt` is the local path to this library

---

## iOS Integration

Verify building it:

```shell
swift build
```

Verify compatibility of C++ changes

```shell
swift test
```

Update Package.swift:

```swift
let package = Package(
  ...
  dependencies: [
    // Without local changes
    .package(
      url: "<TODO: insert git link>",
      branch: "main"),
    // With local changes
    .package(name: "firebase-telemetry-persistence", path: "../../../firebase-telemetry-persistence"),
  ],
  targets: [
    .target(
      dependencies: [
      ...
        .product(name: "FirebaseTelemetryPersistence", package: "firebase-telemetry-persistence"),
      ],
      swiftSettings: [
        .interoperabilityMode(.Cxx)
      ]
    ),
  ],
  cxxLanguageStandard: .cxx17
)
```

## Source Code Headers

Every file containing source code must include copyright and license
information. This includes any JS/CSS files that you might be serving out to
browsers. (This is to help well-intentioned people avoid accidental copying that
doesn't comply with the license.)

Apache header:

    Copyright 2024 Google LLC

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        https://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
