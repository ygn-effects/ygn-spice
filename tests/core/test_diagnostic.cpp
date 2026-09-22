#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <string>
#include <type_traits>

#include "diagnostic.hpp"

namespace {

using ygn::spice::core::Diagnostic;
using ygn::spice::core::DiagnosticSeverity;
using ygn::spice::core::Uuid;

static_assert(std::is_same_v<decltype(Diagnostic::object_id), std::optional<Uuid>>);

TEST_CASE("diagnostics can describe a document-independent problem") {
  const Diagnostic diagnostic{
    .severity = DiagnosticSeverity::warning,
    .code = "library.model-not-found",
    .message = "The selected simulation model could not be resolved.",
    .object_id = std::nullopt,
  };

  CHECK(diagnostic.severity == DiagnosticSeverity::warning);
  CHECK(diagnostic.code == "library.model-not-found");
  CHECK(diagnostic.message == "The selected simulation model could not be resolved.");
  CHECK_FALSE(diagnostic.object_id.has_value());
}

TEST_CASE("diagnostics can identify the document object that caused them") {
  const auto object_id = Uuid::parse("123e4567-e89b-12d3-a456-426614174000");
  REQUIRE(object_id.has_value());

  const Diagnostic diagnostic{
    .severity = DiagnosticSeverity::error,
    .code = "netlist.unconnected-pin",
    .message = "A required pin is not connected.",
    .object_id = *object_id,
  };

  REQUIRE(diagnostic.object_id.has_value());
  CHECK(*diagnostic.object_id == *object_id);
}

TEST_CASE("diagnostic severities remain distinct") {
  CHECK(DiagnosticSeverity::info != DiagnosticSeverity::warning);
  CHECK(DiagnosticSeverity::warning != DiagnosticSeverity::error);
  CHECK(DiagnosticSeverity::info != DiagnosticSeverity::error);
}

} // namespace
