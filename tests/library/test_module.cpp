#include <catch2/catch_test_macros.hpp>

#include "module.hpp"

TEST_CASE("library module target links") {
  CHECK(ygn::spice::library::module_name() == "library");
}
