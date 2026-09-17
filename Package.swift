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
  platforms: [
    .iOS(.v13)
  ],
  products: [
    .library(
      name: "FirebaseTelemetryPersistence",
      targets: ["FirebaseTelemetryPersistence"]
    )
  ],
  dependencies: [
    .package(
      url: "https://github.com/open-telemetry/opentelemetry-swift-core.git",
      .upToNextMajor(from: "2.3.0"))
  ],
  targets: [
    .target(
      name: "FirebaseTelemetryPersistence",
      path: ".",
      exclude: [
        "CMakeLists.txt",
        "README.md",
        "LICENSE",
        "tests",
        "src/android_span_data.cpp",
      ],
      sources: [
        "src/header.cpp",
        "src/initialize.cpp",
        "src/mutable_span_data.cpp",
        "src/recovered_span_data.cpp",
        "src/span_data_impl.cpp",
      ],
      publicHeadersPath: "include"
        // TODO: Add compiler and linker flags if needed as defined in CmakeLists.txt.
    ),
    .testTarget(
      name: "SwiftCompatibilityTests",
      dependencies: [
        "FirebaseTelemetryPersistence",
        .product(name: "OpenTelemetryApi", package: "opentelemetry-swift-core"),
        .product(name: "OpenTelemetrySdk", package: "opentelemetry-swift-core"),
      ],
      path: ".",
      exclude: [
        "CMakeLists.txt",
        "README.md",
        "LICENSE",
        "src",
        "include",
      ],
      sources: ["tests/swift_compatibility_test.swift"],
      swiftSettings: [
        .interoperabilityMode(.Cxx)
      ]
    ),
  ],
  cxxLanguageStandard: .cxx17
)
