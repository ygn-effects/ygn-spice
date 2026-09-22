// Keep the public header first so this test also verifies that it is self-contained.
#include "connectivity.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <limits>
#include <random>
#include <string>
#include <vector>

namespace {

using ygn::spice::core::analyze_connectivity;
using ygn::spice::core::ComponentInstance;
using ygn::spice::core::ConnectionRef;
using ygn::spice::core::Coordinate;
using ygn::spice::core::directly_connected;
using ygn::spice::core::Junction;
using ygn::spice::core::Net;
using ygn::spice::core::NetLabel;
using ygn::spice::core::Participant;
using ygn::spice::core::Point;
using ygn::spice::core::ResolvedPin;
using ygn::spice::core::Sheet;
using ygn::spice::core::Uuid;
using ygn::spice::core::Wire;

Uuid uuid(const std::string &text) {
  return Uuid::parse(text).value();
}

Point point(const Coordinate::rep x, const Coordinate::rep y) {
  return Point{
    .x = Coordinate::from_nanometres(x),
    .y = Coordinate::from_nanometres(y),
  };
}

TEST_CASE("wire endpoints at the same coordinate are connected") {
  Sheet sheet;
  const auto first_id = uuid("10000000-0000-0000-0000-000000000001");
  const auto second_id = uuid("10000000-0000-0000-0000-000000000002");

  REQUIRE(sheet.add_wire(Wire(first_id, std::array{point(0, 0), point(10, 0)})));
  REQUIRE(sheet.add_wire(Wire(second_id, std::array{point(10, 0), point(20, 0)})));

  const auto connectivity = analyze_connectivity(sheet);

  CHECK(connectivity.are_connected(ConnectionRef::wire(first_id), ConnectionRef::wire(second_id)));
}

TEST_CASE("a wire endpoint on another wire forms a T junction") {
  Sheet sheet;
  const auto trunk_id = uuid("20000000-0000-0000-0000-000000000001");
  const auto branch_id = uuid("20000000-0000-0000-0000-000000000002");

  REQUIRE(sheet.add_wire(Wire(trunk_id, std::array{point(0, 0), point(20, 0)})));
  REQUIRE(sheet.add_wire(Wire(branch_id, std::array{point(10, 0), point(10, 10)})));

  const auto connectivity = analyze_connectivity(sheet);

  CHECK(connectivity.are_connected(ConnectionRef::wire(trunk_id), ConnectionRef::wire(branch_id)));
}

TEST_CASE("crossing wire interiors remain electrically separate") {
  Sheet sheet;
  const auto horizontal_id = uuid("30000000-0000-0000-0000-000000000001");
  const auto vertical_id = uuid("30000000-0000-0000-0000-000000000002");

  REQUIRE(sheet.add_wire(Wire(horizontal_id, std::array{point(-10, 0), point(10, 0)})));
  REQUIRE(sheet.add_wire(Wire(vertical_id, std::array{point(0, -10), point(0, 10)})));

  const auto connectivity = analyze_connectivity(sheet);

  CHECK_FALSE(connectivity.are_connected(
    ConnectionRef::wire(horizontal_id),
    ConnectionRef::wire(vertical_id)));
}

TEST_CASE("an explicit junction connects crossing wire interiors") {
  Sheet sheet;
  const auto horizontal_id = uuid("40000000-0000-0000-0000-000000000001");
  const auto vertical_id = uuid("40000000-0000-0000-0000-000000000002");
  const auto junction_id = uuid("40000000-0000-0000-0000-000000000003");

  REQUIRE(sheet.add_wire(Wire(horizontal_id, std::array{point(-10, 0), point(10, 0)})));
  REQUIRE(sheet.add_wire(Wire(vertical_id, std::array{point(0, -10), point(0, 10)})));
  REQUIRE(sheet.add_junction(Junction(junction_id, point(0, 0))));

  const auto connectivity = analyze_connectivity(sheet);

  CHECK(connectivity.are_connected(
    ConnectionRef::wire(horizontal_id),
    ConnectionRef::wire(vertical_id)));
  CHECK(connectivity.are_connected(
    ConnectionRef::junction(junction_id),
    ConnectionRef::wire(horizontal_id)));
  CHECK(connectivity.are_connected(
    ConnectionRef::junction(junction_id),
    ConnectionRef::wire(vertical_id)));
}

TEST_CASE("equal net labels connect separate physical groups using exact text") {
  Sheet sheet;
  const auto first_wire_id = uuid("50000000-0000-0000-0000-000000000001");
  const auto second_wire_id = uuid("50000000-0000-0000-0000-000000000002");
  const auto third_wire_id = uuid("50000000-0000-0000-0000-000000000003");
  const auto first_label_id = uuid("50000000-0000-0000-0000-000000000004");
  const auto second_label_id = uuid("50000000-0000-0000-0000-000000000005");
  const auto third_label_id = uuid("50000000-0000-0000-0000-000000000006");

  REQUIRE(sheet.add_wire(Wire(first_wire_id, std::array{point(0, 0), point(10, 0)})));
  REQUIRE(sheet.add_wire(Wire(second_wire_id, std::array{point(100, 0), point(110, 0)})));
  REQUIRE(sheet.add_wire(Wire(third_wire_id, std::array{point(200, 0), point(210, 0)})));
  REQUIRE(sheet.add_label(NetLabel(first_label_id, "signal", point(5, 0))));
  REQUIRE(sheet.add_label(NetLabel(second_label_id, "signal", point(105, 0))));
  REQUIRE(sheet.add_label(NetLabel(third_label_id, "Signal", point(205, 0))));

  const auto connectivity = analyze_connectivity(sheet);

  CHECK(connectivity.are_connected(
    ConnectionRef::wire(first_wire_id),
    ConnectionRef::wire(second_wire_id)));
  CHECK(connectivity.are_connected(
    ConnectionRef::label(first_label_id),
    ConnectionRef::wire(first_wire_id)));
  CHECK(connectivity.are_connected(
    ConnectionRef::label(second_label_id),
    ConnectionRef::wire(second_wire_id)));
  CHECK_FALSE(connectivity.are_connected(
    ConnectionRef::wire(first_wire_id),
    ConnectionRef::wire(third_wire_id)));
  CHECK_FALSE(connectivity.are_connected(
    ConnectionRef::label(first_label_id),
    ConnectionRef::label(third_label_id)));
}

TEST_CASE("blank labels attach geometrically without merging by name", "[blank-labels]") {
  Sheet sheet;
  const auto first_wire = Wire::create(std::array{point(0, 0), point(20, 0)});
  const auto second_wire = Wire::create(std::array{point(100, 0), point(120, 0)});
  const auto first_label = NetLabel::create("", point(5, 0));
  const auto same_wire_label = NetLabel::create("", point(15, 0));
  const auto remote_label = NetLabel::create("", point(105, 0));
  REQUIRE(sheet.add_wire(first_wire));
  REQUIRE(sheet.add_wire(second_wire));
  REQUIRE(sheet.add_label(first_label));
  REQUIRE(sheet.add_label(same_wire_label));
  REQUIRE(sheet.add_label(remote_label));

  auto connectivity = analyze_connectivity(sheet);
  const auto first = ConnectionRef::label(first_label.id());
  const auto same_wire = ConnectionRef::label(same_wire_label.id());
  const auto remote = ConnectionRef::label(remote_label.id());
  CHECK(connectivity.are_connected(first, ConnectionRef::wire(first_wire.id())));
  CHECK(connectivity.are_connected(same_wire, ConnectionRef::wire(first_wire.id())));
  CHECK(connectivity.are_connected(remote, ConnectionRef::wire(second_wire.id())));
  // Blank labels can share a net through a wire, but never through their text.
  CHECK(connectivity.are_connected(first, same_wire));
  CHECK_FALSE(connectivity.are_connected(first, remote));
  CHECK_FALSE(connectivity.are_connected(remote, first));
  CHECK_FALSE(connectivity.are_connected(
    ConnectionRef::wire(first_wire.id()),
    ConnectionRef::wire(second_wire.id())));
  CHECK(connectivity.nets().size() == 2);
}

TEST_CASE("blank labels without wires remain isolated", "[blank-labels]") {
  for (const bool coincident : {false, true}) {
    CAPTURE(coincident);
    Sheet sheet;
    const auto first_label = NetLabel::create("", point(0, 0));
    const auto second_label = NetLabel::create("", coincident ? point(0, 0) : point(100, 0));
    REQUIRE(sheet.add_label(first_label));
    REQUIRE(sheet.add_label(second_label));

    auto connectivity = analyze_connectivity(sheet);
    const auto first = ConnectionRef::label(first_label.id());
    const auto second = ConnectionRef::label(second_label.id());
    CHECK(connectivity.are_connected(first, first));
    CHECK(connectivity.are_connected(second, second));
    CHECK_FALSE(connectivity.are_connected(first, second));
    CHECK_FALSE(connectivity.are_connected(second, first));
    CHECK(connectivity.nets().size() == 2);
  }
}

TEST_CASE("net labels are scoped to the sheet being analyzed") {
  Sheet first_sheet;
  Sheet second_sheet;
  const auto first_label_id = uuid("51000000-0000-0000-0000-000000000001");
  const auto second_label_id = uuid("51000000-0000-0000-0000-000000000002");

  REQUIRE(first_sheet.add_label(NetLabel(first_label_id, "signal", point(0, 0))));
  REQUIRE(second_sheet.add_label(NetLabel(second_label_id, "signal", point(0, 0))));

  const auto connectivity = analyze_connectivity(first_sheet);

  CHECK_FALSE(connectivity.are_connected(
    ConnectionRef::label(first_label_id),
    ConnectionRef::label(second_label_id)));
}

TEST_CASE("resolved component pins participate without embedding library data") {
  Sheet sheet;
  const auto first_component_id = uuid("60000000-0000-0000-0000-000000000001");
  const auto second_component_id = uuid("60000000-0000-0000-0000-000000000002");
  const auto shared_library_pin_id = uuid("60000000-0000-0000-0000-000000000003");
  const auto wire_id = uuid("60000000-0000-0000-0000-000000000004");

  REQUIRE(sheet.add_component(ComponentInstance(first_component_id, "R1", point(0, 0))));
  REQUIRE(sheet.add_component(ComponentInstance(second_component_id, "R2", point(100, 0))));
  REQUIRE(sheet.add_wire(Wire(wire_id, std::array{point(10, 0), point(20, 0)})));

  const std::array resolved_pins{
    ResolvedPin{
      .component_id = first_component_id,
      .pin_id = shared_library_pin_id,
      .position = point(10, 0),
    },
    ResolvedPin{
      .component_id = second_component_id,
      .pin_id = shared_library_pin_id,
      .position = point(100, 0),
    },
  };
  const auto connectivity = analyze_connectivity(sheet, resolved_pins);

  CHECK(connectivity.are_connected(
    ConnectionRef::pin(first_component_id, shared_library_pin_id),
    ConnectionRef::wire(wire_id)));
  CHECK_FALSE(connectivity.are_connected(
    ConnectionRef::pin(second_component_id, shared_library_pin_id),
    ConnectionRef::wire(wire_id)));
}

TEST_CASE("connectivity is an immutable snapshot of document topology") {
  Sheet sheet;
  const auto horizontal_id = uuid("70000000-0000-0000-0000-000000000001");
  const auto vertical_id = uuid("70000000-0000-0000-0000-000000000002");
  const auto junction_id = uuid("70000000-0000-0000-0000-000000000003");

  REQUIRE(sheet.add_wire(Wire(horizontal_id, std::array{point(-10, 0), point(10, 0)})));
  REQUIRE(sheet.add_wire(Wire(vertical_id, std::array{point(0, -10), point(0, 10)})));
  REQUIRE(sheet.add_junction(Junction(junction_id, point(0, 0))));

  const auto before_removal = analyze_connectivity(sheet);
  REQUIRE(sheet.remove_junction(junction_id).has_value());
  const auto after_removal = analyze_connectivity(sheet);

  CHECK(before_removal.are_connected(
    ConnectionRef::wire(horizontal_id),
    ConnectionRef::wire(vertical_id)));
  CHECK_FALSE(after_removal.are_connected(
    ConnectionRef::wire(horizontal_id),
    ConnectionRef::wire(vertical_id)));
}

TEST_CASE("connectivity output is deterministic across insertion order") {
  const auto first_wire_id = uuid("80000000-0000-0000-0000-000000000001");
  const auto second_wire_id = uuid("80000000-0000-0000-0000-000000000002");
  const auto isolated_wire_id = uuid("80000000-0000-0000-0000-000000000003");
  const Wire first_wire(first_wire_id, std::array{point(0, 0), point(10, 0)});
  const Wire second_wire(second_wire_id, std::array{point(10, 0), point(20, 0)});
  const Wire isolated_wire(isolated_wire_id, std::array{point(100, 0), point(110, 0)});

  Sheet forward_sheet;
  REQUIRE(forward_sheet.add_wire(first_wire));
  REQUIRE(forward_sheet.add_wire(second_wire));
  REQUIRE(forward_sheet.add_wire(isolated_wire));

  Sheet reverse_sheet;
  REQUIRE(reverse_sheet.add_wire(isolated_wire));
  REQUIRE(reverse_sheet.add_wire(second_wire));
  REQUIRE(reverse_sheet.add_wire(first_wire));

  auto forward = analyze_connectivity(forward_sheet);
  auto reverse = analyze_connectivity(reverse_sheet);

  CHECK(forward.nets() == reverse.nets());
}

TEST_CASE("point-on-segment checks support the full document coordinate scale") {
  const auto trunk_id = uuid("90000000-0000-0000-0000-000000000001");
  const auto branch_id = uuid("90000000-0000-0000-0000-000000000002");
  constexpr auto minimum = std::numeric_limits<Coordinate::rep>::min();
  constexpr auto maximum = std::numeric_limits<Coordinate::rep>::max();

  // Exercise both axes using orthogonal wires. The branch meets the trunk's
  // interior, so a shared-endpoint check alone cannot satisfy this test.
  for (const bool vertical : {false, true}) {
    for (const bool reverse : {false, true}) {
      CAPTURE(vertical, reverse);
      Sheet sheet;
      const auto start = vertical ? point(0, minimum) : point(minimum, 0);
      const auto end = vertical ? point(0, maximum) : point(maximum, 0);
      const auto branch_end = vertical ? point(maximum, 0) : point(0, maximum);
      REQUIRE(
        sheet.add_wire(Wire(trunk_id, reverse ? std::array{end, start} : std::array{start, end})));
      REQUIRE(sheet.add_wire(Wire(branch_id, std::array{point(0, 0), branch_end})));

      const auto connectivity = analyze_connectivity(sheet);
      CHECK(
        connectivity.are_connected(ConnectionRef::wire(trunk_id), ConnectionRef::wire(branch_id)));
    }
  }
}

TEST_CASE("wire attachments respect segment bounds in either endpoint order", "[regression]") {
  struct AttachmentCase {
    Coordinate::rep along;
    Coordinate::rep off_line;
    bool connected;
  };
  const std::array cases{
    AttachmentCase{-30, 0, false},
    AttachmentCase{-20, 0, true},
    AttachmentCase{0, 0, true},
    AttachmentCase{20, 0, true},
    AttachmentCase{30, 0, false},
    AttachmentCase{0, 1, false},
  };

  for (const bool vertical : {false, true}) {
    // A nonzero fixed coordinate catches accidental mixing of the x and y bounds.
    const auto position = [vertical](Coordinate::rep along, Coordinate::rep off_line) {
      return vertical ? point(7 + off_line, along) : point(along, 7 + off_line);
    };

    for (const bool reverse : {false, true}) {
      for (const auto *kind : {"wire", "junction", "label"}) {
        for (const auto &test : cases) {
          CAPTURE(vertical, reverse, kind, test.along, test.off_line, test.connected);
          Sheet sheet;
          const auto wire_id = uuid("a0000000-0000-0000-0000-000000000001");
          const auto attachment_id = uuid("a0000000-0000-0000-0000-000000000002");
          const auto start = position(-20, 0);
          const auto end = position(20, 0);
          const auto anchor = position(test.along, test.off_line);
          REQUIRE(sheet.add_wire(
            Wire(wire_id, reverse ? std::array{end, start} : std::array{start, end})));

          const auto attachment = [&]() -> ConnectionRef {
            if (std::string(kind) == "wire") {
              REQUIRE(sheet.add_wire(Wire(
                attachment_id,
                std::array{anchor, position(test.along, test.off_line + 100)})));
              return ConnectionRef::wire(attachment_id);
            }
            if (std::string(kind) == "junction") {
              REQUIRE(sheet.add_junction(Junction(attachment_id, anchor)));
              return ConnectionRef::junction(attachment_id);
            }
            REQUIRE(sheet.add_label(NetLabel(attachment_id, "signal", anchor)));
            return ConnectionRef::label(attachment_id);
          }();

          auto connectivity = analyze_connectivity(sheet);
          const auto wire_ref = ConnectionRef::wire(wire_id);
          CHECK(connectivity.are_connected(wire_ref, attachment) == test.connected);
          CHECK(connectivity.are_connected(attachment, wire_ref) == test.connected);
          CHECK(connectivity.are_connected(wire_ref, wire_ref));
          CHECK(connectivity.are_connected(attachment, attachment));
          CHECK(connectivity.nets().size() == (test.connected ? 1 : 2));
        }
      }
    }
  }
}

TEST_CASE("unrelated junctions labels and resolved pins coexist safely", "[regression]") {
  Sheet sheet;
  const auto junction_id = uuid("a1000000-0000-0000-0000-000000000001");
  const auto label_id = uuid("a1000000-0000-0000-0000-000000000002");
  const auto component_id = uuid("a1000000-0000-0000-0000-000000000003");
  const auto pin_id = uuid("a1000000-0000-0000-0000-000000000004");
  REQUIRE(sheet.add_junction(Junction(junction_id, point(0, 0))));
  REQUIRE(sheet.add_label(NetLabel(label_id, "signal", point(100, 100))));
  REQUIRE(sheet.add_component(ComponentInstance(component_id, "R1", point(200, 200))));
  const std::array pins{ResolvedPin{component_id, pin_id, point(200, 200)}};

  // Analysis must complete even when every participant has a different kind.
  auto connectivity = analyze_connectivity(sheet, pins);
  const std::array refs{
    ConnectionRef::junction(junction_id),
    ConnectionRef::label(label_id),
    ConnectionRef::pin(component_id, pin_id),
  };
  REQUIRE(connectivity.nets().size() == refs.size());
  for (std::size_t i = 0; i < refs.size(); ++i) {
    for (std::size_t j = 0; j < refs.size(); ++j) {
      CAPTURE(i, j);
      CHECK(connectivity.are_connected(refs[i], refs[j]) == (i == j));
    }
  }
}

TEST_CASE("junction and label connections compose into one net", "[regression]") {
  Sheet sheet;
  const auto horizontal = Wire::create(std::array{point(-20, 0), point(20, 0)});
  const auto vertical = Wire::create(std::array{point(0, -20), point(0, 20)});
  const auto remote = Wire::create(std::array{point(100, 0), point(120, 0)});
  const auto junction = Junction::create(point(0, 0));
  const auto local_label = NetLabel::create("signal", point(10, 0));
  const auto remote_label = NetLabel::create("signal", point(110, 0));
  REQUIRE(sheet.add_wire(horizontal));
  REQUIRE(sheet.add_wire(vertical));
  REQUIRE(sheet.add_wire(remote));
  REQUIRE(sheet.add_junction(junction));
  REQUIRE(sheet.add_label(local_label));
  REQUIRE(sheet.add_label(remote_label));

  auto connectivity = analyze_connectivity(sheet);
  const std::array refs{
    ConnectionRef::wire(horizontal.id()),
    ConnectionRef::wire(vertical.id()),
    ConnectionRef::wire(remote.id()),
    ConnectionRef::junction(junction.id()),
    ConnectionRef::label(local_label.id()),
    ConnectionRef::label(remote_label.id()),
  };
  REQUIRE(connectivity.nets().size() == 1);
  for (std::size_t i = 0; i < refs.size(); ++i) {
    for (std::size_t j = 0; j < refs.size(); ++j) {
      CAPTURE(i, j);
      CHECK(connectivity.are_connected(refs[i], refs[j]));
    }
  }
}

TEST_CASE("traversal retains isolated wires after a connected chain", "[regression]") {
  const auto first = Wire::create(std::array{point(0, 0), point(10, 0)});
  const auto middle = Wire::create(std::array{point(10, 0), point(20, 0)});
  const auto last = Wire::create(std::array{point(20, 0), point(30, 0)});
  const auto isolated = Wire::create(std::array{point(100, 100), point(110, 100)});
  Sheet sheet;
  REQUIRE(sheet.add_wire(first));
  REQUIRE(sheet.add_wire(middle));
  REQUIRE(sheet.add_wire(last));
  REQUIRE(sheet.add_wire(isolated));

  auto connectivity = analyze_connectivity(sheet);
  const auto isolated_ref = ConnectionRef::wire(isolated.id());
  REQUIRE(connectivity.nets().size() == 2);
  CHECK(
    connectivity.are_connected(ConnectionRef::wire(first.id()), ConnectionRef::wire(last.id())));
  CHECK(connectivity.are_connected(isolated_ref, isolated_ref));
  CHECK_FALSE(connectivity.are_connected(ConnectionRef::wire(first.id()), isolated_ref));
  for (const auto &wire : {first, middle, last, isolated}) {
    std::size_t memberships = 0;
    for (const auto &net : connectivity.nets()) {
      if (net.contains_ref(ConnectionRef::wire(wire.id()))) {
        ++memberships;
      }
    }
    CHECK(memberships == 1);
  }
}

TEST_CASE("pins attach only to wire endpoints in either argument order", "[pin-rules]") {
  struct PinCase {
    Coordinate::rep along;
    Coordinate::rep off_line;
    bool connected;
  };
  const std::array cases{
    PinCase{-30, 0, false},
    PinCase{-20, 0, true},
    PinCase{0, 0, false},
    PinCase{20, 0, true},
    PinCase{30, 0, false},
    PinCase{-20, 1, false},
    PinCase{20, -1, false},
    PinCase{0, 1, false},
  };

  for (const bool vertical : {false, true}) {
    const auto position = [vertical](Coordinate::rep along, Coordinate::rep off_line) {
      return vertical ? point(7 + off_line, along) : point(along, 7 + off_line);
    };
    for (const bool reverse : {false, true}) {
      for (const auto &test : cases) {
        CAPTURE(vertical, reverse, test.along, test.off_line, test.connected);
        Sheet sheet;
        const auto start = position(-20, 0);
        const auto end = position(20, 0);
        const auto wire = Wire::create(reverse ? std::array{end, start} : std::array{start, end});
        const auto component = ComponentInstance::create("R1", point(100, 100));
        const auto pin_id = uuid("b0000000-0000-0000-0000-000000000001");
        const auto pin_position = position(test.along, test.off_line);
        REQUIRE(sheet.add_wire(wire));
        REQUIRE(sheet.add_component(component));
        const std::array pins{ResolvedPin{component.id(), pin_id, pin_position}};
        const auto wire_ref = ConnectionRef::wire(wire.id());
        const auto pin_ref = ConnectionRef::pin(component.id(), pin_id);

        // Query symmetry alone cannot reveal a directed edge in the graph builder.
        const Participant wire_participant{wire_ref, wire.points()};
        const Participant pin_participant{pin_ref, pin_position};
        CHECK(directly_connected(wire_participant, pin_participant) == test.connected);
        CHECK(directly_connected(pin_participant, wire_participant) == test.connected);

        auto connectivity = analyze_connectivity(sheet, pins);
        CHECK(connectivity.are_connected(wire_ref, pin_ref) == test.connected);
        CHECK(connectivity.are_connected(pin_ref, wire_ref) == test.connected);
        CHECK(connectivity.nets().size() == (test.connected ? 1 : 2));
        for (const auto &ref : {wire_ref, pin_ref}) {
          std::size_t memberships = 0;
          for (const auto &net : connectivity.nets()) {
            memberships += net.contains_ref(ref);
          }
          CHECK(memberships == 1);
        }
      }
    }
  }
}

TEST_CASE("coincident point participants need a wire except for matching labels", "[pin-rules]") {
  for (const bool matching_labels : {false, true}) {
    CAPTURE(matching_labels);
    Sheet sheet;
    const auto position = point(10, 20);
    const auto first = ComponentInstance::create("R1", position);
    const auto second = ComponentInstance::create("R2", position);
    const auto pin_id = uuid("b1000000-0000-0000-0000-000000000001");
    const auto other_pin_id = uuid("b1000000-0000-0000-0000-000000000002");
    const auto first_junction = Junction::create(position);
    const auto second_junction = Junction::create(position);
    const auto first_label = NetLabel::create("signal", position);
    const auto second_label = NetLabel::create(matching_labels ? "signal" : "Signal", position);
    REQUIRE(sheet.add_component(first));
    REQUIRE(sheet.add_component(second));
    REQUIRE(sheet.add_junction(first_junction));
    REQUIRE(sheet.add_junction(second_junction));
    REQUIRE(sheet.add_label(first_label));
    REQUIRE(sheet.add_label(second_label));
    const std::array pins{
      ResolvedPin{first.id(), pin_id, position},
      ResolvedPin{second.id(), pin_id, position},
      ResolvedPin{first.id(), other_pin_id, position},
    };
    const std::array refs{
      ConnectionRef::pin(first.id(), pin_id),
      ConnectionRef::pin(second.id(), pin_id),
      ConnectionRef::pin(first.id(), other_pin_id),
      ConnectionRef::junction(first_junction.id()),
      ConnectionRef::junction(second_junction.id()),
      ConnectionRef::label(first_label.id()),
      ConnectionRef::label(second_label.id()),
    };

    auto connectivity = analyze_connectivity(sheet, pins);
    CHECK(connectivity.nets().size() == refs.size() - (matching_labels ? 1 : 0));
    for (std::size_t i = 0; i < refs.size(); ++i) {
      for (std::size_t j = 0; j < refs.size(); ++j) {
        CAPTURE(i, j);
        const bool both_labels = refs[i].kind() == ygn::spice::core::ConnectionKind::label &&
          refs[j].kind() == ygn::spice::core::ConnectionKind::label;
        const bool expected = i == j || (matching_labels && both_labels);
        CHECK(connectivity.are_connected(refs[i], refs[j]) == expected);
      }
    }
  }
}

TEST_CASE("a junction and label on wire interiors do not attach a coincident pin", "[pin-rules]") {
  Sheet sheet;
  const auto horizontal = Wire::create(std::array{point(-20, 0), point(20, 0)});
  const auto vertical = Wire::create(std::array{point(0, -20), point(0, 20)});
  const auto junction = Junction::create(point(0, 0));
  const auto label = NetLabel::create("signal", point(0, 0));
  const auto component = ComponentInstance::create("R1", point(0, 0));
  const auto pin_id = uuid("b2000000-0000-0000-0000-000000000001");
  REQUIRE(sheet.add_wire(horizontal));
  REQUIRE(sheet.add_wire(vertical));
  REQUIRE(sheet.add_junction(junction));
  REQUIRE(sheet.add_label(label));
  REQUIRE(sheet.add_component(component));
  const std::array pins{ResolvedPin{component.id(), pin_id, point(0, 0)}};
  const auto pin_ref = ConnectionRef::pin(component.id(), pin_id);
  const std::array connected_refs{
    ConnectionRef::wire(horizontal.id()),
    ConnectionRef::wire(vertical.id()),
    ConnectionRef::junction(junction.id()),
    ConnectionRef::label(label.id()),
  };

  auto connectivity = analyze_connectivity(sheet, pins);
  CHECK(connectivity.nets().size() == 2);
  CHECK(connectivity.are_connected(pin_ref, pin_ref));
  for (const auto &ref : connected_refs) {
    CHECK(connectivity.are_connected(connected_refs.front(), ref));
    CHECK_FALSE(connectivity.are_connected(pin_ref, ref));
    CHECK_FALSE(connectivity.are_connected(ref, pin_ref));
  }
}

TEST_CASE(
  "wire endpoints connect pins indirectly to other pins labels and junctions", "[pin-rules]") {
  Sheet sheet;
  const auto wire = Wire::create(std::array{point(0, 0), point(20, 0)});
  const auto first = ComponentInstance::create("R1", point(0, 0));
  const auto second = ComponentInstance::create("R2", point(20, 0));
  const auto pin_id = uuid("b3000000-0000-0000-0000-000000000001");
  const auto junction = Junction::create(point(10, 0));
  const auto label = NetLabel::create("signal", point(5, 0));
  REQUIRE(sheet.add_wire(wire));
  REQUIRE(sheet.add_component(first));
  REQUIRE(sheet.add_component(second));
  REQUIRE(sheet.add_junction(junction));
  REQUIRE(sheet.add_label(label));
  const std::array pins{
    ResolvedPin{first.id(), pin_id, point(0, 0)},
    ResolvedPin{second.id(), pin_id, point(20, 0)},
  };
  const std::array refs{
    ConnectionRef::wire(wire.id()),
    ConnectionRef::pin(first.id(), pin_id),
    ConnectionRef::pin(second.id(), pin_id),
    ConnectionRef::junction(junction.id()),
    ConnectionRef::label(label.id()),
  };

  auto connectivity = analyze_connectivity(sheet, pins);
  CHECK(connectivity.nets().size() == 1);
  for (std::size_t i = 0; i < refs.size(); ++i) {
    for (std::size_t j = 0; j < refs.size(); ++j) {
      CAPTURE(i, j);
      CHECK(connectivity.are_connected(refs[i], refs[j]));
    }
  }
}

TEST_CASE("connection reference ordering uses kind and both identity fields", "[ordering]") {
  const auto low = uuid("00000000-0000-0000-0000-000000000001");
  const auto high = uuid("ffffffff-ffff-ffff-ffff-ffffffffffff");
  // Pin factories take component first; their ordering key stores pin first.
  const std::array ordered{
    ConnectionRef::wire(low),
    ConnectionRef::wire(high),
    ConnectionRef::junction(low),
    ConnectionRef::label(low),
    ConnectionRef::pin(low, low),
    ConnectionRef::pin(high, low),
    ConnectionRef::pin(low, high),
    ConnectionRef::pin(high, high),
  };
  for (std::size_t i = 0; i < ordered.size(); ++i) {
    for (std::size_t j = 0; j < ordered.size(); ++j) {
      CAPTURE(i, j);
      CHECK((ordered[i] < ordered[j]) == (i < j));
      CHECK((ordered[i] == ordered[j]) == (i == j));
      CHECK((ordered[i] != ordered[j]) == (i != j));
    }
  }
}

TEST_CASE("net comparisons handle empty values prefixes and unequal lengths", "[ordering]") {
  // Every subset is connected through the shared endpoint at the origin.
  const std::array wires{
    Wire(uuid("c0000000-0000-0000-0000-000000000001"), std::array{point(0, 0), point(10, 0)}),
    Wire(uuid("c0000000-0000-0000-0000-000000000002"), std::array{point(0, 0), point(0, 10)}),
    Wire(uuid("c0000000-0000-0000-0000-000000000003"), std::array{point(-10, 0), point(0, 0)}),
  };
  // Explicit lexicographical order: length decides only after a matching prefix.
  const std::vector<std::vector<std::size_t>> sequences{
    {},
    {0},
    {0, 1},
    {0, 1, 2},
    {0, 2},
    {1},
    {1, 2},
    {2},
  };
  const std::array names{"[]", "[a]", "[a,b]", "[a,b,c]", "[a,c]", "[b]", "[b,c]", "[c]"};
  std::vector<Net> ordered;
  for (const auto &sequence : sequences) {
    if (sequence.empty()) {
      ordered.emplace_back();
      continue;
    }
    Sheet sheet;
    std::vector<ConnectionRef> expected;
    for (const auto index : sequence) {
      expected.push_back(ConnectionRef::wire(wires[index].id()));
    }
    for (auto it = sequence.rbegin(); it != sequence.rend(); ++it) {
      REQUIRE(sheet.add_wire(wires[*it]));
    }
    const auto connectivity = analyze_connectivity(sheet);
    REQUIRE(connectivity.nets().size() == 1);
    REQUIRE(connectivity.nets().front().connections() == expected);
    ordered.push_back(connectivity.nets().front());
  }
  for (std::size_t i = 0; i < ordered.size(); ++i) {
    const auto copy = ordered[i];
    CHECK(copy == ordered[i]);
    CHECK_FALSE(copy < ordered[i]);
    CHECK_FALSE(ordered[i] < copy);
    for (std::size_t j = 0; j < ordered.size(); ++j) {
      CAPTURE(names[i], names[j]);
      CHECK((ordered[i] < ordered[j]) == (i < j));
      CHECK((ordered[i] == ordered[j]) == (i == j));
    }
  }
}

TEST_CASE("mixed connectivity is deterministic across participant permutations", "[ordering]") {
  std::array wires{
    Wire(uuid("c1000000-0000-0000-0000-000000000001"), std::array{point(0, 0), point(20, 0)}),
    Wire(uuid("c1000000-0000-0000-0000-000000000002"), std::array{point(20, 0), point(20, 20)}),
    Wire(uuid("c1000000-0000-0000-0000-000000000003"), std::array{point(20, 20), point(0, 20)}),
    Wire(uuid("c1000000-0000-0000-0000-000000000004"), std::array{point(0, 20), point(0, 0)}),
    Wire(uuid("c1000000-0000-0000-0000-000000000005"), std::array{point(100, 0), point(120, 0)}),
    Wire(uuid("c1000000-0000-0000-0000-000000000006"), std::array{point(300, 0), point(320, 0)}),
  };
  std::array junctions{
    Junction(uuid("c2000000-0000-0000-0000-000000000001"), point(20, 0)),
    Junction(uuid("c2000000-0000-0000-0000-000000000002"), point(0, 20)),
  };
  std::array labels{
    NetLabel(uuid("c3000000-0000-0000-0000-000000000001"), "signal", point(10, 0)),
    NetLabel(uuid("c3000000-0000-0000-0000-000000000002"), "signal", point(110, 0)),
    NetLabel(uuid("c3000000-0000-0000-0000-000000000003"), "other", point(310, 0)),
  };
  std::array components{
    ComponentInstance(uuid("c4000000-0000-0000-0000-000000000001"), "R1", point(0, 0)),
    ComponentInstance(uuid("c4000000-0000-0000-0000-000000000002"), "R2", point(100, 0)),
  };
  const auto shared_pin_id = uuid("c5000000-0000-0000-0000-000000000001");
  const auto other_pin_id = uuid("c5000000-0000-0000-0000-000000000002");
  std::array pins{
    ResolvedPin{components[0].id(), shared_pin_id, point(0, 0)},
    ResolvedPin{components[1].id(), shared_pin_id, point(100, 0)},
    ResolvedPin{components[0].id(), other_pin_id, point(400, 0)},
  };

  // Record expected membership before changing insertion order. The square is a
  // cycle, and matching labels connect it to the remote wire and its pin.
  std::vector<ConnectionRef> main_group;
  for (std::size_t i = 0; i < 5; ++i) {
    main_group.push_back(ConnectionRef::wire(wires[i].id()));
  }
  for (const auto &junction : junctions) {
    main_group.push_back(ConnectionRef::junction(junction.id()));
  }
  for (std::size_t i = 0; i < 2; ++i) {
    main_group.push_back(ConnectionRef::label(labels[i].id()));
    main_group.push_back(ConnectionRef::pin(pins[i].component_id, pins[i].pin_id));
  }
  const std::vector<std::vector<ConnectionRef>> groups{
    main_group,
    {ConnectionRef::wire(wires[5].id()), ConnectionRef::label(labels[2].id())},
    {ConnectionRef::pin(pins[2].component_id, pins[2].pin_id)},
  };

  std::mt19937 random(12);
  std::vector<Net> baseline;
  for (std::size_t trial = 0; trial < 32; ++trial) {
    CAPTURE(trial);
    if (trial != 0) {
      std::shuffle(wires.begin(), wires.end(), random);
      std::shuffle(junctions.begin(), junctions.end(), random);
      std::shuffle(labels.begin(), labels.end(), random);
      std::shuffle(components.begin(), components.end(), random);
      std::shuffle(pins.begin(), pins.end(), random);
    }
    Sheet sheet;
    for (const auto &wire : wires) {
      auto endpoints = wire.points();
      if (trial % 2 != 0) {
        std::swap(endpoints[0], endpoints[1]);
      }
      REQUIRE(sheet.add_wire(Wire(wire.id(), endpoints)));
    }
    for (const auto &junction : junctions) {
      REQUIRE(sheet.add_junction(junction));
    }
    for (const auto &label : labels) {
      REQUIRE(sheet.add_label(label));
    }
    for (const auto &component : components) {
      REQUIRE(sheet.add_component(component));
    }
    auto connectivity = analyze_connectivity(sheet, pins);
    REQUIRE(connectivity.nets().size() == groups.size());
    if (trial == 0) {
      baseline = connectivity.nets();
    } else {
      CHECK(connectivity.nets() == baseline);
    }
    CHECK(std::is_sorted(connectivity.nets().begin(), connectivity.nets().end()));
    for (auto &net : connectivity.nets()) {
      CHECK(std::is_sorted(net.connections().begin(), net.connections().end()));
    }
    for (std::size_t group = 0; group < groups.size(); ++group) {
      for (std::size_t member = 0; member < groups[group].size(); ++member) {
        CAPTURE(group, member);
        const auto &ref = groups[group][member];
        CHECK(connectivity.are_connected(groups[group].front(), ref));
        CHECK_FALSE(connectivity.are_connected(groups[(group + 1) % groups.size()].front(), ref));
        std::size_t occurrences = 0;
        for (const auto &net : connectivity.nets()) {
          occurrences += std::count(net.connections().begin(), net.connections().end(), ref);
        }
        CHECK(occurrences == 1);
      }
    }
  }
}

TEST_CASE("an empty sheet has no nets and unknown references are disconnected", "[ordering]") {
  const auto unknown = ConnectionRef::wire(uuid("c6000000-0000-0000-0000-000000000001"));
  const auto connectivity = analyze_connectivity(Sheet{});
  CHECK(connectivity.nets().empty());
  CHECK_FALSE(connectivity.are_connected(unknown, unknown));
}

} // namespace
