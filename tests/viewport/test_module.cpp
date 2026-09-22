#include <ygn_spice/viewport/module.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("viewport module target links") {
  CHECK(ygn::spice::viewport::module_name() == "viewport");
}
