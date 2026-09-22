#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>

#include "units.hpp"

namespace {

using ygn::spice::core::Coordinate;

static_assert(std::is_same_v<Coordinate::rep, std::int64_t>);
static_assert(std::is_signed_v<Coordinate::rep>);

TEST_CASE("coordinates preserve signed nanometre values") {
  CHECK(Coordinate::from_nanometres(42).nanometres() == 42);
  CHECK(Coordinate::from_nanometres(-42).nanometres() == -42);
}

TEST_CASE("metric coordinate factories convert exactly to nanometres") {
  CHECK(Coordinate::from_micrometres(1).nanometres() == 1'000);
  CHECK(Coordinate::from_millimetres(1).nanometres() == 1'000'000);
}

TEST_CASE("mil coordinate factories convert exactly to nanometres") {
  CHECK(Coordinate::from_mils(1).nanometres() == 25'400);
  CHECK(Coordinate::from_mils(1'000).nanometres() == 25'400'000);
}

TEST_CASE("coordinates compare by their exact internal value") {
  CHECK(Coordinate::from_mils(1) == Coordinate::from_nanometres(25'400));
  CHECK(Coordinate::from_millimetres(1) != Coordinate::from_nanometres(999'999));
}

TEST_CASE("coordinate ordering handles equality signs and extremes", "[regression]") {
  const std::array<Coordinate::rep, 5> ordered{
    std::numeric_limits<Coordinate::rep>::min(),
    -1,
    0,
    1,
    std::numeric_limits<Coordinate::rep>::max(),
  };
  for (std::size_t i = 0; i < ordered.size(); ++i) {
    for (std::size_t j = 0; j < ordered.size(); ++j) {
      CAPTURE(ordered[i], ordered[j]);
      const auto a = Coordinate::from_nanometres(ordered[i]);
      const auto b = Coordinate::from_nanometres(ordered[j]);
      CHECK((a == b) == (i == j));
      CHECK((a != b) == (i != j));
      CHECK((a < b) == (i < j));
      CHECK((a > b) == (i > j));
      CHECK((a <= b) == (i <= j));
      CHECK((a >= b) == (i >= j));
    }
  }
}

TEST_CASE("scaled coordinate factories reject values that overflow nanometres") {
  using rep = Coordinate::rep;

  const auto check_scaled_factory = [](auto factory, const rep scale) {
    constexpr auto maximum = std::numeric_limits<rep>::max();
    constexpr auto minimum = std::numeric_limits<rep>::min();
    const auto largest_valid = maximum / scale;
    const auto smallest_valid = minimum / scale;

    CHECK(factory(largest_valid).nanometres() == largest_valid * scale);
    CHECK(factory(smallest_valid).nanometres() == smallest_valid * scale);
    CHECK_THROWS_AS(factory(largest_valid + 1), std::overflow_error);
    CHECK_THROWS_AS(factory(smallest_valid - 1), std::overflow_error);
  };

  check_scaled_factory(Coordinate::from_micrometres, 1'000);
  check_scaled_factory(Coordinate::from_millimetres, 1'000'000);
  check_scaled_factory(Coordinate::from_mils, 25'400);
}

TEST_CASE("nanometre coordinates preserve the full representation range") {
  constexpr auto maximum = std::numeric_limits<Coordinate::rep>::max();
  constexpr auto minimum = std::numeric_limits<Coordinate::rep>::min();

  CHECK(Coordinate::from_nanometres(maximum).nanometres() == maximum);
  CHECK(Coordinate::from_nanometres(minimum).nanometres() == minimum);
}

} // namespace
