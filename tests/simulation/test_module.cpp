#include <ygn_spice/simulation/module.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("simulation module target links") {
  CHECK(ygn::spice::simulation::module_name() == "simulation");
}
