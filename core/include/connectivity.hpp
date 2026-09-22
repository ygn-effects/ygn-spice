#pragma once

#include "document.hpp"

#include <span>
#include <variant>
#include <vector>

namespace ygn::spice::core {
class Connectivity;
struct ResolvedPin;

enum class ConnectionKind { wire, junction, label, pin };

class ConnectionRef {
public:
  static ConnectionRef wire(const Uuid &id);
  static ConnectionRef junction(const Uuid &id);
  static ConnectionRef label(const Uuid &id);
  static ConnectionRef pin(const Uuid &component_id, const Uuid &pin_id);

  const ConnectionKind &kind() const;

  bool operator<(const ConnectionRef &other) const;
  friend bool operator==(const ConnectionRef &self, const ConnectionRef &other);
  friend bool operator!=(const ConnectionRef &self, const ConnectionRef &other);

private:
  ConnectionRef(ConnectionKind kind, Uuid p_id, std::optional<Uuid> s_id = std::nullopt);

  ConnectionKind kind_;
  Uuid primary_id_;
  std::optional<Uuid> secondary_id_;
};

struct LabelData {
  Point position;
  std::string text;
};

using ParticipantGeometry = std::variant<Point, Segment, LabelData>;

struct Participant {
  ConnectionRef ref;
  ParticipantGeometry geometry;
};

class Net {
public:
  const std::vector<ConnectionRef> &connections() const;

  bool contains_ref(const ConnectionRef &ref) const;

  bool operator<(const Net &other) const;
  friend bool operator==(const Net &self, const Net &other);

private:
  std::vector<ConnectionRef> connections_;

  void insert_ref(ConnectionRef ref);
  void sort_refs();

  friend Connectivity analyze_connectivity(const Sheet &sheet, std::span<const ResolvedPin> pins);
};

struct ResolvedPin {
  Uuid component_id;
  Uuid pin_id;
  Point position;
};

class Connectivity {
public:
  bool are_connected(const ConnectionRef &ref1, const ConnectionRef &ref2) const;

  const std::vector<Net> &nets() const;

private:
  std::vector<Net> nets_;

  void insert_net(Net net);
  void sort_nets();

  friend Connectivity analyze_connectivity(const Sheet &sheet, std::span<const ResolvedPin> pins);
};

Connectivity analyze_connectivity(const Sheet &sheet, std::span<const ResolvedPin> pins = {});
bool directly_connected(const Participant &a, const Participant &b);
bool segments_connected(const Segment &a, const Segment &b);
bool segment_contains_point(const Segment &s, const Point &p);
bool segment_edges_contains_point(const Segment &s, const Point &p);

} // namespace ygn::spice::core
