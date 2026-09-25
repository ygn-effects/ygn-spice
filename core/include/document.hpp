#pragma once

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <vector>

#include "units.hpp"
#include "uuid.hpp"

namespace ygn::spice::core {
struct Point {
  Coordinate x;
  Coordinate y;

  friend auto operator<=>(const Point &self, const Point &other) = default;
};

using Segment = std::array<Point, 2>;

class ComponentInstance {
public:
  ComponentInstance(Uuid id, std::string designator, Point position);

  static ComponentInstance create(std::string designator, Point position);

  const Uuid &id() const noexcept;
  const std::string &designator() const noexcept;
  const Point &position() const noexcept;

private:
  Uuid id_;
  std::string designator_;
  Point position_;

  void set_designator(std::string designator);

  friend class Sheet;
};

class Wire {
public:
  Wire(Uuid id, Segment points);

  static Wire create(Segment points);

  const Uuid &id() const noexcept;
  const Segment &points() const noexcept;

private:
  Uuid id_;
  Segment points_;

  friend class Sheet;
};

class Junction {
public:
  Junction(Uuid id, Point position);

  static Junction create(Point position);

  const Uuid &id() const noexcept;
  const Point &position() const noexcept;

private:
  Uuid id_;
  Point position_;

  friend class Sheet;
};

class NetLabel {
public:
  NetLabel(Uuid id, std::string text, Point position);

  static NetLabel create(std::string text, Point position);

  const Uuid &id() const noexcept;
  const std::string &text() const noexcept;
  const Point &position() const noexcept;

private:
  Uuid id_;
  std::string text_;
  Point position_;

  friend class Sheet;
};

struct Directive {
public:
  Directive(Uuid id, std::string text, Point position);

  static Directive create(std::string text, Point position);

  const Uuid &id() const noexcept;
  const std::string &text() const noexcept;
  const Point &position() const noexcept;

private:
  Uuid id_;
  std::string text_;
  Point position_;

  friend class Sheet;
};

enum class ProbeKind { current, voltage };

class Probe {
public:
  Probe(Uuid id, ProbeKind kind);

  static Probe create(ProbeKind kind);

  const Uuid &id() const noexcept;
  const ProbeKind &kind() const noexcept;

private:
  Uuid id_;
  ProbeKind kind_;

  friend class Sheet;
};

class AnalysisSetup {
public:
  AnalysisSetup(Uuid id, std::string name);

  static AnalysisSetup create(std::string name);

  const Uuid &id() const noexcept;
  const std::string &name() const noexcept;

private:
  Uuid id_;
  std::string name_;

  friend class Document;
};

class Sheet {
public:
  Sheet();

  const Uuid &id() const noexcept;
  const std::string &name() const noexcept;

  const std::vector<ComponentInstance> &components() const noexcept;
  const std::vector<Wire> &wires() const noexcept;
  const std::vector<Junction> &junctions() const noexcept;
  const std::vector<NetLabel> &labels() const noexcept;
  const std::vector<Directive> &directives() const noexcept;
  const std::vector<Probe> &probes() const noexcept;

private:
  Uuid id_ = Uuid::random();
  std::string name_ = "New sheet";

  std::vector<ComponentInstance> components_;
  std::vector<Wire> wires_;
  std::vector<Junction> junctions_;
  std::vector<NetLabel> labels_;
  std::vector<Directive> directives_;
  std::vector<Probe> probes_;

  bool contains_uuid(const Uuid &id) const;

  bool rename_component(const Uuid &id, std::string designator);

  friend class Document;
};

class Document {
public:
  Document();

  const Uuid &id() const noexcept;

  const std::vector<Sheet> &sheets() const noexcept;
  const std::vector<AnalysisSetup> &analyses() const noexcept;

  const ComponentInstance *lookup(const Uuid &id) const;

  bool add_component(const Uuid &sheet_id, ComponentInstance component);
  bool rename_component(const Uuid &id, std::string designator);
  std::optional<ComponentInstance> remove_component(const Uuid &id);

  bool add_wire(const Uuid &sheet_id, Wire wire);
  std::optional<Wire> remove_wire(const Uuid &id);
  bool add_junction(const Uuid &sheet_id, Junction label);
  std::optional<Junction> remove_junction(const Uuid &id);
  bool add_label(const Uuid &sheet_id, NetLabel label);
  std::optional<NetLabel> remove_label(const Uuid &id);
  bool add_directive(const Uuid &sheet_id, Directive directive);
  std::optional<Directive> remove_directive(const Uuid &id);
  bool add_probe(const Uuid &sheet_id, Probe probe);
  std::optional<Probe> remove_probe(const Uuid &id);

  bool add_analysis(AnalysisSetup analysis);
  std::optional<AnalysisSetup> remove_analysis(const Uuid &id);

private:
  Uuid id_ = Uuid::random();

  std::vector<Sheet> sheets_;
  std::vector<AnalysisSetup> analyses_;

  bool contains_uuid(const Uuid &id) const;

  template <typename T>
  bool add(std::vector<T> Sheet::*member, const Uuid &sheet_id, T object);

  template <typename T>
  std::optional<T> remove(std::vector<T> Sheet::*member, const Uuid &id);
};

} // namespace ygn::spice::core
