// Keep the public header first to verify that it is self-contained.
#include "command_group.hpp"

#include "command_test_support.hpp"
#include "commands.hpp"
#include "document.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <stdexcept>
#include <utility>

namespace {

using namespace ygn::spice::core;
using namespace ygn::spice::core::test;

// A group is one atomic transaction. Children execute in order; the first
// rejected child stops execution, the children already applied are undone in
// reverse, and the group returns that child's result. Undo runs the children in
// reverse. A nested group is just another child.

TEST_CASE("a command group executes forwards and undoes backwards repeatedly", "[group]") {
  GroupFixture fixture;
  const auto trace = std::make_shared<Trace>();
  Children children;
  children.push_back(traced(fixture.resistor.id(), "R2", "A", trace));
  children.push_back(traced(fixture.resistor.id(), "R3", "B", trace));
  CommandGroup group(std::move(children));

  // Redo is another execute after undo.
  for (int cycle = 0; cycle < 3; ++cycle) {
    trace->calls.clear();
    check_result(group.execute(fixture.document), CommandOutcome::Completed);
    fixture.check_names("R3", "C1");
    group.undo(fixture.document);
    fixture.check_names("R1", "C1");
    CHECK(trace->calls == Calls{"execute A", "execute B", "undo B", "undo A"});
  }
}

TEST_CASE("a rejected child rolls back only the children already applied", "[group][rejection]") {
  for (const auto &failure : {"A", "B", "C"}) {
    CAPTURE(failure);
    GroupFixture fixture;
    const auto trace = std::make_shared<Trace>();
    trace->fail_execute = failure;
    Children children;
    children.push_back(traced(fixture.resistor.id(), "R2", "A", trace));
    children.push_back(traced(fixture.resistor.id(), "R3", "B", trace));
    children.push_back(traced(fixture.resistor.id(), "R4", "C", trace));
    CommandGroup group(std::move(children));

    check_result(
      group.execute(fixture.document),
      CommandOutcome::Unchanged,
      CommandReason::NotFound);
    fixture.check_names("R1", "C1");
    if (trace->fail_execute == "A") {
      CHECK(trace->calls == Calls{"execute A"});
    } else if (trace->fail_execute == "B") {
      CHECK(trace->calls == Calls{"execute A", "execute B", "undo A"});
    } else {
      CHECK(trace->calls == Calls{"execute A", "execute B", "execute C", "undo B", "undo A"});
    }

    // Nothing is left half-applied, so a retry starts from the first child.
    trace->fail_execute.clear();
    trace->calls.clear();
    check_result(group.execute(fixture.document), CommandOutcome::Completed);
    CHECK(trace->calls == Calls{"execute A", "execute B", "execute C"});
    fixture.check_names("R4", "C1");
    group.undo(fixture.document);
    fixture.check_names("R1", "C1");
  }
}

TEST_CASE("a no-op child rejects the whole group with its reason", "[group][rejection]") {
  GroupFixture fixture;
  Children children;
  children.push_back(rename_command(fixture.resistor.id(), "R2"));
  children.push_back(rename_command(fixture.capacitor.id(), "C1")); // Already named C1.
  children.push_back(rename_command(fixture.capacitor.id(), "C2"));
  CommandGroup group(std::move(children));

  check_result(
    group.execute(fixture.document),
    CommandOutcome::Unchanged,
    CommandReason::NoChangeNeeded);
  fixture.check_names("R1", "C1");
}

TEST_CASE("an empty group is rejected as no change", "[group][rejection]") {
  GroupFixture fixture;
  CommandGroup group(Children{});

  check_result(
    group.execute(fixture.document),
    CommandOutcome::Unchanged,
    CommandReason::NoChangeNeeded);
  fixture.check_names("R1", "C1");
}

// P -> [A -> B] -> Z. Every child renames the resistor, so skipping, repeating
// or reordering a child also shows up in the final name, not just the trace.
std::unique_ptr<CommandGroup>
nested_group(const GroupFixture &fixture, const std::shared_ptr<Trace> &trace) {
  const auto &id = fixture.resistor.id();
  Children inner;
  inner.push_back(traced(id, "R3", "A", trace));
  inner.push_back(traced(id, "R4", "B", trace));
  Children outer;
  outer.push_back(traced(id, "R2", "P", trace));
  outer.push_back(std::make_unique<CommandGroup>(std::move(inner)));
  outer.push_back(traced(id, "R5", "Z", trace));
  return std::make_unique<CommandGroup>(std::move(outer));
}

TEST_CASE("a nested group executes and undoes within one ordered sequence", "[group][nested]") {
  GroupFixture fixture;
  const auto trace = std::make_shared<Trace>();
  const auto group = nested_group(fixture, trace);

  for (int cycle = 0; cycle < 2; ++cycle) {
    trace->calls.clear();
    check_result(group->execute(fixture.document), CommandOutcome::Completed);
    CHECK(trace->calls == Calls{"execute P", "execute A", "execute B", "execute Z"});
    fixture.check_names("R5", "C1");

    trace->calls.clear();
    group->undo(fixture.document);
    CHECK(trace->calls == Calls{"undo Z", "undo B", "undo A", "undo P"});
    fixture.check_names("R1", "C1");
  }
}

TEST_CASE("a rejection anywhere in a nested group rolls back everything", "[group][nested]") {
  GroupFixture fixture;
  const auto trace = std::make_shared<Trace>();
  const auto group = nested_group(fixture, trace);

  SECTION("inside the nested group") {
    trace->fail_execute = "B";
    check_result(
      group->execute(fixture.document),
      CommandOutcome::Unchanged,
      CommandReason::NotFound);
    CHECK(trace->calls == Calls{"execute P", "execute A", "execute B", "undo A", "undo P"});
  }
  SECTION("after the nested group") {
    trace->fail_execute = "Z";
    check_result(
      group->execute(fixture.document),
      CommandOutcome::Unchanged,
      CommandReason::NotFound);
    CHECK(
      trace->calls ==
      Calls{"execute P", "execute A", "execute B", "execute Z", "undo B", "undo A", "undo P"});
  }
  fixture.check_names("R1", "C1");

  trace->fail_execute.clear();
  trace->calls.clear();
  check_result(group->execute(fixture.document), CommandOutcome::Completed);
  CHECK(trace->calls == Calls{"execute P", "execute A", "execute B", "execute Z"});
  fixture.check_names("R5", "C1");
}

TEST_CASE("group undo without a completed execution is a programming error", "[group][contract]") {
  GroupFixture fixture;
  const auto trace = std::make_shared<Trace>();
  Children children;
  children.push_back(traced(fixture.resistor.id(), "R2", "A", trace));
  children.push_back(traced(fixture.capacitor.id(), "C2", "B", trace));
  CommandGroup group(std::move(children));

  SECTION("never executed") {
    // Nothing to set up.
  }
  SECTION("execution rejected") {
    trace->fail_execute = "B";
    check_result(
      group.execute(fixture.document),
      CommandOutcome::Unchanged,
      CommandReason::NotFound);
  }
  SECTION("already undone") {
    check_result(group.execute(fixture.document), CommandOutcome::Completed);
    group.undo(fixture.document);
  }

  REQUIRE_THROWS_AS(group.undo(fixture.document), std::logic_error);
  fixture.check_names("R1", "C1");
}

} // namespace
