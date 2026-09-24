#pragma once

// Shared helpers for the command, command group, and history tests.

#include "commands.hpp"
#include "document.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_tostring.hpp>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// Print enumerator names instead of integers in assertion failures.
CATCH_REGISTER_ENUM(
  ygn::spice::core::CommandOutcome,
  ygn::spice::core::CommandOutcome::Completed,
  ygn::spice::core::CommandOutcome::Unchanged)
CATCH_REGISTER_ENUM(
  ygn::spice::core::CommandReason,
  ygn::spice::core::CommandReason::NotFound,
  ygn::spice::core::CommandReason::NoChangeNeeded)

namespace ygn::spice::core::test {

using Calls = std::vector<std::string>;
using Children = std::vector<std::unique_ptr<Command>>;

// A completed result carries no reason; a rejected result carries the reason.
inline void check_result(
  const CommandResult &result,
  CommandOutcome outcome,
  std::optional<CommandReason> reason = std::nullopt) {
  REQUIRE(result.outcome() == outcome);
  REQUIRE(result.reason().has_value() == reason.has_value());
  if (reason.has_value()) {
    CHECK(*result.reason() == *reason);
  }
}

// Records execute and undo calls in order. The command whose label matches
// fail_execute is rejected without touching the document, like any rejected
// command.
struct Trace {
  Calls calls;
  std::string fail_execute;
};

class TracedRename final : public Command {
public:
  TracedRename(const Uuid &id, std::string name, std::string label, std::shared_ptr<Trace> trace)
      : rename_(id, std::move(name)), label_(std::move(label)), trace_(std::move(trace)) {
  }

  CommandResult execute(Document &document) override {
    trace_->calls.push_back("execute " + label_);
    if (trace_->fail_execute == label_) {
      return CommandResult::unchanged(CommandReason::NotFound);
    }
    return rename_.execute(document);
  }

  void undo(Document &document) override {
    trace_->calls.push_back("undo " + label_);
    rename_.undo(document);
  }

private:
  RenameComponentCommand rename_;
  std::string label_;
  std::shared_ptr<Trace> trace_;
};

inline std::unique_ptr<Command> rename_command(const Uuid &id, std::string name) {
  return std::make_unique<RenameComponentCommand>(id, std::move(name));
}

inline std::unique_ptr<Command>
traced(const Uuid &id, std::string name, std::string label, const std::shared_ptr<Trace> &trace) {
  return std::make_unique<TracedRename>(id, std::move(name), std::move(label), trace);
}

// Two components with distinct names for tests that edit one or both.
struct GroupFixture {
  Document document;
  Uuid sheet_id = document.sheets().front().id();
  ComponentInstance resistor = ComponentInstance::create(
    "R1", {Coordinate::from_millimetres(1), Coordinate::from_millimetres(2)});
  ComponentInstance capacitor = ComponentInstance::create(
    "C1", {Coordinate::from_millimetres(3), Coordinate::from_millimetres(4)});

  GroupFixture() {
    REQUIRE(document.add_component(sheet_id, resistor));
    REQUIRE(document.add_component(sheet_id, capacitor));
  }

  void check_names(std::string_view r, std::string_view c) const {
    REQUIRE(document.sheets().size() == 1);
    CHECK(document.sheets().front().id() == sheet_id);
    REQUIRE(document.sheets().front().components().size() == 2);
    const auto *actual_r = document.lookup(resistor.id());
    const auto *actual_c = document.lookup(capacitor.id());
    REQUIRE(actual_r != nullptr);
    REQUIRE(actual_c != nullptr);
    CHECK(actual_r->position() == resistor.position());
    CHECK(actual_c->position() == capacitor.position());
    CHECK(actual_r->designator() == r);
    CHECK(actual_c->designator() == c);
  }
};

} // namespace ygn::spice::core::test
