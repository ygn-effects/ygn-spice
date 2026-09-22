// Keep the public header first so this test also verifies that it is self-contained.
#include "document.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <concepts>
#include <ranges>
#include <string>
#include <type_traits>
#include <unordered_set>

namespace {

using ygn::spice::core::AnalysisSetup;
using ygn::spice::core::ComponentInstance;
using ygn::spice::core::Coordinate;
using ygn::spice::core::Directive;
using ygn::spice::core::Document;
using ygn::spice::core::Junction;
using ygn::spice::core::NetLabel;
using ygn::spice::core::Point;
using ygn::spice::core::Probe;
using ygn::spice::core::ProbeKind;
using ygn::spice::core::Sheet;
using ygn::spice::core::Uuid;
using ygn::spice::core::Wire;

template <typename T>
concept HasPublicIdField = requires(T &object) { object.id; };

template <typename T>
concept HasPublicIdSetter = requires(T &object, const Uuid &id) { object.set_id(id); };

template <typename T>
concept HasPublicSheetsField = requires(T &object) { object.sheets; };

template <typename T>
concept HasPublicAnalysesField = requires(T &object) { object.analyses; };

template <typename T>
concept HasPublicComponentsField = requires(T &object) { object.components; };

template <typename T>
concept HasPublicWiresField = requires(T &object) { object.wires; };

template <typename T>
concept HasPublicJunctionsField = requires(T &object) { object.junctions; };

template <typename T>
concept HasPublicLabelsField = requires(T &object) { object.labels; };

template <typename T>
concept HasPublicDirectivesField = requires(T &object) { object.directives; };

template <typename T>
concept HasPublicProbesField = requires(T &object) { object.probes; };

template <typename Result>
concept ReadOnlyUuid = std::same_as<std::remove_cvref_t<Result>, Uuid> &&
  (!std::is_lvalue_reference_v<Result> || std::is_const_v<std::remove_reference_t<Result>>);

template <typename Range, typename Value>
concept ReadOnlyRangeOf = std::ranges::forward_range<Range> && std::ranges::sized_range<Range> &&
  std::same_as<std::ranges::range_value_t<Range>, Value> &&
  std::same_as<std::ranges::range_reference_t<Range>, const Value &>;

template <typename T>
concept HasReadOnlyId = requires(const T &object) {
  { object.id() } -> ReadOnlyUuid;
};

template <typename T>
concept HasReadOnlySheets =
  requires(T &object) { requires ReadOnlyRangeOf<decltype(object.sheets()), Sheet>; };

template <typename T>
concept HasReadOnlyAnalyses =
  requires(T &object) { requires ReadOnlyRangeOf<decltype(object.analyses()), AnalysisSetup>; };

template <typename T>
concept HasReadOnlyComponents = requires(T &object) {
  requires ReadOnlyRangeOf<decltype(object.components()), ComponentInstance>;
};

template <typename T>
concept HasReadOnlyWires =
  requires(T &object) { requires ReadOnlyRangeOf<decltype(object.wires()), Wire>; };

template <typename T>
concept HasReadOnlyJunctions =
  requires(T &object) { requires ReadOnlyRangeOf<decltype(object.junctions()), Junction>; };

template <typename T>
concept HasReadOnlyLabels =
  requires(T &object) { requires ReadOnlyRangeOf<decltype(object.labels()), NetLabel>; };

template <typename T>
concept HasReadOnlyDirectives =
  requires(T &object) { requires ReadOnlyRangeOf<decltype(object.directives()), Directive>; };

template <typename T>
concept HasReadOnlyProbes =
  requires(T &object) { requires ReadOnlyRangeOf<decltype(object.probes()), Probe>; };

static_assert(std::is_aggregate_v<Point>);

static_assert(!std::is_aggregate_v<ComponentInstance>);
static_assert(!std::is_aggregate_v<Wire>);
static_assert(!std::is_aggregate_v<Junction>);
static_assert(!std::is_aggregate_v<NetLabel>);
static_assert(!std::is_aggregate_v<Directive>);
static_assert(!std::is_aggregate_v<Probe>);
static_assert(!std::is_aggregate_v<AnalysisSetup>);
static_assert(!std::is_aggregate_v<Sheet>);
static_assert(!std::is_aggregate_v<Document>);

static_assert(HasReadOnlyId<ComponentInstance>);
static_assert(HasReadOnlyId<Wire>);
static_assert(HasReadOnlyId<Junction>);
static_assert(HasReadOnlyId<NetLabel>);
static_assert(HasReadOnlyId<Directive>);
static_assert(HasReadOnlyId<Probe>);
static_assert(HasReadOnlyId<AnalysisSetup>);
static_assert(HasReadOnlyId<Sheet>);
static_assert(HasReadOnlyId<Document>);

static_assert(!HasPublicIdField<ComponentInstance>);
static_assert(!HasPublicIdField<Wire>);
static_assert(!HasPublicIdField<Junction>);
static_assert(!HasPublicIdField<NetLabel>);
static_assert(!HasPublicIdField<Directive>);
static_assert(!HasPublicIdField<Probe>);
static_assert(!HasPublicIdField<AnalysisSetup>);
static_assert(!HasPublicIdField<Sheet>);
static_assert(!HasPublicIdField<Document>);

static_assert(!HasPublicIdSetter<ComponentInstance>);
static_assert(!HasPublicIdSetter<Wire>);
static_assert(!HasPublicIdSetter<Junction>);
static_assert(!HasPublicIdSetter<NetLabel>);
static_assert(!HasPublicIdSetter<Directive>);
static_assert(!HasPublicIdSetter<Probe>);
static_assert(!HasPublicIdSetter<AnalysisSetup>);
static_assert(!HasPublicIdSetter<Sheet>);
static_assert(!HasPublicIdSetter<Document>);

static_assert(!HasPublicSheetsField<Document>);
static_assert(!HasPublicAnalysesField<Document>);
static_assert(!HasPublicComponentsField<Sheet>);
static_assert(!HasPublicWiresField<Sheet>);
static_assert(!HasPublicJunctionsField<Sheet>);
static_assert(!HasPublicLabelsField<Sheet>);
static_assert(!HasPublicDirectivesField<Sheet>);
static_assert(!HasPublicProbesField<Sheet>);

static_assert(HasReadOnlySheets<Document>);
static_assert(HasReadOnlyAnalyses<Document>);
static_assert(HasReadOnlyComponents<Sheet>);
static_assert(HasReadOnlyWires<Sheet>);
static_assert(HasReadOnlyJunctions<Sheet>);
static_assert(HasReadOnlyLabels<Sheet>);
static_assert(HasReadOnlyDirectives<Sheet>);
static_assert(HasReadOnlyProbes<Sheet>);

static_assert(!std::is_default_constructible_v<ComponentInstance>);
static_assert(!std::is_default_constructible_v<Wire>);
static_assert(!std::is_default_constructible_v<Junction>);
static_assert(!std::is_default_constructible_v<NetLabel>);
static_assert(!std::is_default_constructible_v<Directive>);
static_assert(!std::is_default_constructible_v<Probe>);
static_assert(!std::is_default_constructible_v<AnalysisSetup>);
static_assert(std::is_default_constructible_v<Document>);

Uuid uuid(const std::string &text) {
  return Uuid::parse(text).value();
}

Point point(const Coordinate::rep x, const Coordinate::rep y) {
  return Point{
    .x = Coordinate::from_nanometres(x),
    .y = Coordinate::from_nanometres(y),
  };
}

TEST_CASE("entity factories create complete objects with fresh identities") {
  const auto component = ComponentInstance::create("R1", point(1'000'000, 2'000'000));
  const auto wire = Wire::create(std::array{
    point(1'000'000, 2'000'000),
    point(3'000'000, 2'000'000),
  });
  const auto junction = Junction::create(point(3'000'000, 2'000'000));
  const auto label = NetLabel::create("output", point(3'000'000, 2'000'000));
  const auto directive =
    Directive::create(".include models/diodes.lib", point(-1'000'000, -2'000'000));
  const auto probe = Probe::create(ProbeKind::voltage);
  const auto analysis = AnalysisSetup::create("Startup transient");

  std::unordered_set<Uuid, Uuid::UuidHash> ids;
  const auto check_fresh_id = [&ids](const auto &object) {
    CHECK_FALSE(object.id().is_nil());
    CHECK(ids.insert(object.id()).second);
  };

  check_fresh_id(component);
  check_fresh_id(wire);
  check_fresh_id(junction);
  check_fresh_id(label);
  check_fresh_id(directive);
  check_fresh_id(probe);
  check_fresh_id(analysis);

  CHECK(component.designator() == "R1");
  CHECK(component.position().x == Coordinate::from_millimetres(1));
  CHECK(wire.points()[1].x == Coordinate::from_millimetres(3));
  CHECK(junction.position().x == Coordinate::from_millimetres(3));
  CHECK(label.text() == "output");
  CHECK(directive.text() == ".include models/diodes.lib");
  CHECK(probe.kind() == ProbeKind::voltage);
  CHECK(analysis.name() == "Startup transient");
}

TEST_CASE("a new document owns one editable schematic sheet") {
  const Document document;

  CHECK_FALSE(document.id().is_nil());
  REQUIRE(document.sheets().size() == 1);

  const auto &sheet = document.sheets().front();
  CHECK_FALSE(sheet.id().is_nil());
  CHECK_FALSE(sheet.name().empty());
  CHECK(sheet.components().empty());
  CHECK(sheet.wires().empty());
  CHECK(sheet.junctions().empty());
  CHECK(sheet.labels().empty());
  CHECK(sheet.directives().empty());
  CHECK(sheet.probes().empty());
  CHECK(document.analyses().empty());
}

TEST_CASE("document operations add persistent schematic objects") {
  Document document;
  const auto sheet_id = document.sheets().front().id();

  const auto component_id = uuid("10000000-0000-0000-0000-000000000001");
  const auto wire_id = uuid("20000000-0000-0000-0000-000000000002");
  const auto junction_id = uuid("30000000-0000-0000-0000-000000000003");
  const auto label_id = uuid("40000000-0000-0000-0000-000000000004");
  const auto directive_id = uuid("50000000-0000-0000-0000-000000000005");

  document.add_component(
    sheet_id,
    ComponentInstance(component_id, "R1", point(1'000'000, 2'000'000)));
  document.add_wire(
    sheet_id,
    Wire(
      wire_id,
      std::array{
        point(1'000'000, 2'000'000),
        point(3'000'000, 2'000'000),
      }));
  document.add_junction(sheet_id, Junction(junction_id, point(3'000'000, 2'000'000)));
  document.add_label(sheet_id, NetLabel(label_id, "output", point(3'000'000, 2'000'000)));
  document.add_directive(
    sheet_id,
    Directive(directive_id, ".include models/diodes.lib", point(-1'000'000, -2'000'000)));

  const auto &sheet = document.sheets().front();

  REQUIRE(sheet.components().size() == 1);
  CHECK(sheet.components().front().id() == component_id);
  CHECK(sheet.components().front().designator() == "R1");
  CHECK(sheet.components().front().position().x == Coordinate::from_millimetres(1));
  CHECK(sheet.components().front().position().y == Coordinate::from_millimetres(2));

  REQUIRE(sheet.wires().size() == 1);
  CHECK(sheet.wires().front().id() == wire_id);
  CHECK(sheet.wires().front().points()[0].x == Coordinate::from_millimetres(1));
  CHECK(sheet.wires().front().points()[1].x == Coordinate::from_millimetres(3));

  REQUIRE(sheet.junctions().size() == 1);
  CHECK(sheet.junctions().front().id() == junction_id);
  CHECK(sheet.junctions().front().position().x == Coordinate::from_millimetres(3));

  REQUIRE(sheet.labels().size() == 1);
  CHECK(sheet.labels().front().id() == label_id);
  CHECK(sheet.labels().front().text() == "output");

  REQUIRE(sheet.directives().size() == 1);
  CHECK(sheet.directives().front().id() == directive_id);
  CHECK(sheet.directives().front().text() == ".include models/diodes.lib");
  CHECK(sheet.directives().front().position().x == Coordinate::from_millimetres(-1));
  CHECK(sheet.directives().front().position().y == Coordinate::from_millimetres(-2));
}

TEST_CASE("document operations add analysis setups and schematic probes") {
  Document document;
  const auto sheet_id = document.sheets().front().id();

  const auto analysis_id = uuid("60000000-0000-0000-0000-000000000006");
  const auto voltage_probe_id = uuid("70000000-0000-0000-0000-000000000007");
  const auto current_probe_id = uuid("80000000-0000-0000-0000-000000000008");

  document.add_analysis(AnalysisSetup(analysis_id, "Startup transient"));
  document.add_probe(sheet_id, Probe(voltage_probe_id, ProbeKind::voltage));
  document.add_probe(sheet_id, Probe(current_probe_id, ProbeKind::current));

  REQUIRE(document.analyses().size() == 1);
  CHECK(document.analyses().front().id() == analysis_id);
  CHECK(document.analyses().front().name() == "Startup transient");

  const auto &probes = document.sheets().front().probes();
  REQUIRE(probes.size() == 2);
  CHECK(probes[0].id() == voltage_probe_id);
  CHECK(probes[0].kind() == ProbeKind::voltage);
  CHECK(probes[1].id() == current_probe_id);
  CHECK(probes[1].kind() == ProbeKind::current);
  CHECK(ProbeKind::voltage != ProbeKind::current);
}

TEST_CASE("document operations remove objects without losing their persistent state") {
  Document document;
  const auto sheet_id = document.sheets().front().id();

  const auto component_id = uuid("10000000-0000-0000-0000-000000000011");
  const auto wire_id = uuid("20000000-0000-0000-0000-000000000012");
  const auto junction_id = uuid("30000000-0000-0000-0000-000000000013");
  const auto label_id = uuid("40000000-0000-0000-0000-000000000014");
  const auto directive_id = uuid("50000000-0000-0000-0000-000000000015");
  const auto probe_id = uuid("60000000-0000-0000-0000-000000000016");
  const auto analysis_id = uuid("70000000-0000-0000-0000-000000000017");

  REQUIRE(document.add_component(
    sheet_id,
    ComponentInstance(component_id, "R1", point(1'000'000, 2'000'000))));
  REQUIRE(document.add_wire(
    sheet_id,
    Wire(
      wire_id,
      std::array{
        point(1'000'000, 2'000'000),
        point(3'000'000, 2'000'000),
      })));
  REQUIRE(document.add_junction(sheet_id, Junction(junction_id, point(3'000'000, 2'000'000))));
  REQUIRE(document.add_label(sheet_id, NetLabel(label_id, "output", point(3'000'000, 2'000'000))));
  REQUIRE(document.add_directive(
    sheet_id,
    Directive(directive_id, ".include models/diodes.lib", point(-1'000'000, -2'000'000))));
  REQUIRE(document.add_probe(sheet_id, Probe(probe_id, ProbeKind::current)));
  REQUIRE(document.add_analysis(AnalysisSetup(analysis_id, "Startup transient")));
  REQUIRE(document.rename_component(component_id, "R7"));

  const auto component = document.remove_component(component_id);
  const auto wire = document.remove_wire(wire_id);
  const auto junction = document.remove_junction(junction_id);
  const auto label = document.remove_label(label_id);
  const auto directive = document.remove_directive(directive_id);
  const auto probe = document.remove_probe(probe_id);
  const auto analysis = document.remove_analysis(analysis_id);

  REQUIRE(component.has_value());
  CHECK(component->id() == component_id);
  CHECK(component->designator() == "R7");
  CHECK(component->position().x == Coordinate::from_millimetres(1));

  REQUIRE(wire.has_value());
  CHECK(wire->id() == wire_id);
  CHECK(wire->points()[1].x == Coordinate::from_millimetres(3));

  REQUIRE(junction.has_value());
  CHECK(junction->id() == junction_id);
  CHECK(junction->position().x == Coordinate::from_millimetres(3));

  REQUIRE(label.has_value());
  CHECK(label->id() == label_id);
  CHECK(label->text() == "output");

  REQUIRE(directive.has_value());
  CHECK(directive->id() == directive_id);
  CHECK(directive->text() == ".include models/diodes.lib");

  REQUIRE(probe.has_value());
  CHECK(probe->id() == probe_id);
  CHECK(probe->kind() == ProbeKind::current);

  REQUIRE(analysis.has_value());
  CHECK(analysis->id() == analysis_id);
  CHECK(analysis->name() == "Startup transient");

  const auto &sheet = document.sheets().front();
  CHECK(sheet.components().empty());
  CHECK(sheet.wires().empty());
  CHECK(sheet.junctions().empty());
  CHECK(sheet.labels().empty());
  CHECK(sheet.directives().empty());
  CHECK(sheet.probes().empty());
  CHECK(document.analyses().empty());
}

TEST_CASE("invalid sheet identifiers leave document state unchanged") {
  Document document;
  const auto sheet_id = document.sheets().front().id();
  const auto missing_id = uuid("80000000-0000-0000-0000-000000000018");

  CHECK_FALSE(document.add_component(
    missing_id,
    ComponentInstance::create("R1", point(1'000'000, 2'000'000))));
  CHECK_FALSE(document.add_wire(
    missing_id,
    Wire::create(std::array{
      point(1'000'000, 2'000'000),
      point(3'000'000, 2'000'000),
    })));
  CHECK_FALSE(document.add_junction(missing_id, Junction::create(point(3'000'000, 2'000'000))));
  CHECK_FALSE(
    document.add_label(missing_id, NetLabel::create("output", point(3'000'000, 2'000'000))));
  CHECK_FALSE(document.add_directive(
    missing_id,
    Directive::create(".include models/diodes.lib", point(-1'000'000, -2'000'000))));
  CHECK_FALSE(document.add_probe(missing_id, Probe::create(ProbeKind::voltage)));

  const auto &sheet = document.sheets().front();
  CHECK(sheet.id() == sheet_id);
  CHECK(sheet.components().empty());
  CHECK(sheet.wires().empty());
  CHECK(sheet.junctions().empty());
  CHECK(sheet.labels().empty());
  CHECK(sheet.directives().empty());
  CHECK(sheet.probes().empty());
  CHECK(document.analyses().empty());
}

TEST_CASE("missing object identifiers leave existing state unchanged") {
  Document document;
  const auto sheet_id = document.sheets().front().id();
  const auto missing_id = uuid("80000000-0000-0000-0000-000000000020");

  const auto component = ComponentInstance::create("R1", point(1'000'000, 2'000'000));
  const auto wire = Wire::create(std::array{
    point(1'000'000, 2'000'000),
    point(3'000'000, 2'000'000),
  });
  const auto junction = Junction::create(point(3'000'000, 2'000'000));
  const auto label = NetLabel::create("output", point(3'000'000, 2'000'000));
  const auto directive =
    Directive::create(".include models/diodes.lib", point(-1'000'000, -2'000'000));
  const auto probe = Probe::create(ProbeKind::voltage);
  const auto analysis = AnalysisSetup::create("Startup transient");

  REQUIRE(document.add_component(sheet_id, component));
  REQUIRE(document.add_wire(sheet_id, wire));
  REQUIRE(document.add_junction(sheet_id, junction));
  REQUIRE(document.add_label(sheet_id, label));
  REQUIRE(document.add_directive(sheet_id, directive));
  REQUIRE(document.add_probe(sheet_id, probe));
  REQUIRE(document.add_analysis(analysis));

  CHECK_FALSE(document.remove_component(missing_id).has_value());
  CHECK_FALSE(document.remove_wire(missing_id).has_value());
  CHECK_FALSE(document.remove_junction(missing_id).has_value());
  CHECK_FALSE(document.remove_label(missing_id).has_value());
  CHECK_FALSE(document.remove_directive(missing_id).has_value());
  CHECK_FALSE(document.remove_probe(missing_id).has_value());
  CHECK_FALSE(document.remove_analysis(missing_id).has_value());
  CHECK_FALSE(document.rename_component(missing_id, "R7"));

  const auto &sheet = document.sheets().front();
  REQUIRE(sheet.components().size() == 1);
  CHECK(sheet.components().front().id() == component.id());
  CHECK(sheet.components().front().designator() == "R1");
  REQUIRE(sheet.wires().size() == 1);
  CHECK(sheet.wires().front().id() == wire.id());
  REQUIRE(sheet.junctions().size() == 1);
  CHECK(sheet.junctions().front().id() == junction.id());
  REQUIRE(sheet.labels().size() == 1);
  CHECK(sheet.labels().front().id() == label.id());
  REQUIRE(sheet.directives().size() == 1);
  CHECK(sheet.directives().front().id() == directive.id());
  REQUIRE(sheet.probes().size() == 1);
  CHECK(sheet.probes().front().id() == probe.id());
  REQUIRE(document.analyses().size() == 1);
  CHECK(document.analyses().front().id() == analysis.id());
}

TEST_CASE(
  "component lookup rejects UUIDs belonging to other object types", "[lookup][regression]") {
  for (const bool with_component : {false, true}) {
    CAPTURE(with_component);
    Document document;
    const auto sheet_id = document.sheets().front().id();
    const auto component = ComponentInstance::create("R1", point(1, 2));
    if (with_component) {
      REQUIRE(document.add_component(sheet_id, component));
    }

    const auto wire = Wire::create(std::array{point(0, 0), point(10, 0)});
    const auto junction = Junction::create(point(10, 0));
    const auto label = NetLabel::create("output", point(10, 0));
    const auto directive = Directive::create(".op", point(0, 10));
    const auto probe = Probe::create(ProbeKind::voltage);
    const auto analysis = AnalysisSetup::create("Operating point");
    REQUIRE(document.add_wire(sheet_id, wire));
    REQUIRE(document.add_junction(sheet_id, junction));
    REQUIRE(document.add_label(sheet_id, label));
    REQUIRE(document.add_directive(sheet_id, directive));
    REQUIRE(document.add_probe(sheet_id, probe));
    REQUIRE(document.add_analysis(analysis));

    const Document &view = document;
    CHECK(view.lookup(document.id()) == nullptr);
    CHECK(view.lookup(sheet_id) == nullptr);
    CHECK(view.lookup(wire.id()) == nullptr);
    CHECK(view.lookup(junction.id()) == nullptr);
    CHECK(view.lookup(label.id()) == nullptr);
    CHECK(view.lookup(directive.id()) == nullptr);
    CHECK(view.lookup(probe.id()) == nullptr);
    CHECK(view.lookup(analysis.id()) == nullptr);
    CHECK(view.lookup(uuid("80000000-0000-0000-0000-000000000099")) == nullptr);

    if (with_component) {
      const auto *found = view.lookup(component.id());
      REQUIRE(found != nullptr);
      CHECK(found->id() == component.id());
      CHECK(found->designator() == component.designator());
      CHECK(found->position() == component.position());
    } else {
      CHECK(view.lookup(component.id()) == nullptr);
    }
  }
}

TEST_CASE("persistent UUIDs are unique across the entire document") {
  Document document;
  const auto sheet_id = document.sheets().front().id();
  const auto shared_id = uuid("90000000-0000-0000-0000-000000000019");

  CHECK_FALSE(document.add_component(
    sheet_id,
    ComponentInstance(document.id(), "R1", point(1'000'000, 2'000'000))));
  CHECK_FALSE(document.add_component(
    sheet_id,
    ComponentInstance(sheet_id, "R1", point(1'000'000, 2'000'000))));

  REQUIRE(document.add_component(
    sheet_id,
    ComponentInstance(shared_id, "R1", point(1'000'000, 2'000'000))));
  CHECK_FALSE(document.add_component(
    sheet_id,
    ComponentInstance(shared_id, "R2", point(3'000'000, 2'000'000))));
  CHECK_FALSE(document.add_wire(
    sheet_id,
    Wire(
      shared_id,
      std::array{
        point(1'000'000, 2'000'000),
        point(3'000'000, 2'000'000),
      })));
  CHECK_FALSE(document.add_junction(sheet_id, Junction(shared_id, point(3'000'000, 2'000'000))));
  CHECK_FALSE(
    document.add_label(sheet_id, NetLabel(shared_id, "output", point(3'000'000, 2'000'000))));
  CHECK_FALSE(document.add_directive(
    sheet_id,
    Directive(shared_id, ".include models/diodes.lib", point(-1'000'000, -2'000'000))));
  CHECK_FALSE(document.add_probe(sheet_id, Probe(shared_id, ProbeKind::voltage)));
  CHECK_FALSE(document.add_analysis(AnalysisSetup(shared_id, "Startup transient")));

  const auto &sheet = document.sheets().front();
  REQUIRE(sheet.components().size() == 1);
  CHECK(sheet.components().front().id() == shared_id);
  CHECK(sheet.wires().empty());
  CHECK(sheet.junctions().empty());
  CHECK(sheet.labels().empty());
  CHECK(sheet.directives().empty());
  CHECK(sheet.probes().empty());
  CHECK(document.analyses().empty());
}

TEST_CASE("meaningful document operations preserve persistent identity") {
  Document document;
  const auto sheet_id = document.sheets().front().id();
  const auto component_id = uuid("90000000-0000-0000-0000-000000000009");

  document.add_component(sheet_id, ComponentInstance(component_id, "R1", point(0, 0)));
  document.rename_component(component_id, "R7");

  const auto &component = document.sheets().front().components().front();
  CHECK(component.designator() == "R7");
  CHECK(component.id() == component_id);
}

} // namespace
