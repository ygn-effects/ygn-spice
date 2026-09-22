#include <catch2/catch_test_macros.hpp>

#include <array>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include "uuid.hpp"

namespace {

using ygn::spice::core::Uuid;

static_assert(std::is_same_v<decltype(Uuid::parse(std::string{})), std::optional<Uuid>>);
static_assert(!std::is_constructible_v<Uuid, std::string>);
static_assert(!std::is_constructible_v<Uuid, unsigned char *>);

TEST_CASE("UUID strings round-trip in canonical form") {
  const auto uuid = Uuid::parse("123e4567-e89b-12d3-a456-426614174000");

  REQUIRE(uuid.has_value());
  CHECK(uuid->to_string() == "123e4567-e89b-12d3-a456-426614174000");
}

TEST_CASE("UUID parsing accepts uppercase hexadecimal digits") {
  const auto uuid = Uuid::parse("123E4567-E89B-12D3-A456-426614174ABC");

  REQUIRE(uuid.has_value());
  CHECK(uuid->to_string() == "123e4567-e89b-12d3-a456-426614174abc");
}

TEST_CASE("UUID parsing rejects malformed strings") {
  CHECK_FALSE(Uuid::parse("").has_value());
  CHECK_FALSE(Uuid::parse("123e4567-e89b-12d3-a456-42661417400").has_value());
  CHECK_FALSE(Uuid::parse("123e4567e89b-12d3-a456-426614174000").has_value());
  CHECK_FALSE(Uuid::parse("123e4567-e89b-12d3-a456-42661417400z").has_value());
}

TEST_CASE("the nil UUID string is valid input") {
  const auto uuid = Uuid::parse("00000000-0000-0000-0000-000000000000");

  REQUIRE(uuid.has_value());
  CHECK(uuid->is_nil());
}

TEST_CASE("the nil UUID has the all-zero representation") {
  const auto uuid = Uuid::nil();

  CHECK(uuid.is_nil());
  CHECK(uuid.to_string() == "00000000-0000-0000-0000-000000000000");
}

TEST_CASE("generated UUIDs are non-nil and parseable") {
  const auto uuid = Uuid::random();
  const auto parsed = Uuid::parse(uuid.to_string());

  CHECK_FALSE(uuid.is_nil());
  REQUIRE(parsed.has_value());
  CHECK(*parsed == uuid);
}

TEST_CASE("equal UUIDs behave as one key in a hash container") {
  const auto first = Uuid::parse("123e4567-e89b-12d3-a456-426614174000");
  const auto equal = Uuid::parse("123E4567-E89B-12D3-A456-426614174000");
  REQUIRE(first.has_value());
  REQUIRE(equal.has_value());

  std::unordered_set<Uuid, Uuid::UuidHash> uuids;

  CHECK(uuids.insert(*first).second);
  CHECK_FALSE(uuids.insert(*equal).second);
  CHECK(uuids.contains(*first));
}

TEST_CASE("UUID ordering compares the entire value in byte order", "[ordering]") {
  const std::array texts{
    "00000000-0000-0000-0000-000000000000",
    "00000000-0000-0000-0000-000000000001",
    "00000000-0000-0000-0000-0000000000ff",
    "00000000-0000-0000-0000-000000000100",
    "00000000-0000-0000-0001-000000000000",
    "7fffffff-ffff-ffff-ffff-ffffffffffff",
    "80000000-0000-0000-0000-000000000000",
    "ffffffff-ffff-ffff-ffff-ffffffffffff",
  };
  std::vector<Uuid> ordered;
  for (const auto *text : texts) {
    const auto parsed = Uuid::parse(text);
    REQUIRE(parsed.has_value());
    ordered.push_back(*parsed);
  }

  for (std::size_t i = 0; i < ordered.size(); ++i) {
    for (std::size_t j = 0; j < ordered.size(); ++j) {
      CAPTURE(texts[i], texts[j]);
      CHECK((ordered[i] < ordered[j]) == (i < j));
      CHECK((ordered[i] == ordered[j]) == (i == j));
    }
  }
}

} // namespace
