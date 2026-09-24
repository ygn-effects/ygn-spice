// Keep the public header first so this test also verifies that it is self-contained.
#include "history.hpp"

#include "command_test_support.hpp"
#include "commands.hpp"
#include "document.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace {

using namespace ygn::spice::core;
using namespace ygn::spice::core::test;

// History is bound to one document, which outlives it, and owns the commands it
// accepts. execute returns the command's own result, and a rejected command
// never enters the history. undo and redo return false only when there is
// nothing to undo or redo; anything else going wrong is a broken invariant that
// throws std::logic_error. These tests exercise observable behavior without
// prescribing stacks or a vector/cursor internally.
static_assert(
  std::is_same_v<
    decltype(std::declval<CommandHistory &>().execute(std::declval<std::unique_ptr<Command>>())),
    CommandResult>);
static_assert(std::is_same_v<decltype(std::declval<CommandHistory &>().undo()), bool>);
static_assert(std::is_same_v<decltype(std::declval<CommandHistory &>().redo()), bool>);

struct HistoryFixture {
  Document document;
  Uuid document_id = document.id();
  Uuid sheet_id = document.sheets().front().id();
  ComponentInstance target = ComponentInstance::create(
    "R1", {Coordinate::from_millimetres(1), Coordinate::from_millimetres(2)});
  ComponentInstance other = ComponentInstance::create(
    "R9", {Coordinate::from_millimetres(3), Coordinate::from_millimetres(4)});
  CommandHistory history{document};

  HistoryFixture() {
    REQUIRE(document.add_component(sheet_id, target));
    REQUIRE(document.add_component(sheet_id, other));
  }

  CommandResult rename(const Uuid &id, std::string name) {
    return history.execute(rename_command(id, std::move(name)));
  }

  void check_names(std::string_view target_name, std::string_view other_name) const {
    CHECK(document.id() == document_id);
    REQUIRE(document.sheets().size() == 1);
    CHECK(document.sheets().front().id() == sheet_id);
    REQUIRE(document.sheets().front().components().size() == 2);

    const auto *actual_target = document.lookup(target.id());
    REQUIRE(actual_target != nullptr);
    CHECK(actual_target->position() == target.position());
    CHECK(actual_target->designator() == target_name);

    const auto *actual_other = document.lookup(other.id());
    REQUIRE(actual_other != nullptr);
    CHECK(actual_other->position() == other.position());
    CHECK(actual_other->designator() == other_name);
  }

  void check_state(
    std::string_view target_name, std::string_view other_name, bool can_undo, bool can_redo) const {
    CHECK(history.can_undo() == can_undo);
    CHECK(history.can_redo() == can_redo);
    check_names(target_name, other_name);
  }
};

TEST_CASE("empty history cannot undo or redo", "[history]") {
  HistoryFixture fixture;
  fixture.check_state("R1", "R9", false, false);
  CHECK_FALSE(fixture.history.undo());
  CHECK_FALSE(fixture.history.redo());
  fixture.check_state("R1", "R9", false, false);

  // Boundary attempts must not prevent the first real edit.
  check_result(fixture.rename(fixture.target.id(), "R2"), CommandOutcome::Completed);
  fixture.check_state("R2", "R9", true, false);
}

TEST_CASE("history retains a rename for repeated undo and redo", "[history]") {
  HistoryFixture fixture;
  REQUIRE(fixture.rename(fixture.target.id(), "R2"));
  fixture.check_state("R2", "R9", true, false);

  for (int cycle = 0; cycle < 3; ++cycle) {
    REQUIRE(fixture.history.undo());
    fixture.check_state("R1", "R9", false, true);
    CHECK_FALSE(fixture.history.undo());
    fixture.check_state("R1", "R9", false, true);

    REQUIRE(fixture.history.redo());
    fixture.check_state("R2", "R9", true, false);
    CHECK_FALSE(fixture.history.redo());
    fixture.check_state("R2", "R9", true, false);
  }
}

TEST_CASE("history undoes newest edits first and redoes them in execution order", "[history]") {
  HistoryFixture fixture;
  REQUIRE(fixture.rename(fixture.target.id(), "R2"));
  REQUIRE(fixture.rename(fixture.other.id(), "R8"));
  REQUIRE(fixture.rename(fixture.target.id(), "R3"));
  fixture.check_state("R3", "R8", true, false);

  REQUIRE(fixture.history.undo());
  fixture.check_state("R2", "R8", true, true);
  REQUIRE(fixture.history.undo());
  fixture.check_state("R2", "R9", true, true);
  REQUIRE(fixture.history.undo());
  fixture.check_state("R1", "R9", false, true);

  REQUIRE(fixture.history.redo());
  fixture.check_state("R2", "R9", true, true);
  REQUIRE(fixture.history.redo());
  fixture.check_state("R2", "R8", true, true);
  REQUIRE(fixture.history.redo());
  fixture.check_state("R3", "R8", true, false);
}

TEST_CASE("a new edit replaces the entire redo branch", "[history]") {
  HistoryFixture fixture;
  REQUIRE(fixture.rename(fixture.target.id(), "R2")); // A
  REQUIRE(fixture.rename(fixture.other.id(), "R8"));  // B
  REQUIRE(fixture.rename(fixture.target.id(), "R3")); // C
  REQUIRE(fixture.history.undo());
  REQUIRE(fixture.history.undo());
  fixture.check_state("R2", "R9", true, true);

  REQUIRE(fixture.rename(fixture.target.id(), "R4")); // D replaces B and C.
  fixture.check_state("R4", "R9", true, false);
  CHECK_FALSE(fixture.history.redo());
  fixture.check_state("R4", "R9", true, false);

  REQUIRE(fixture.history.undo());
  fixture.check_state("R2", "R9", true, true);
  REQUIRE(fixture.history.undo());
  fixture.check_state("R1", "R9", false, true);
  REQUIRE(fixture.history.redo());
  fixture.check_state("R2", "R9", true, true);
  REQUIRE(fixture.history.redo());
  fixture.check_state("R4", "R9", true, false);
  CHECK_FALSE(fixture.history.redo());
  fixture.check_state("R4", "R9", true, false);
}

TEST_CASE("rejected edits report their reason and stay out of an empty history", "[history]") {
  HistoryFixture fixture;

  SECTION("missing component") {
    const auto absent = ComponentInstance::create("R0", fixture.target.position());
    check_result(
      fixture.rename(absent.id(), "R7"),
      CommandOutcome::Unchanged,
      CommandReason::NotFound);
  }
  SECTION("unchanged name") {
    check_result(
      fixture.rename(fixture.target.id(), "R1"),
      CommandOutcome::Unchanged,
      CommandReason::NoChangeNeeded);
  }

  fixture.check_state("R1", "R9", false, false);
  CHECK_FALSE(fixture.history.undo());
  CHECK_FALSE(fixture.history.redo());
  REQUIRE(fixture.rename(fixture.target.id(), "R2"));
  REQUIRE(fixture.history.undo());
  fixture.check_state("R1", "R9", false, true);
}

TEST_CASE("rejected new edits preserve both undo and redo history", "[history]") {
  HistoryFixture fixture;
  REQUIRE(fixture.rename(fixture.target.id(), "R2"));
  REQUIRE(fixture.rename(fixture.target.id(), "R3"));
  REQUIRE(fixture.history.undo());
  fixture.check_state("R2", "R9", true, true);

  SECTION("missing component") {
    const auto absent = ComponentInstance::create("R0", fixture.target.position());
    check_result(
      fixture.rename(absent.id(), "R7"),
      CommandOutcome::Unchanged,
      CommandReason::NotFound);
  }
  SECTION("unchanged name") {
    check_result(
      fixture.rename(fixture.target.id(), "R2"),
      CommandOutcome::Unchanged,
      CommandReason::NoChangeNeeded);
  }

  fixture.check_state("R2", "R9", true, true);
  REQUIRE(fixture.history.redo());
  fixture.check_state("R3", "R9", true, false);
  REQUIRE(fixture.history.undo());
  fixture.check_state("R2", "R9", true, true);
  REQUIRE(fixture.history.undo());
  fixture.check_state("R1", "R9", false, true);
}

// Explicitly establish a saved baseline: these tests do not decide whether a
// newly created, never-saved project should initially be considered dirty.
TEST_CASE("saved baseline becomes dirty on edit and clean on undo", "[history][dirty]") {
  HistoryFixture fixture;
  const CommandHistory &view = fixture.history;
  fixture.history.mark_saved();
  CHECK_FALSE(view.is_dirty());
  CHECK_FALSE(fixture.history.undo());
  CHECK_FALSE(fixture.history.redo());
  CHECK_FALSE(view.is_dirty());

  REQUIRE(fixture.rename(fixture.target.id(), "R2"));
  CHECK(view.is_dirty());
  REQUIRE(fixture.history.undo());
  CHECK_FALSE(view.is_dirty());
  REQUIRE(fixture.history.redo());
  CHECK(view.is_dirty());
}

TEST_CASE(
  "saving in the middle preserves undo and redo and replaces the saved marker",
  "[history][dirty]") {
  HistoryFixture fixture;
  fixture.history.mark_saved();
  REQUIRE(fixture.rename(fixture.target.id(), "R2"));
  REQUIRE(fixture.rename(fixture.target.id(), "R3"));
  REQUIRE(fixture.history.undo());

  fixture.history.mark_saved();
  fixture.history.mark_saved(); // Repeating a save at the same state is harmless.
  CHECK_FALSE(fixture.history.is_dirty());
  fixture.check_state("R2", "R9", true, true);

  REQUIRE(fixture.history.undo());
  CHECK(fixture.history.is_dirty()); // The old baseline is no longer the saved state.
  REQUIRE(fixture.history.redo());
  CHECK_FALSE(fixture.history.is_dirty());
  REQUIRE(fixture.history.redo());
  CHECK(fixture.history.is_dirty());
  REQUIRE(fixture.history.undo());
  CHECK_FALSE(fixture.history.is_dirty());
}

TEST_CASE(
  "discarded saved state stays dirty even at the same numeric history position",
  "[history][dirty]") {
  HistoryFixture fixture;
  REQUIRE(fixture.rename(fixture.target.id(), "R2")); // A
  REQUIRE(fixture.rename(fixture.target.id(), "R3")); // B
  fixture.history.mark_saved();
  REQUIRE(fixture.history.undo());
  CHECK(fixture.history.is_dirty());
  REQUIRE(fixture.rename(fixture.target.id(), "R4")); // D replaces saved B.
  fixture.check_state("R4", "R9", true, false);
  CHECK(fixture.history.is_dirty());

  REQUIRE(fixture.history.undo());
  CHECK(fixture.history.is_dirty());
  REQUIRE(fixture.history.undo());
  CHECK(fixture.history.is_dirty());
  REQUIRE(fixture.history.redo());
  CHECK(fixture.history.is_dirty());
  REQUIRE(fixture.history.redo());
  CHECK(fixture.history.is_dirty());

  fixture.history.mark_saved(); // A new save makes the current branch clean.
  CHECK_FALSE(fixture.history.is_dirty());
  REQUIRE(fixture.history.undo());
  CHECK(fixture.history.is_dirty());
  REQUIRE(fixture.history.redo());
  CHECK_FALSE(fixture.history.is_dirty());
}

TEST_CASE("branching preserves a saved state at or before the branch point", "[history][dirty]") {
  for (const bool save_at_branch_point : {false, true}) {
    CAPTURE(save_at_branch_point);
    HistoryFixture fixture;
    fixture.history.mark_saved();
    REQUIRE(fixture.rename(fixture.target.id(), "R2"));
    if (save_at_branch_point) {
      fixture.history.mark_saved();
    }
    REQUIRE(fixture.rename(fixture.target.id(), "R3"));
    REQUIRE(fixture.history.undo()); // Branch point at position 1.
    REQUIRE(fixture.rename(fixture.target.id(), "R4"));
    CHECK(fixture.history.is_dirty());
    REQUIRE(fixture.history.undo());
    CHECK(fixture.history.is_dirty() == !save_at_branch_point);
    REQUIRE(fixture.history.undo());
    CHECK(fixture.history.is_dirty() == save_at_branch_point);
    REQUIRE(fixture.history.redo());
    CHECK(fixture.history.is_dirty() == !save_at_branch_point);
  }
}

TEST_CASE(
  "rejected edits preserve dirty state and a saved state in the redo branch", "[history][dirty]") {
  for (const bool start_dirty : {false, true}) {
    CAPTURE(start_dirty);
    HistoryFixture fixture;
    REQUIRE(fixture.rename(fixture.target.id(), "R2"));
    fixture.history.mark_saved();
    if (start_dirty) {
      REQUIRE(fixture.history.undo());
    }
    const auto current_name = start_dirty ? "R1" : "R2";
    const auto absent = ComponentInstance::create("R0", fixture.target.position());
    check_result(
      fixture.rename(absent.id(), "R7"),
      CommandOutcome::Unchanged,
      CommandReason::NotFound);
    CHECK(fixture.history.is_dirty() == start_dirty);
    check_result(
      fixture.rename(fixture.target.id(), current_name),
      CommandOutcome::Unchanged,
      CommandReason::NoChangeNeeded);
    CHECK(fixture.history.is_dirty() == start_dirty);
    fixture.check_state(current_name, "R9", !start_dirty, start_dirty);

    if (start_dirty) {
      REQUIRE(fixture.history.redo());
      CHECK_FALSE(fixture.history.is_dirty());
    } else {
      REQUIRE(fixture.history.undo());
      CHECK(fixture.history.is_dirty());
      REQUIRE(fixture.history.redo());
      CHECK_FALSE(fixture.history.is_dirty());
    }
  }
}

// Undo and redo cannot fail while the history is the only writer of the
// document. These cases break that rule on purpose and only check that the
// history fails loudly instead of trying to recover.
TEST_CASE("broken invariants fail loudly instead of being recovered", "[history][contract]") {
  HistoryFixture fixture;

  SECTION("a redo that its command now rejects") {
    const auto trace = std::make_shared<Trace>();
    REQUIRE(fixture.history.execute(traced(fixture.target.id(), "R2", "A", trace)));
    REQUIRE(fixture.history.undo());
    trace->fail_execute = "A";

    REQUIRE_THROWS_AS(fixture.history.redo(), std::logic_error);
    fixture.check_names("R1", "R9");
  }
  SECTION("an undo whose target was removed outside the history") {
    REQUIRE(fixture.rename(fixture.target.id(), "R2"));
    REQUIRE(fixture.document.remove_component(fixture.target.id()).has_value());

    REQUIRE_THROWS_AS(fixture.history.undo(), std::logic_error);
    const auto *other = fixture.document.lookup(fixture.other.id());
    REQUIRE(other != nullptr);
    CHECK(other->designator() == "R9");
  }
}

} // namespace
