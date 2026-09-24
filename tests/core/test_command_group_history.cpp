#include "command_group.hpp"
#include "command_test_support.hpp"
#include "commands.hpp"
#include "document.hpp"
#include "history.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <stdexcept>
#include <utility>

namespace {

using namespace ygn::spice::core;
using namespace ygn::spice::core::test;

// A group enters the history as one entry, or not at all when it is rejected.
struct HistoryGroupFixture : GroupFixture {
  CommandHistory history{document};

  HistoryGroupFixture() {
    history.mark_saved();
  }

  void check_history(bool can_undo, bool can_redo, bool dirty) const {
    CHECK(history.can_undo() == can_undo);
    CHECK(history.can_redo() == can_redo);
    CHECK(history.is_dirty() == dirty);
  }
};

std::unique_ptr<Command> group_of(Children children) {
  return std::make_unique<CommandGroup>(std::move(children));
}

TEST_CASE("a group of component edits is one history entry", "[group][history]") {
  HistoryGroupFixture fixture;
  Children children;
  children.push_back(rename_command(fixture.resistor.id(), "R2"));
  children.push_back(rename_command(fixture.capacitor.id(), "C2"));

  check_result(fixture.history.execute(group_of(std::move(children))), CommandOutcome::Completed);
  fixture.check_names("R2", "C2");
  fixture.check_history(true, false, true);

  REQUIRE(fixture.history.undo());
  fixture.check_names("R1", "C1");
  fixture.check_history(false, true, false);

  REQUIRE(fixture.history.redo());
  fixture.check_names("R2", "C2");
  fixture.check_history(true, false, true);
}

TEST_CASE("a rejected group creates no history entry and reports its reason", "[group][history]") {
  HistoryGroupFixture fixture;

  SECTION("a no-op child") {
    Children children;
    children.push_back(rename_command(fixture.resistor.id(), "R2"));
    children.push_back(rename_command(fixture.capacitor.id(), "C1")); // Already named C1.
    children.push_back(rename_command(fixture.capacitor.id(), "C2"));
    check_result(
      fixture.history.execute(group_of(std::move(children))),
      CommandOutcome::Unchanged,
      CommandReason::NoChangeNeeded);
  }
  SECTION("an empty group") {
    check_result(
      fixture.history.execute(group_of(Children{})),
      CommandOutcome::Unchanged,
      CommandReason::NoChangeNeeded);
  }

  fixture.check_names("R1", "C1");
  fixture.check_history(false, false, false);
}

TEST_CASE("a rejected group preserves the saved redo branch", "[group][history]") {
  HistoryGroupFixture fixture;
  REQUIRE(fixture.history.execute(rename_command(fixture.resistor.id(), "R2")));
  fixture.history.mark_saved();
  REQUIRE(fixture.history.undo());

  Children children;
  children.push_back(rename_command(fixture.resistor.id(), "R3"));
  children.push_back(rename_command(fixture.sheet_id, "invalid target"));
  check_result(
    fixture.history.execute(group_of(std::move(children))),
    CommandOutcome::Unchanged,
    CommandReason::NotFound);
  fixture.check_names("R1", "C1");
  fixture.check_history(false, true, true);

  REQUIRE(fixture.history.redo());
  fixture.check_names("R2", "C1");
  fixture.check_history(true, false, false);
}

// Only an edit made outside the history could make a redo invalid.
TEST_CASE(
  "a group rejected on redo rolls back and then fails loudly", "[group][history][contract]") {
  HistoryGroupFixture fixture;
  const auto trace = std::make_shared<Trace>();
  Children children;
  children.push_back(traced(fixture.resistor.id(), "R2", "A", trace));
  children.push_back(traced(fixture.capacitor.id(), "C2", "B", trace));
  REQUIRE(fixture.history.execute(group_of(std::move(children))));
  REQUIRE(fixture.history.undo());

  trace->calls.clear();
  trace->fail_execute = "B";
  REQUIRE_THROWS_AS(fixture.history.redo(), std::logic_error);
  CHECK(trace->calls == Calls{"execute A", "execute B", "undo A"});
  fixture.check_names("R1", "C1");
}

} // namespace
