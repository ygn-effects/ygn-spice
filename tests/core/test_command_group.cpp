// Keep the public header first to verify that it is self-contained.
#include "command_group.hpp"

#include "commands.hpp"
#include "document.hpp"
#include "history.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using namespace ygn::spice::core;
using Children = std::vector<std::unique_ptr<Command>>;

struct GroupFixture {
  Document document;
  Uuid sheet_id = document.sheets().front().id();
  ComponentInstance resistor = ComponentInstance::create(
    "R1", {Coordinate::from_millimetres(1), Coordinate::from_millimetres(2)});
  ComponentInstance capacitor = ComponentInstance::create(
    "C1", {Coordinate::from_millimetres(3), Coordinate::from_millimetres(4)});
  CommandHistory history{document};

  GroupFixture() {
    REQUIRE(document.add_component(sheet_id, resistor));
    REQUIRE(document.add_component(sheet_id, capacitor));
    history.mark_saved();
  }

  void check_names(std::string_view r, std::string_view c) const {
    REQUIRE(document.sheets().size() == 1);
    CHECK(document.sheets().front().id() == sheet_id);
    REQUIRE(document.sheets().front().components().size() == 2);
    const auto *actual_r = document.lookup(resistor.id());
    const auto *actual_c = document.lookup(capacitor.id());
    REQUIRE(actual_r != nullptr);
    REQUIRE(actual_c != nullptr);
    CHECK(actual_r->id() == resistor.id());
    CHECK(actual_c->id() == capacitor.id());
    CHECK(actual_r->position() == resistor.position());
    CHECK(actual_c->position() == capacitor.position());
    CHECK(actual_r->designator() == r);
    CHECK(actual_c->designator() == c);
  }
};

void add_rename(Children &children, const Uuid &id, const std::string &name) {
  children.push_back(std::make_unique<RenameComponentCommand>(id, name));
}

struct Trace {
  std::vector<std::string> calls;
  std::string fail_execute;
  std::string fail_undo;
};

// A child returning false never mutates the document.
// Tracing catches accidental execution of later children or undo of a failed child.
class TracedRename final : public Command {
public:
  TracedRename(
    const Uuid &id, const std::string &name, std::string label, std::shared_ptr<Trace> trace)
      : rename_(id, name), label_(std::move(label)), trace_(std::move(trace)) {
  }

  bool execute(Document &document) override {
    trace_->calls.push_back("execute " + label_);
    return trace_->fail_execute != label_ && rename_.execute(document);
  }

  bool undo(Document &document) override {
    trace_->calls.push_back("undo " + label_);
    return trace_->fail_undo != label_ && rename_.undo(document);
  }

private:
  RenameComponentCommand rename_;
  std::string label_;
  std::shared_ptr<Trace> trace_;
};

TEST_CASE("a command group executes forwards and undoes backwards repeatedly", "[group]") {
  GroupFixture fixture;
  const auto trace = std::make_shared<Trace>();
  Children children;
  children.push_back(std::make_unique<TracedRename>(fixture.resistor.id(), "R2", "A", trace));
  children.push_back(std::make_unique<TracedRename>(fixture.resistor.id(), "R3", "B", trace));
  CommandGroup group(std::move(children));

  for (int cycle = 0; cycle < 3; ++cycle) {
    trace->calls.clear();
    REQUIRE(group.execute(fixture.document));
    fixture.check_names("R3", "C1");
    REQUIRE(group.undo(fixture.document));
    fixture.check_names("R1", "C1");
    CHECK(trace->calls == std::vector<std::string>{"execute A", "execute B", "undo B", "undo A"});
  }
}

TEST_CASE("a group rolls back only successful children and can retry", "[group][failure]") {
  for (const auto &failure : {"A", "B", "C"}) {
    CAPTURE(failure);
    GroupFixture fixture;
    const auto trace = std::make_shared<Trace>();
    trace->fail_execute = failure;
    Children children;
    children.push_back(std::make_unique<TracedRename>(fixture.resistor.id(), "R2", "A", trace));
    children.push_back(std::make_unique<TracedRename>(fixture.resistor.id(), "R3", "B", trace));
    children.push_back(std::make_unique<TracedRename>(fixture.resistor.id(), "R4", "C", trace));
    CommandGroup group(std::move(children));

    CHECK_FALSE(group.execute(fixture.document));
    fixture.check_names("R1", "C1");
    if (trace->fail_execute == "A") {
      CHECK(trace->calls == std::vector<std::string>{"execute A"});
    } else if (trace->fail_execute == "B") {
      CHECK(trace->calls == std::vector<std::string>{"execute A", "execute B", "undo A"});
    } else {
      CHECK(
        trace->calls ==
        std::vector<std::string>{"execute A", "execute B", "execute C", "undo B", "undo A"});
    }

    trace->fail_execute.clear();
    trace->calls.clear();
    REQUIRE(group.execute(fixture.document));
    CHECK(trace->calls == std::vector<std::string>{"execute A", "execute B", "execute C"});
    fixture.check_names("R4", "C1");
    REQUIRE(group.undo(fixture.document));
    fixture.check_names("R1", "C1");
  }
}

TEST_CASE("a no-op child makes the group unsuccessful and rolls back earlier changes", "[group]") {
  GroupFixture fixture;
  Children children;
  add_rename(children, fixture.resistor.id(), "R2");
  add_rename(children, fixture.capacitor.id(), "C1"); // false: no change.
  add_rename(children, fixture.capacitor.id(), "C2");
  CHECK_FALSE(fixture.history.execute(std::make_unique<CommandGroup>(std::move(children))));
  fixture.check_names("R1", "C1");
  CHECK_FALSE(fixture.history.can_undo());
  CHECK_FALSE(fixture.history.can_redo());
  CHECK_FALSE(fixture.history.is_dirty());
}

TEST_CASE("an empty group creates no edit or history entry", "[group]") {
  GroupFixture fixture;
  CommandGroup empty(Children{});
  CHECK_FALSE(empty.execute(fixture.document));
  CHECK_FALSE(empty.undo(fixture.document));
  CHECK_FALSE(fixture.history.execute(std::make_unique<CommandGroup>(Children{})));
  fixture.check_names("R1", "C1");
  CHECK_FALSE(fixture.history.can_undo());
  CHECK_FALSE(fixture.history.can_redo());
  CHECK_FALSE(fixture.history.is_dirty());
}

TEST_CASE("a group of component edits is one history entry", "[group][history]") {
  GroupFixture fixture;
  Children children;
  add_rename(children, fixture.resistor.id(), "R2");
  add_rename(children, fixture.capacitor.id(), "C2");
  REQUIRE(fixture.history.execute(std::make_unique<CommandGroup>(std::move(children))));
  fixture.check_names("R2", "C2");
  CHECK(fixture.history.is_dirty());

  REQUIRE(fixture.history.undo());
  fixture.check_names("R1", "C1");
  CHECK_FALSE(fixture.history.can_undo());
  CHECK(fixture.history.can_redo());
  CHECK_FALSE(fixture.history.is_dirty());
  REQUIRE(fixture.history.redo());
  fixture.check_names("R2", "C2");
  CHECK(fixture.history.can_undo());
  CHECK_FALSE(fixture.history.can_redo());
  CHECK(fixture.history.is_dirty());
}

TEST_CASE("an unsuccessful group preserves the saved redo branch", "[group][history][failure]") {
  GroupFixture fixture;
  REQUIRE(
    fixture.history.execute(std::make_unique<RenameComponentCommand>(fixture.resistor.id(), "R2")));
  fixture.history.mark_saved();
  REQUIRE(fixture.history.undo());

  Children children;
  add_rename(children, fixture.resistor.id(), "R3");
  add_rename(children, fixture.sheet_id, "invalid target");
  CHECK_FALSE(fixture.history.execute(std::make_unique<CommandGroup>(std::move(children))));
  fixture.check_names("R1", "C1");
  CHECK_FALSE(fixture.history.can_undo());
  CHECK(fixture.history.can_redo());
  CHECK(fixture.history.is_dirty());
  REQUIRE(fixture.history.redo());
  fixture.check_names("R2", "C1");
  CHECK_FALSE(fixture.history.is_dirty());
}

TEST_CASE(
  "failed group redo rolls back its partial edits and allows retry", "[group][history][failure]") {
  GroupFixture fixture;
  const auto trace = std::make_shared<Trace>();
  Children children;
  children.push_back(std::make_unique<TracedRename>(fixture.resistor.id(), "R2", "A", trace));
  children.push_back(std::make_unique<TracedRename>(fixture.capacitor.id(), "C2", "B", trace));
  REQUIRE(fixture.history.execute(std::make_unique<CommandGroup>(std::move(children))));
  REQUIRE(fixture.history.undo());
  trace->calls.clear();
  trace->fail_execute = "B";

  CHECK_FALSE(fixture.history.redo());
  CHECK(trace->calls == std::vector<std::string>{"execute A", "execute B", "undo A"});
  fixture.check_names("R1", "C1");
  CHECK_FALSE(fixture.history.can_undo());
  CHECK(fixture.history.can_redo());
  CHECK_FALSE(fixture.history.is_dirty());

  trace->fail_execute.clear();
  REQUIRE(fixture.history.redo());
  fixture.check_names("R2", "C2");
  CHECK(fixture.history.is_dirty());
  REQUIRE(fixture.history.undo());
  fixture.check_names("R1", "C1");
  CHECK_FALSE(fixture.history.is_dirty());
}

// These tests retain the group directly after partial failure. History recovery
// and reporting partial failure through the public API remain a separate contract.
TEST_CASE("partial group undo retains progress for either retry direction", "[group][retry]") {
  GroupFixture fixture;
  const auto trace = std::make_shared<Trace>();
  Children children;
  children.push_back(std::make_unique<TracedRename>(fixture.resistor.id(), "R2", "A", trace));
  children.push_back(std::make_unique<TracedRename>(fixture.resistor.id(), "R3", "B", trace));
  children.push_back(std::make_unique<TracedRename>(fixture.resistor.id(), "R4", "C", trace));
  CommandGroup group(std::move(children));
  REQUIRE(group.execute(fixture.document));

  trace->calls.clear();
  trace->fail_undo = "B";
  CHECK_FALSE(group.undo(fixture.document));
  CHECK(trace->calls == std::vector<std::string>{"undo C", "undo B"});
  fixture.check_names("R3", "C1"); // A and B remain applied; C is already undone.

  trace->calls.clear();
  CHECK_FALSE(group.undo(fixture.document));
  CHECK(trace->calls == std::vector<std::string>{"undo B"});
  fixture.check_names("R3", "C1");

  trace->fail_undo.clear();
  trace->calls.clear();
  SECTION("undo resumes at the failed child") {
    REQUIRE(group.undo(fixture.document));
    CHECK(trace->calls == std::vector<std::string>{"undo B", "undo A"});
    fixture.check_names("R1", "C1");

    trace->calls.clear();
    CHECK_FALSE(group.undo(fixture.document));
    CHECK(trace->calls.empty()); // No applied children remain.
    fixture.check_names("R1", "C1");

    REQUIRE(group.execute(fixture.document));
    CHECK(trace->calls == std::vector<std::string>{"execute A", "execute B", "execute C"});
    fixture.check_names("R4", "C1");
  }
  SECTION("execution resumes at the first unapplied child") {
    REQUIRE(group.execute(fixture.document));
    CHECK(trace->calls == std::vector<std::string>{"execute C"});
    fixture.check_names("R4", "C1");

    trace->calls.clear();
    REQUIRE(group.undo(fixture.document));
    CHECK(trace->calls == std::vector<std::string>{"undo C", "undo B", "undo A"});
    fixture.check_names("R1", "C1");
  }
}

TEST_CASE("failed execution and partial rollback retain the applied prefix", "[group][retry]") {
  GroupFixture fixture;
  const auto trace = std::make_shared<Trace>();
  Children children;
  children.push_back(std::make_unique<TracedRename>(fixture.resistor.id(), "R2", "A", trace));
  children.push_back(std::make_unique<TracedRename>(fixture.resistor.id(), "R3", "B", trace));
  children.push_back(std::make_unique<TracedRename>(fixture.resistor.id(), "R4", "C", trace));
  CommandGroup group(std::move(children));
  trace->fail_execute = "C";
  trace->fail_undo = "A";

  CHECK_FALSE(group.execute(fixture.document));
  CHECK(
    trace->calls ==
    std::vector<std::string>{"execute A", "execute B", "execute C", "undo B", "undo A"});
  fixture.check_names("R2", "C1"); // Only A remains applied.

  trace->fail_execute.clear();
  trace->fail_undo.clear();
  trace->calls.clear();
  SECTION("retrying rollback only undoes the remaining applied child") {
    REQUIRE(group.undo(fixture.document));
    CHECK(trace->calls == std::vector<std::string>{"undo A"});
    fixture.check_names("R1", "C1");

    trace->calls.clear();
    CHECK_FALSE(group.undo(fixture.document));
    CHECK(trace->calls.empty());
    REQUIRE(group.execute(fixture.document));
    CHECK(trace->calls == std::vector<std::string>{"execute A", "execute B", "execute C"});
    fixture.check_names("R4", "C1");
  }
  SECTION("retrying execution restores rolled-back children before the failed child") {
    REQUIRE(group.execute(fixture.document));
    CHECK(trace->calls == std::vector<std::string>{"execute B", "execute C"});
    fixture.check_names("R4", "C1");

    trace->calls.clear();
    REQUIRE(group.undo(fixture.document));
    CHECK(trace->calls == std::vector<std::string>{"undo C", "undo B", "undo A"});
    fixture.check_names("R1", "C1");
  }
}

} // namespace
