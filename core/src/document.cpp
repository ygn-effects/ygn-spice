#include "document.hpp"

namespace ygn::spice::core {

namespace {
template <typename Collection>
auto find_by_id(Collection &objects, const Uuid &id) {
  return std::ranges::find(objects, id, [](const auto &object) {
    return object.id();
  });
}

template <typename Collection>
bool contains_id(const Collection &objects, const Uuid &id) {
  return find_by_id(objects, id) != objects.end();
}
} // namespace

ComponentInstance::ComponentInstance(Uuid id, std::string designator, Point position)
    : id_(id), designator_(designator), position_(position) {
}

ComponentInstance ComponentInstance::create(std::string designator, Point position) {
  ComponentInstance comp(Uuid::random(), designator, position);

  return comp;
}

const Uuid &ComponentInstance::id() const noexcept {
  return id_;
}

const std::string &ComponentInstance::designator() const noexcept {
  return designator_;
}

const Point &ComponentInstance::position() const noexcept {
  return position_;
}

void ComponentInstance::set_designator(std::string designator) {
  designator_ = designator;
}

Wire::Wire(Uuid id, Segment points) : id_(id), points_(points) {
}

Wire Wire::create(Segment points) {
  Wire wire(Uuid::random(), points);

  return wire;
}

const Uuid &Wire::id() const noexcept {
  return id_;
}

const Segment &Wire::points() const noexcept {
  return points_;
}

Junction::Junction(Uuid id, Point position) : id_(id), position_(position) {
}

Junction Junction::create(Point position) {
  Junction junction(Uuid::random(), position);

  return junction;
}

const Uuid &Junction::id() const noexcept {
  return id_;
}

const Point &Junction::position() const noexcept {
  return position_;
}

NetLabel::NetLabel(Uuid id, std::string text, Point position)
    : id_(id), text_(text), position_(position) {
}

NetLabel NetLabel::create(std::string text, Point position) {
  NetLabel label(Uuid::random(), text, position);

  return label;
}

const Uuid &NetLabel::id() const noexcept {
  return id_;
}

const std::string &NetLabel::text() const noexcept {
  return text_;
}

const Point &NetLabel::position() const noexcept {
  return position_;
}

Directive::Directive(Uuid id, std::string text, Point position)
    : id_(id), text_(text), position_(position) {
}

Directive Directive::create(std::string text, Point position) {
  Directive direct(Uuid::random(), text, position);

  return direct;
}

const Uuid &Directive::id() const noexcept {
  return id_;
}

const std::string &Directive::text() const noexcept {
  return text_;
}

const Point &Directive::position() const noexcept {
  return position_;
}

Probe::Probe(Uuid id, ProbeKind kind) : id_(id), kind_(kind) {
}

Probe Probe::create(ProbeKind kind) {
  Probe probe(Uuid::random(), kind);

  return probe;
}

const Uuid &Probe::id() const noexcept {
  return id_;
}

const ProbeKind &Probe::kind() const noexcept {
  return kind_;
}

AnalysisSetup::AnalysisSetup(Uuid id, std::string name) : id_(id), name_(name) {
}

AnalysisSetup AnalysisSetup::create(std::string name) {
  AnalysisSetup analysis(Uuid::random(), name);

  return analysis;
}

const Uuid &AnalysisSetup::id() const noexcept {
  return id_;
}

const std::string &AnalysisSetup::name() const noexcept {
  return name_;
}

Document::Document() {
  sheets_.push_back(Sheet{});
}

const Uuid &Document::id() const noexcept {
  return id_;
}

const std::vector<Sheet> &Document::sheets() const noexcept {
  return sheets_;
}

const std::vector<AnalysisSetup> &Document::analyses() const noexcept {
  return analyses_;
}

const ComponentInstance *Document::lookup(const Uuid &id) const {
  for (const auto &s : sheets_) {
    if (contains_id(s.components_, id)) {
      const auto it = find_by_id(s.components_, id);
      return &(*it);
    }
  }

  return nullptr;
}

bool Document::add_component(const Uuid &sheet_id, ComponentInstance component) {
  if (contains_uuid(component.id())) {
    return false;
  }

  auto it = find_by_id<std::vector<Sheet>>(sheets_, sheet_id);

  if (it == sheets_.end()) {
    return false;
  }

  return it->add_component(std::move(component));
}

bool Document::rename_component(const Uuid &id, std::string designator) {
  for (auto &s : sheets_) {
    if (s.rename_component(id, designator)) {
      return true;
    }
  }

  return false;
}

std::optional<ComponentInstance> Document::remove_component(const Uuid &id) {
  for (auto &s : sheets_) {
    auto c = s.remove_component(id);

    if (c != std::nullopt) {
      return c;
    }
  }

  return std::nullopt;
}

bool Document::add_wire(const Uuid &sheet_id, Wire wire) {
  if (contains_uuid(wire.id())) {
    return false;
  }

  auto it = find_by_id<std::vector<Sheet>>(sheets_, sheet_id);

  if (it == sheets_.end()) {
    return false;
  }

  return it->add_wire(std::move(wire));
}

std::optional<Wire> Document::remove_wire(const Uuid &id) {
  for (auto &s : sheets_) {
    auto c = s.remove_wire(id);

    if (c != std::nullopt) {
      return c;
    }
  }

  return std::nullopt;
}

bool Document::add_junction(const Uuid &sheet_id, Junction junction) {
  if (contains_uuid(junction.id())) {
    return false;
  }

  auto it = find_by_id<std::vector<Sheet>>(sheets_, sheet_id);

  if (it == sheets_.end()) {
    return false;
  }

  return it->add_junction(std::move(junction));
}

std::optional<Junction> Document::remove_junction(const Uuid &id) {
  for (auto &s : sheets_) {
    auto c = s.remove_junction(id);

    if (c != std::nullopt) {
      return c;
    }
  }

  return std::nullopt;
}

bool Document::add_label(const Uuid &sheet_id, NetLabel label) {
  if (contains_uuid(label.id())) {
    return false;
  }

  auto it = find_by_id<std::vector<Sheet>>(sheets_, sheet_id);

  if (it == sheets_.end()) {
    return false;
  }

  return it->add_label(std::move(label));
}

std::optional<NetLabel> Document::remove_label(const Uuid &id) {
  for (auto &s : sheets_) {
    auto c = s.remove_label(id);

    if (c != std::nullopt) {
      return c;
    }
  }

  return std::nullopt;
}

bool Document::add_directive(const Uuid &sheet_id, Directive directive) {
  if (contains_uuid(directive.id())) {
    return false;
  }

  auto it = find_by_id<std::vector<Sheet>>(sheets_, sheet_id);

  if (it == sheets_.end()) {
    return false;
  }

  return it->add_directive(std::move(directive));
}

std::optional<Directive> Document::remove_directive(const Uuid &id) {
  for (auto &s : sheets_) {
    auto c = s.remove_directive(id);

    if (c != std::nullopt) {
      return c;
    }
  }

  return std::nullopt;
}

bool Document::add_probe(const Uuid &sheet_id, Probe probe) {
  if (contains_uuid(probe.id())) {
    return false;
  }

  auto it = find_by_id<std::vector<Sheet>>(sheets_, sheet_id);

  if (it == sheets_.end()) {
    return false;
  }

  return it->add_probe(std::move(probe));
}

std::optional<Probe> Document::remove_probe(const Uuid &id) {
  for (auto &s : sheets_) {
    auto c = s.remove_probe(id);

    if (c != std::nullopt) {
      return c;
    }
  }

  return std::nullopt;
}

bool Document::add_analysis(AnalysisSetup analysis) {
  if (contains_uuid(analysis.id())) {
    return false;
  }

  auto it = find_by_id<std::vector<AnalysisSetup>>(analyses_, analysis.id());

  if (it != analyses_.end()) {
    return false;
  }

  analyses_.push_back(analysis);
  return true;
}

std::optional<AnalysisSetup> Document::remove_analysis(const Uuid &id) {
  auto it = find_by_id<std::vector<AnalysisSetup>>(analyses_, id);

  if (it == analyses_.end()) {
    return std::nullopt;
  }

  auto removed = std::move(*it);
  analyses_.erase(it);

  return removed;
}

bool Document::contains_uuid(const Uuid &id) const {
  if (id_ == id || contains_id(analyses_, id)) {
    return true;
  }

  return std::ranges::any_of(sheets_, [&id](const Sheet &sheet) {
    return sheet.contains_uuid(id);
  });
}

Sheet::Sheet() {
}

const Uuid &Sheet::id() const noexcept {
  return id_;
}

const std::string &Sheet::name() const noexcept {
  return name_;
}

const std::vector<ComponentInstance> &Sheet::components() const noexcept {
  return components_;
}

const std::vector<Wire> &Sheet::wires() const noexcept {
  return wires_;
}

const std::vector<Junction> &Sheet::junctions() const noexcept {
  return junctions_;
}

const std::vector<NetLabel> &Sheet::labels() const noexcept {
  return labels_;
}

const std::vector<Directive> &Sheet::directives() const noexcept {
  return directives_;
}

const std::vector<Probe> &Sheet::probes() const noexcept {
  return probes_;
}

bool Sheet::add_component(ComponentInstance component) {
  auto it = find_by_id<std::vector<ComponentInstance>>(components_, component.id());

  if (it != components_.end()) {
    return false;
  }

  components_.push_back(component);
  return true;
}

std::optional<ComponentInstance> Sheet::remove_component(const Uuid &id) {
  auto it = find_by_id<std::vector<ComponentInstance>>(components_, id);

  if (it == components_.end()) {
    return std::nullopt;
  }

  auto removed = std::move(*it);
  components_.erase(it);

  return removed;
}

bool Sheet::rename_component(const Uuid &id, std::string designator) {
  auto it = find_by_id<std::vector<ComponentInstance>>(components_, id);

  if (it == components_.end()) {
    return false;
  }

  it->set_designator(designator);
  return true;
}

bool Sheet::add_wire(Wire wire) {
  auto it = find_by_id<std::vector<Wire>>(wires_, wire.id());

  if (it != wires_.end()) {
    return false;
  }

  wires_.push_back(wire);
  return true;
}

std::optional<Wire> Sheet::remove_wire(const Uuid &id) {
  auto it = find_by_id<std::vector<Wire>>(wires_, id);

  if (it == wires_.end()) {
    return std::nullopt;
  }

  auto removed = std::move(*it);
  wires_.erase(it);

  return removed;
}

bool Sheet::add_junction(Junction junction) {
  auto it = find_by_id<std::vector<Junction>>(junctions_, junction.id());

  if (it != junctions_.end()) {
    return false;
  }

  junctions_.push_back(junction);
  return true;
}

std::optional<Junction> Sheet::remove_junction(const Uuid &id) {
  auto it = find_by_id<std::vector<Junction>>(junctions_, id);

  if (it == junctions_.end()) {
    return std::nullopt;
  }

  auto removed = std::move(*it);
  junctions_.erase(it);

  return removed;
}

bool Sheet::add_label(NetLabel label) {
  auto it = find_by_id<std::vector<NetLabel>>(labels_, label.id());

  if (it != labels_.end()) {
    return false;
  }

  labels_.push_back(label);
  return true;
}

std::optional<NetLabel> Sheet::remove_label(const Uuid &id) {
  auto it = find_by_id<std::vector<NetLabel>>(labels_, id);

  if (it == labels_.end()) {
    return std::nullopt;
  }

  auto removed = std::move(*it);
  labels_.erase(it);

  return removed;
}

bool Sheet::add_directive(Directive directive) {
  auto it = find_by_id<std::vector<Directive>>(directives_, directive.id());

  if (it != directives_.end()) {
    return false;
  }

  directives_.push_back(directive);
  return true;
}

std::optional<Directive> Sheet::remove_directive(const Uuid &id) {
  auto it = find_by_id<std::vector<Directive>>(directives_, id);

  if (it == directives_.end()) {
    return std::nullopt;
  }

  auto removed = std::move(*it);
  directives_.erase(it);

  return removed;
}

bool Sheet::add_probe(Probe probe) {
  auto it = find_by_id<std::vector<Probe>>(probes_, probe.id());

  if (it != probes_.end()) {
    return false;
  }

  probes_.push_back(probe);
  return true;
}

std::optional<Probe> Sheet::remove_probe(const Uuid &id) {
  auto it = find_by_id<std::vector<Probe>>(probes_, id);

  if (it == probes_.end()) {
    return std::nullopt;
  }

  auto removed = std::move(*it);
  probes_.erase(it);

  return removed;
}

bool Sheet::contains_uuid(const Uuid &id) const {
  return id_ == id || contains_id(components_, id) || contains_id(wires_, id) ||
    contains_id(junctions_, id) || contains_id(labels_, id) || contains_id(directives_, id) ||
    contains_id(probes_, id);
}
} // namespace ygn::spice::core
