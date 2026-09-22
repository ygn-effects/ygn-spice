// Keep the public header first so this test also verifies that it is self-contained.
#include "history.hpp"

#include "commands.hpp"
#include "document.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <string_view>

namespace {

using namespace ygn::spice::core;

struct RenameControl {
  bool allow_execute = true;
  bool allow_undo = true;
  int execute_calls = 0;
  int undo_calls = 0;
};

// Inject a recoverable failure without changing the document outside history.
// Successful calls still perform a real rename and its inverse.
class ControlledRename final : public Command {
public:
  ControlledRename(const Uuid &id, const std::string &name, std::shared_ptr<RenameControl> control)
      : rename_(id, name), control_(control) {
  }

  bool execute(Document &document) override {
    ++control_->execute_calls;
    return control_->allow_execute && rename_.execute(document);
  }

  bool undo(Document &document) override {
    ++control_->undo_calls;
    return control_->allow_undo && rename_.undo(document);
  }

private:
  RenameComponentCommand rename_;
  std::shared_ptr<RenameControl> control_;
};

// History is bound to one document, which outlives it. It owns submitted commands.
// execute/undo/redo return true only when an edit succeeds. These tests exercise
// observable behavior without prescribing stacks or a vector/cursor internally.
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

  bool rename(const Uuid &id, std::string name) {
    return history.execute(std::make_unique<RenameComponentCommand>(id, name));
  }

  void check_state(
    std::string_view target_name, std::string_view other_name, bool can_undo, bool can_redo) const {
    CHECK(history.can_undo() == can_undo);
    CHECK(history.can_redo() == can_redo);
    CHECK(document.id() == document_id);
    REQUIRE(document.sheets().size() == 1);
    CHECK(document.sheets().front().id() == sheet_id);
    REQUIRE(document.sheets().front().components().size() == 2);

    const auto *actual_target = document.lookup(target.id());
    REQUIRE(actual_target != nullptr);
    CHECK(actual_target->id() == target.id());
    CHECK(actual_target->position() == target.position());
    CHECK(actual_target->designator() == target_name);

    const auto *actual_other = document.lookup(other.id());
    REQUIRE(actual_other != nullptr);
    CHECK(actual_other->id() == other.id());
    CHECK(actual_other->position() == other.position());
    CHECK(actual_other->designator() == other_name);
  }
};

TEST_CASE("empty history cannot undo or redo", "[history]") {
  HistoryFixture fixture;
  fixture.check_state("R1", "R9", false, false);
  CHECK_FALSE(fixture.history.undo());
  CHECK_FALSE(fixture.history.redo());
  fixture.check_state("R1", "R9", false, false);

  // Boundary attempts must not prevent the first real edit.
  REQUIRE(fixture.rename(fixture.target.id(), "R2"));
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

TEST_CASE("a successful new edit replaces the entire redo branch", "[history]") {
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

TEST_CASE("unsuccessful edits do not enter an empty history", "[history]") {
  HistoryFixture fixture;

  SECTION("missing component") {
    const auto absent = ComponentInstance::create("R0", fixture.target.position());
    CHECK_FALSE(fixture.rename(absent.id(), "R7"));
  }
  SECTION("unchanged name") {
    CHECK_FALSE(fixture.rename(fixture.target.id(), "R1"));
  }

  fixture.check_state("R1", "R9", false, false);
  CHECK_FALSE(fixture.history.undo());
  CHECK_FALSE(fixture.history.redo());
  REQUIRE(fixture.rename(fixture.target.id(), "R2"));
  REQUIRE(fixture.history.undo());
  fixture.check_state("R1", "R9", false, true);
}

TEST_CASE("unsuccessful new edits preserve both undo and redo history", "[history]") {
  HistoryFixture fixture;
  REQUIRE(fixture.rename(fixture.target.id(), "R2"));
  REQUIRE(fixture.rename(fixture.target.id(), "R3"));
  REQUIRE(fixture.history.undo());
  fixture.check_state("R2", "R9", true, true);

  SECTION("missing component") {
    const auto absent = ComponentInstance::create("R0", fixture.target.position());
    CHECK_FALSE(fixture.rename(absent.id(), "R7"));
  }
  SECTION("unchanged name") {
    CHECK_FALSE(fixture.rename(fixture.target.id(), "R2"));
  }

  fixture.check_state("R2", "R9", true, true);
  REQUIRE(fixture.history.redo());
  fixture.check_state("R3", "R9", true, false);
  REQUIRE(fixture.history.undo());
  fixture.check_state("R2", "R9", true, true);
  REQUIRE(fixture.history.undo());
  fixture.check_state("R1", "R9", false, true);
}

TEST_CASE("failed undo preserves history and retries the same command", "[history][failure]") {
  HistoryFixture fixture;
  const auto control = std::make_shared<RenameControl>();
  REQUIRE(fixture.rename(fixture.target.id(), "R2")); // A
  REQUIRE(fixture.history.execute(
    std::make_unique<ControlledRename>(fixture.other.id(), "R8", control))); // B
  REQUIRE(fixture.rename(fixture.target.id(), "R3"));                        // C
  REQUIRE(fixture.history.undo()); // Leave C available to redo.
  fixture.check_state("R2", "R8", true, true);

  control->allow_undo = false;
  CHECK_FALSE(fixture.history.undo());
  CHECK(control->undo_calls == 1);
  fixture.check_state("R2", "R8", true, true);

  control->allow_undo = true;
  REQUIRE(fixture.history.undo());
  CHECK(control->undo_calls == 2);
  fixture.check_state("R2", "R9", true, true);
  REQUIRE(fixture.history.undo());
  fixture.check_state("R1", "R9", false, true);

  // A, B and the pre-existing redo entry C must all remain usable.
  REQUIRE(fixture.history.redo());
  fixture.check_state("R2", "R9", true, true);
  REQUIRE(fixture.history.redo());
  CHECK(control->execute_calls == 2);
  fixture.check_state("R2", "R8", true, true);
  REQUIRE(fixture.history.redo());
  fixture.check_state("R3", "R8", true, false);
}

TEST_CASE("failed redo preserves history and retries the same command", "[history][failure]") {
  HistoryFixture fixture;
  const auto control = std::make_shared<RenameControl>();
  REQUIRE(fixture.rename(fixture.target.id(), "R2")); // A
  REQUIRE(fixture.history.execute(
    std::make_unique<ControlledRename>(fixture.other.id(), "R8", control))); // B
  REQUIRE(fixture.rename(fixture.target.id(), "R3"));                        // C
  REQUIRE(fixture.history.undo());
  REQUIRE(fixture.history.undo());
  fixture.check_state("R2", "R9", true, true);

  control->allow_execute = false;
  CHECK_FALSE(fixture.history.redo());
  CHECK(control->execute_calls == 2); // Initial execution plus failed redo.
  fixture.check_state("R2", "R9", true, true);

  control->allow_execute = true;
  REQUIRE(fixture.history.redo());
  CHECK(control->execute_calls == 3);
  fixture.check_state("R2", "R8", true, true);
  REQUIRE(fixture.history.redo());
  fixture.check_state("R3", "R8", true, false);

  // The applied prefix and both redo entries must survive the failure.
  REQUIRE(fixture.history.undo());
  fixture.check_state("R2", "R8", true, true);
  REQUIRE(fixture.history.undo());
  CHECK(control->undo_calls == 2);
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
    CHECK_FALSE(fixture.rename(absent.id(), "R7"));
    CHECK(fixture.history.is_dirty() == start_dirty);
    CHECK_FALSE(fixture.rename(fixture.target.id(), current_name));
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

TEST_CASE(
  "failed undo preserves clean and dirty states until a successful retry",
  "[history][dirty][failure]") {
  for (const bool save_after_edit : {false, true}) {
    CAPTURE(save_after_edit);
    HistoryFixture fixture;
    fixture.history.mark_saved();
    const auto control = std::make_shared<RenameControl>();
    REQUIRE(fixture.history.execute(
      std::make_unique<ControlledRename>(fixture.target.id(), "R2", control)));
    if (save_after_edit) {
      fixture.history.mark_saved();
    }
    control->allow_undo = false;
    CHECK_FALSE(fixture.history.undo());
    CHECK(fixture.history.is_dirty() == !save_after_edit);
    fixture.check_state("R2", "R9", true, false);
    control->allow_undo = true;
    REQUIRE(fixture.history.undo());
    CHECK(fixture.history.is_dirty() == save_after_edit);
  }
}

TEST_CASE(
  "failed redo preserves clean and dirty states until a successful retry",
  "[history][dirty][failure]") {
  for (const bool save_after_edit : {false, true}) {
    CAPTURE(save_after_edit);
    HistoryFixture fixture;
    fixture.history.mark_saved();
    const auto control = std::make_shared<RenameControl>();
    REQUIRE(fixture.history.execute(
      std::make_unique<ControlledRename>(fixture.target.id(), "R2", control)));
    if (save_after_edit) {
      fixture.history.mark_saved();
    }
    REQUIRE(fixture.history.undo());
    control->allow_execute = false;
    CHECK_FALSE(fixture.history.redo());
    CHECK(fixture.history.is_dirty() == save_after_edit);
    fixture.check_state("R1", "R9", false, true);
    control->allow_execute = true;
    REQUIRE(fixture.history.redo());
    CHECK(fixture.history.is_dirty() == !save_after_edit);
  }
}

} // namespace
