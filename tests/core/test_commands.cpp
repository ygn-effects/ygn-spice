// Keep the public header first so this test also verifies that it is self-contained.
#include "commands.hpp"

#include "document.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string_view>

namespace {

using namespace ygn::spice::core;

// First command contract: execute/undo report whether they changed the document.
// Redo is another execute after undo. History and transactions come later.
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

TEST_CASE(
  "component rename executes undoes and redoes without changing identity", "[commands][rename]") {
  RenameFixture fixture;
  RenameComponentCommand command(fixture.target.id(), "R7");
  fixture.check_state("R1");

  // Repeat to ensure redo does not overwrite the original name used by undo.
  for (int cycle = 0; cycle < 3; ++cycle) {
    REQUIRE(command.execute(fixture.document));
    fixture.check_state("R7");
    REQUIRE(command.undo(fixture.document));
    fixture.check_state("R1");
  }
}

TEST_CASE("component rename captures the old name at execution", "[commands][rename]") {
  RenameFixture fixture;
  RenameComponentCommand command(fixture.target.id(), "R7");
  REQUIRE(fixture.document.rename_component(fixture.target.id(), "R3"));

  REQUIRE(command.execute(fixture.document));
  fixture.check_state("R7");
  REQUIRE(command.undo(fixture.document));
  fixture.check_state("R3");
}

TEST_CASE(
  "component rename follows its UUID after collection order changes", "[commands][rename]") {
  RenameFixture fixture;
  RenameComponentCommand command(fixture.target.id(), "R7");
  const auto removed = fixture.document.remove_component(fixture.other.id());
  REQUIRE(removed.has_value());
  REQUIRE(fixture.document.add_component(fixture.sheet_id, *removed));

  REQUIRE(command.execute(fixture.document));
  fixture.check_state("R7");
  REQUIRE(command.undo(fixture.document));
  fixture.check_state("R1");
}

TEST_CASE(
  "component rename with a missing UUID leaves the document unchanged", "[commands][rename]") {
  RenameFixture fixture;
  const auto absent = ComponentInstance::create("R9", fixture.target.position());
  RenameComponentCommand command(absent.id(), "R7");

  CHECK_FALSE(command.execute(fixture.document));
  fixture.check_state("R1");
}

TEST_CASE("component rename rejects a sheet or wire UUID", "[commands][rename][regression]") {
  RenameFixture fixture;
  const auto wire = Wire::create({fixture.target.position(), fixture.other.position()});
  REQUIRE(fixture.document.add_wire(fixture.sheet_id, wire));

  for (const auto &id : {fixture.sheet_id, wire.id()}) {
    RenameComponentCommand command(id, "R7");
    CHECK_FALSE(command.execute(fixture.document));
    fixture.check_state("R1");
    CHECK_FALSE(command.undo(fixture.document));
    fixture.check_state("R1");

    const auto &wires = fixture.document.sheets().front().wires();
    REQUIRE(wires.size() == 1);
    CHECK(wires.front().id() == wire.id());
    CHECK(wires.front().points() == wire.points());
  }
}

TEST_CASE("component rename to its current name reports no change", "[commands][rename]") {
  RenameFixture fixture;
  RenameComponentCommand command(fixture.target.id(), "R1");

  CHECK_FALSE(command.execute(fixture.document));
  fixture.check_state("R1");
}

} // namespace
