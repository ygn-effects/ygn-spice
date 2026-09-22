#include <catch2/catch_test_macros.hpp>

#include "module.hpp"

TEST_CASE("core module target links") {
  CHECK(ygn::spice::core::module_name() == "core");
}
