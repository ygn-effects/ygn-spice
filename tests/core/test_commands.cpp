// Keep the public header first so this test also verifies that it is self-contained.
#include "commands.hpp"

#include "command_test_support.hpp"
#include "document.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace {

using namespace ygn::spice::core;
using ygn::spice::core::test::check_result;

// A command's first execution may be rejected with a reason, and a rejected
// command leaves the document untouched. Undo is only valid after a completed
// execution, so anything else is a programming error that throws
// std::logic_error. Redo is another execute after undo.
static_assert(std::is_same_v<
              decltype(std::declval<Command &>().execute(std::declval<Document &>())),
              CommandResult>);
static_assert(
  std::is_same_v<decltype(std::declval<Command &>().undo(std::declval<Document &>())), void>);

// Results only come from two factories, so a completed result never carries
// a reason and an unchanged result always does. Nothing else can build one.
static_assert(std::is_same_v<decltype(CommandResult::completed()), CommandResult>);
static_assert(
  std::is_same_v<decltype(CommandResult::unchanged(CommandReason::NotFound)), CommandResult>);
static_assert(!std::is_default_constructible_v<CommandResult>);
static_assert(!std::is_constructible_v<CommandResult, CommandOutcome>);
static_assert(!std::is_constructible_v<CommandResult, CommandOutcome, CommandReason>);

// Read-only access, returning small values by value.
static_assert(
  std::is_same_v<decltype(std::declval<const CommandResult &>().outcome()), CommandOutcome>);
static_assert(std::is_same_v<
              decltype(std::declval<const CommandResult &>().reason()),
              std::optional<CommandReason>>);

// Like std::optional, a result can be tested in a condition, but it never
// converts to bool silently. Testing must work on a const result too.
static_assert(std::is_constructible_v<bool, const CommandResult &>);
static_assert(!std::is_convertible_v<CommandResult, bool>);

struct RenameFixture {
  Document document;
  Uuid document_id = document.id();
  Uuid sheet_id = document.sheets().front().id();
  ComponentInstance target = ComponentInstance::create(
    "R1", {Coordinate::from_millimetres(1), Coordinate::from_millimetres(2)});
  // Deliberately share a designator: only the UUID selects the rename target.
  ComponentInstance other = ComponentInstance::create(
    "R1", {Coordinate::from_millimetres(3), Coordinate::from_millimetres(4)});

  RenameFixture() {
    REQUIRE(document.add_component(sheet_id, other));
    REQUIRE(document.add_component(sheet_id, target));
  }

  void check_state(std::string_view expected_name) const {
    CHECK(document.id() == document_id);
    REQUIRE(document.sheets().size() == 1);
    const auto &sheet = document.sheets().front();
    CHECK(sheet.id() == sheet_id);
    const auto &components = sheet.components();
    REQUIRE(components.size() == 2);

    for (const auto *original : {&target, &other}) {
      const auto found =
        std::find_if(components.begin(), components.end(), [&](const auto &component) {
          return component.id() == original->id();
        });
      REQUIRE(found != components.end());
      CHECK(found->position() == original->position());
      CHECK(found->designator() == (original == &target ? expected_name : other.designator()));
    }
  }
};

TEST_CASE("result factories keep the outcome and reason consistent", "[commands][result]") {
  const auto completed = CommandResult::completed();
  CHECK(completed.outcome() == CommandOutcome::Completed);
  CHECK_FALSE(completed.reason().has_value());
  CHECK(static_cast<bool>(completed));

  for (const auto reason : {CommandReason::NotFound, CommandReason::NoChangeNeeded}) {
    const auto unchanged = CommandResult::unchanged(reason);
    CHECK(unchanged.outcome() == CommandOutcome::Unchanged);
    REQUIRE(unchanged.reason().has_value());
    CHECK(*unchanged.reason() == reason);
    CHECK_FALSE(static_cast<bool>(unchanged));
  }
}

TEST_CASE(
  "component rename executes undoes and redoes without changing identity", "[commands][rename]") {
  RenameFixture fixture;
  RenameComponentCommand command(fixture.target.id(), "R7");
  fixture.check_state("R1");

  // Repeat to ensure redo does not overwrite the original name used by undo.
  for (int cycle = 0; cycle < 3; ++cycle) {
    check_result(command.execute(fixture.document), CommandOutcome::Completed);
    fixture.check_state("R7");
    command.undo(fixture.document);
    fixture.check_state("R1");
  }
}

TEST_CASE("component rename captures the old name at execution", "[commands][rename]") {
  RenameFixture fixture;
  RenameComponentCommand command(fixture.target.id(), "R7");
  REQUIRE(fixture.document.rename_component(fixture.target.id(), "R3"));

  check_result(command.execute(fixture.document), CommandOutcome::Completed);
  fixture.check_state("R7");
  command.undo(fixture.document);
  fixture.check_state("R3");
}

TEST_CASE(
  "component rename follows its UUID after collection order changes", "[commands][rename]") {
  RenameFixture fixture;
  RenameComponentCommand command(fixture.target.id(), "R7");
  const auto removed = fixture.document.remove_component(fixture.other.id());
  REQUIRE(removed.has_value());
  REQUIRE(fixture.document.add_component(fixture.sheet_id, *removed));

  check_result(command.execute(fixture.document), CommandOutcome::Completed);
  fixture.check_state("R7");
  command.undo(fixture.document);
  fixture.check_state("R1");
}

TEST_CASE("an empty previous name is valid undo state", "[commands][rename]") {
  RenameFixture fixture;
  REQUIRE(fixture.document.rename_component(fixture.target.id(), ""));
  RenameComponentCommand command(fixture.target.id(), "R7");
  check_result(command.execute(fixture.document), CommandOutcome::Completed);
  fixture.check_state("R7");
  command.undo(fixture.document);
  fixture.check_state("");
}

TEST_CASE(
  "component rename with a missing UUID is rejected without changes", "[commands][rename]") {
  RenameFixture fixture;
  const auto absent = ComponentInstance::create("R9", fixture.target.position());
  RenameComponentCommand command(absent.id(), "R7");

  check_result(
    command.execute(fixture.document),
    CommandOutcome::Unchanged,
    CommandReason::NotFound);
  fixture.check_state("R1");
}

TEST_CASE("component rename rejects a sheet or wire UUID", "[commands][rename][regression]") {
  RenameFixture fixture;
  const auto wire = Wire::create({fixture.target.position(), fixture.other.position()});
  REQUIRE(fixture.document.add_wire(fixture.sheet_id, wire));

  for (const auto &id : {fixture.sheet_id, wire.id()}) {
    RenameComponentCommand command(id, "R7");
    check_result(
      command.execute(fixture.document),
      CommandOutcome::Unchanged,
      CommandReason::NotFound);
    fixture.check_state("R1");

    const auto &wires = fixture.document.sheets().front().wires();
    REQUIRE(wires.size() == 1);
    CHECK(wires.front().id() == wire.id());
    CHECK(wires.front().points() == wire.points());
  }
}

TEST_CASE("component rename to its current name is rejected as no change", "[commands][rename]") {
  RenameFixture fixture;
  RenameComponentCommand command(fixture.target.id(), "R1");

  check_result(
    command.execute(fixture.document),
    CommandOutcome::Unchanged,
    CommandReason::NoChangeNeeded);
  fixture.check_state("R1");
}

TEST_CASE("undo without a completed execution is a programming error", "[commands][contract]") {
  RenameFixture fixture;

  SECTION("never executed") {
    RenameComponentCommand command(fixture.target.id(), "R7");
    REQUIRE_THROWS_AS(command.undo(fixture.document), std::logic_error);
  }
  SECTION("execution rejected for a missing target") {
    const auto absent = ComponentInstance::create("R9", fixture.target.position());
    RenameComponentCommand command(absent.id(), "R7");
    check_result(
      command.execute(fixture.document),
      CommandOutcome::Unchanged,
      CommandReason::NotFound);
    REQUIRE_THROWS_AS(command.undo(fixture.document), std::logic_error);
  }
  SECTION("execution rejected for an unchanged name") {
    RenameComponentCommand command(fixture.target.id(), "R1");
    check_result(
      command.execute(fixture.document),
      CommandOutcome::Unchanged,
      CommandReason::NoChangeNeeded);
    REQUIRE_THROWS_AS(command.undo(fixture.document), std::logic_error);
  }
  SECTION("already undone") {
    RenameComponentCommand command(fixture.target.id(), "R7");
    check_result(command.execute(fixture.document), CommandOutcome::Completed);
    command.undo(fixture.document);
    REQUIRE_THROWS_AS(command.undo(fixture.document), std::logic_error);
  }

  fixture.check_state("R1");
}

// Only an edit made outside the command history can remove the target.
TEST_CASE("undo after its target disappeared is a programming error", "[commands][contract]") {
  RenameFixture fixture;
  RenameComponentCommand command(fixture.target.id(), "R7");
  check_result(command.execute(fixture.document), CommandOutcome::Completed);
  REQUIRE(fixture.document.remove_component(fixture.target.id()).has_value());

  REQUIRE_THROWS_AS(command.undo(fixture.document), std::logic_error);
  REQUIRE(fixture.document.sheets().front().components().size() == 1);
  const auto *other = fixture.document.lookup(fixture.other.id());
  REQUIRE(other != nullptr);
  CHECK(other->designator() == "R1");
  CHECK(other->position() == fixture.other.position());
}

} // namespace
