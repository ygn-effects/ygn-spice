#pragma once

#include <optional>
#include <string>

#include "uuid.hpp"

namespace ygn::spice::core {
enum class DiagnosticSeverity { info, warning, error };

struct Diagnostic {
  DiagnosticSeverity severity;
  std::string code;
  std::string message;
  std::optional<Uuid> object_id;
};
} // namespace ygn::spice::core
