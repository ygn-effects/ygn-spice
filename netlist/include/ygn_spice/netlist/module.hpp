#pragma once

#include <string_view>

namespace ygn::spice::netlist {

[[nodiscard]] std::string_view module_name() noexcept;

} // namespace ygn::spice::netlist
