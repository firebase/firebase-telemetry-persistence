// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

import CxxStdlib
import Foundation
import Testing

@testable import FirebaseTelemetryPersistence
@testable import OpenTelemetryApi
@testable import OpenTelemetrySdk

/// The native C++ span structure used for cross-language interoperability.
typealias CppSpan = firebase.telemetry.persistence.Span

/// The native C++ span buffer utilized for managing active, in-flight spans.
typealias CppSpanBuffer = firebase.telemetry.persistence.MutableSpanData

/// The native C++ span TraceId.
typealias CppTraceId = firebase.telemetry.persistence.TraceId

struct SwiftCompatibilityTests {

  @Test
  func testInit() async throws {
    let attributes = ["k1": "v1", "k2": "v2"]
    let spanData = createRandomSpanData(
      spanName: "testSpan", attributes: attributes, hasParent: true)

    var cppAttributes = firebase.telemetry.persistence.AttributesList()
    for (key, value) in spanData.attributes {
      cppAttributes.push_back(.init(first: std.string(key), second: std.string(value.description)))
    }
    let startTimeNanoseconds = UInt64(spanData.startTime.timeIntervalSince1970 * 1_000_000_000)
    let endTimeNanoseconds = UInt64(spanData.endTime.timeIntervalSince1970 * 1_000_000_000)

    let span = CppSpan(
      CppTraceId(
        high: spanData.traceId.idHi, low: spanData.traceId.idLo),
      spanData.spanId.rawValue,
      spanData.parentSpanId?.rawValue ?? 0,
      startTimeNanoseconds,
      endTimeNanoseconds,
      std.string(spanData.name),
      cppAttributes
    )

    #expect(span.trace_id().high == spanData.traceId.idHi)
    #expect(span.trace_id().low == spanData.traceId.idLo)
    #expect(span.span_id() == spanData.spanId.rawValue)
    #expect(span.parent_span_id() == spanData.parentSpanId?.rawValue)
    #expect(span.start_time() == spanData.startTime.timeIntervalSince1970.toNanoseconds)
    #expect(span.end_time() == spanData.endTime.timeIntervalSince1970.toNanoseconds)
    #expect(span.name() == "testSpan")
    #expect(span.attributes().size() == attributes.count)
    for pair in span.attributes() {
      #expect(attributes[String(pair.first)] == String(pair.second))
    }
  }

  private func createRandomSpanData(
    spanName: String = "spanName", attributes: [String: String] = ["k1": "v1"],
    hasParent: Bool = false
  ) -> SpanData {
    let attributes = attributes.mapValues({ value in
      return AttributeValue(value)
    })

    return SpanData(
      traceId: TraceId.random(),
      spanId: SpanId.random(),
      traceFlags: TraceFlags(),
      traceState: TraceState(),
      parentSpanId: hasParent ? SpanId.random() : nil,
      resource: Resource(),
      instrumentationScope: InstrumentationScopeInfo(),
      name: spanName,
      kind: .client,
      startTime: Date(timeIntervalSince1970: 1_000_000_000 + 100),
      attributes: attributes,
      endTime: Date(timeIntervalSince1970: 2_000_000_000 + 200),
      hasRemoteParent: false,
      hasEnded: false)
  }
}
