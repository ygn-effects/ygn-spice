#include <ygn_spice/netlist/module.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("netlist module target links") {
  CHECK(ygn::spice::netlist::module_name() == "netlist");
}
