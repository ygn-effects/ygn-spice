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

template <typename T>
bool add_unique(std::vector<T> &vector, T object) {
  auto it = find_by_id<std::vector<T>>(vector, object.id());

  if (it != vector.end()) {
    return false;
  }

  vector.push_back(std::move(object));
  return true;
}

template <typename T>
std::optional<T> remove_by_id(std::vector<T> &vector, const Uuid &id) {
  auto it = find_by_id<std::vector<T>>(vector, id);

  if (it == vector.end()) {
    return std::nullopt;
  }

  auto removed = std::move(*it);
  vector.erase(it);

  return removed;
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
  designator_ = std::move(designator);
}

Wire::Wire(Uuid id, Segment points) : id_(std::move(id)), points_(std::move(points)) {
}

Wire Wire::create(Segment points) {
  Wire wire(Uuid::random(), std::move(points));

  return wire;
}

const Uuid &Wire::id() const noexcept {
  return id_;
}

const Segment &Wire::points() const noexcept {
  return points_;
}

Junction::Junction(Uuid id, Point position) : id_(std::move(id)), position_(std::move(position)) {
}

Junction Junction::create(Point position) {
  Junction junction(Uuid::random(), std::move(position));

  return junction;
}

const Uuid &Junction::id() const noexcept {
  return id_;
}

const Point &Junction::position() const noexcept {
  return position_;
}

NetLabel::NetLabel(Uuid id, std::string text, Point position)
    : id_(std::move(id)), text_(std::move(text)), position_(std::move(position)) {
}

NetLabel NetLabel::create(std::string text, Point position) {
  NetLabel label(Uuid::random(), std::move(text), std::move(position));

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
    : id_(std::move(id)), text_(std::move(text)), position_(std::move(position)) {
}

Directive Directive::create(std::string text, Point position) {
  Directive direct(Uuid::random(), std::move(text), std::move(position));

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

Probe::Probe(Uuid id, ProbeKind kind) : id_(std::move(id)), kind_(std::move(kind)) {
}

Probe Probe::create(ProbeKind kind) {
  Probe probe(Uuid::random(), std::move(kind));

  return probe;
}

const Uuid &Probe::id() const noexcept {
  return id_;
}

ProbeKind Probe::kind() const noexcept {
  return kind_;
}

AnalysisSetup::AnalysisSetup(Uuid id, std::string name)
    : id_(std::move(id)), name_(std::move(name)) {
}

AnalysisSetup AnalysisSetup::create(std::string name) {
  AnalysisSetup analysis(Uuid::random(), std::move(name));

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
  for (const auto &sheet : sheets_) {
    if (contains_id(sheet.components_, id)) {
      const auto it = find_by_id(sheet.components_, id);
      return &(*it);
    }
  }

  return nullptr;
}

template <typename T>
bool Document::add(std::vector<T> Sheet::*member, const Uuid &sheet_id, T object) {
  if (contains_uuid(object.id())) {
    return false;
  }

  auto it = find_by_id(sheets_, sheet_id);

  if (it == sheets_.end()) {
    return false;
  }

  return add_unique((*it).*member, std::move(object));
}

template <typename T>
std::optional<T> Document::remove(std::vector<T> Sheet::*member, const Uuid &id) {
  for (auto &sheet : sheets_) {
    if (auto removed = remove_by_id(sheet.*member, id)) {
      return removed;
    }
  }

  return std::nullopt;
}

bool Document::add_component(const Uuid &sheet_id, ComponentInstance component) {
  return add(&Sheet::components_, sheet_id, std::move(component));
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
  return remove(&Sheet::components_, id);
}

bool Document::add_wire(const Uuid &sheet_id, Wire wire) {
  return add(&Sheet::wires_, sheet_id, std::move(wire));
}

std::optional<Wire> Document::remove_wire(const Uuid &id) {
  return remove(&Sheet::wires_, id);
}

bool Document::add_junction(const Uuid &sheet_id, Junction junction) {
  return add(&Sheet::junctions_, sheet_id, std::move(junction));
}

std::optional<Junction> Document::remove_junction(const Uuid &id) {
  return remove(&Sheet::junctions_, id);
}

bool Document::add_label(const Uuid &sheet_id, NetLabel label) {
  return add(&Sheet::labels_, sheet_id, std::move(label));
}

std::optional<NetLabel> Document::remove_label(const Uuid &id) {
  return remove(&Sheet::labels_, id);
}

bool Document::add_directive(const Uuid &sheet_id, Directive directive) {
  return add(&Sheet::directives_, sheet_id, std::move(directive));
}

std::optional<Directive> Document::remove_directive(const Uuid &id) {
  return remove(&Sheet::directives_, id);
}

bool Document::add_probe(const Uuid &sheet_id, Probe probe) {
  return add(&Sheet::probes_, sheet_id, std::move(probe));
}

std::optional<Probe> Document::remove_probe(const Uuid &id) {
  return remove(&Sheet::probes_, id);
}

bool Document::add_analysis(AnalysisSetup analysis) {
  if (contains_uuid(analysis.id())) {
    return false;
  }

  return add_unique(analyses_, analysis);
}

std::optional<AnalysisSetup> Document::remove_analysis(const Uuid &id) {
  return remove_by_id(analyses_, id);
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

bool Sheet::contains_uuid(const Uuid &id) const {
  return id_ == id || contains_id(components_, id) || contains_id(wires_, id) ||
    contains_id(junctions_, id) || contains_id(labels_, id) || contains_id(directives_, id) ||
    contains_id(probes_, id);
}

bool Sheet::rename_component(const Uuid &id, std::string designator) {
  auto it = find_by_id<std::vector<ComponentInstance>>(components_, id);

  if (it == components_.end()) {
    return false;
  }

  it->set_designator(designator);
  return true;
}

} // namespace ygn::spice::core
