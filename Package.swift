// swift-tools-version: 6.2

// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

import PackageDescription

let package = Package(
  name: "FirebaseTelemetryPersistence",
  platforms: [.iOS(.v15), .macCatalyst(.v15), .macOS(.v11), .tvOS(.v15), .watchOS(.v8)],
  products: [
    .library(
      name: "FirebaseTelemetryPersistence",
      targets: ["FirebaseTelemetryPersistence"]
    )
  ],
  targets: [
    .target(
      name: "FirebaseTelemetryPersistence",
      path: ".",
      exclude: [
        "CMakeLists.txt",
        "main.cpp",
        "README.md",
        "LICENSE",
        "tests",
        "docs",
        "src/android_span_data.cpp",
      ],
      sources: ["src"]
        // TODO: Add compiler and linker flags if needed as defined in CmakeLists.txt.
    ),
  ],
  cxxLanguageStandard: .cxx17
)
