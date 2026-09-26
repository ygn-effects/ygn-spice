#include <algorithm>
#include <queue>

#include "connectivity.hpp"

namespace ygn::spice::core {
ConnectionRef ConnectionRef::wire(const Uuid &id) {
  return ConnectionRef(ConnectionKind::wire, id);
}

ConnectionRef ConnectionRef::junction(const Uuid &id) {
  return ConnectionRef(ConnectionKind::junction, id);
}

ConnectionRef ConnectionRef::label(const Uuid &id) {
  return ConnectionRef(ConnectionKind::label, id);
}

ConnectionRef ConnectionRef::pin(const Uuid &component_id, const Uuid &pin_id) {
  return ConnectionRef(ConnectionKind::pin, pin_id, component_id);
}

ConnectionKind ConnectionRef::kind() const {
  return kind_;
}

bool operator!=(const ConnectionRef &self, const ConnectionRef &other) {
  return !(operator==(self, other));
}

ConnectionRef::ConnectionRef(ConnectionKind kind, Uuid p_id, std::optional<Uuid> s_id)
    : kind_(kind), primary_id_(p_id), secondary_id_(s_id) {
}

const std::vector<ConnectionRef> &Net::connections() const {
  return connections_;
}

bool Net::contains_ref(const ConnectionRef &ref) const {
  return std::ranges::binary_search(connections_, ref);
}

void Net::insert_ref(ConnectionRef ref) {
  connections_.push_back(std::move(ref));
}

void Net::sort_refs() {
  std::sort(connections_.begin(), connections_.end());
}

bool Connectivity::are_connected(const ConnectionRef &ref1, const ConnectionRef &ref2) const {
  for (const auto &n : nets_) {
    if (n.contains_ref(ref1) && n.contains_ref(ref2)) {
      return true;
    }
  }

  return false;
}

const std::vector<Net> &Connectivity::nets() const {
  return nets_;
}

void Connectivity::insert_net(Net net) {
  nets_.push_back(std::move(net));
}

void Connectivity::sort_nets() {
  std::sort(nets_.begin(), nets_.end());
}

Connectivity analyze_connectivity(const Sheet &sheet, std::span<const ResolvedPin> pins) {
  Connectivity con;

  std::vector<Participant> vertices;

  for (const auto &w : sheet.wires()) {
    vertices.emplace_back(ConnectionRef::wire(w.id()), w.points());
  }

  for (const auto &j : sheet.junctions()) {
    vertices.emplace_back(ConnectionRef::junction(j.id()), j.position());
  }

  for (const auto &l : sheet.labels()) {
    vertices.emplace_back(ConnectionRef::label(l.id()), LabelData(l.position(), l.text()));
  }

  for (const auto &p : pins) {
    vertices.emplace_back(ConnectionRef::pin(p.component_id, p.pin_id), p.position);
  }

  std::vector<std::vector<std::size_t>> adjacency(vertices.size());

  for (std::size_t i = 0; i < vertices.size(); i++) {
    for (std::size_t j = i + 1; j < vertices.size(); j++) {
      if (directly_connected(vertices[i], vertices[j])) {
        adjacency[i].push_back(j);
        adjacency[j].push_back(i);
      }
    }
  }

  std::vector<bool> visited(vertices.size(), false);
  std::queue<std::size_t> queue;

  for (std::size_t k = 0; k < vertices.size(); k++) {
    if (!visited[k]) {
      Net n;

      visited[k] = true;
      queue.push(k);

      while (!queue.empty()) {
        std::size_t s = queue.front();
        n.insert_ref(vertices[s].ref);
        queue.pop();

        for (const auto &a : adjacency[s]) {
          if (!visited[a]) {
            visited[a] = true;
            queue.push(a);
          }
        }
      }

      n.sort_refs();
      con.insert_net(std::move(n));
    }
  }

  con.sort_nets();
  return con;
}

bool directly_connected(const Participant &a, const Participant &b) {
  if (a.ref.kind() == ConnectionKind::wire && b.ref.kind() == ConnectionKind::wire) {
    Segment s_a = std::get<Segment>(a.geometry);
    Segment s_b = std::get<Segment>(b.geometry);

    if (segments_connected(s_a, s_b)) {
      return true;
    }

    for (const auto &p : s_a) {
      if (segment_contains_point(s_b, p)) {
        return true;
      }
    }

    for (const auto &p : s_b) {
      if (segment_contains_point(s_a, p)) {
        return true;
      }
    }
  }

  if (
    (a.ref.kind() == ConnectionKind::wire && b.ref.kind() == ConnectionKind::junction) ||
    (a.ref.kind() == ConnectionKind::junction && b.ref.kind() == ConnectionKind::wire)) {
    if (a.ref.kind() == ConnectionKind::wire) {
      const Segment &s_a = std::get<Segment>(a.geometry);
      const Point &p_b = std::get<Point>(b.geometry);

      return segment_contains_point(s_a, p_b);
    } else {
      const Point &p_a = std::get<Point>(a.geometry);
      const Segment &s_b = std::get<Segment>(b.geometry);

      return segment_contains_point(s_b, p_a);
    }
  }

  if (
    (a.ref.kind() == ConnectionKind::wire && b.ref.kind() == ConnectionKind::label) ||
    (a.ref.kind() == ConnectionKind::label && b.ref.kind() == ConnectionKind::wire)) {
    if (a.ref.kind() == ConnectionKind::wire) {
      const Segment &s_a = std::get<Segment>(a.geometry);
      const LabelData &l_b = std::get<LabelData>(b.geometry);

      return segment_contains_point(s_a, l_b.position);
    } else {
      const LabelData &l_a = std::get<LabelData>(a.geometry);
      const Segment &s_b = std::get<Segment>(b.geometry);

      return segment_contains_point(s_b, l_a.position);
    }
  }

  if (a.ref.kind() == ConnectionKind::label && b.ref.kind() == ConnectionKind::label) {
    const LabelData &l_a = std::get<LabelData>(a.geometry);
    const LabelData &l_b = std::get<LabelData>(b.geometry);

    if (l_a.text.empty() && l_b.text.empty()) {
      return false;
    }

    if (l_a.text == l_b.text) {
      return true;
    }
  }

  if (
    (a.ref.kind() == ConnectionKind::wire && b.ref.kind() == ConnectionKind::pin) ||
    (a.ref.kind() == ConnectionKind::pin && b.ref.kind() == ConnectionKind::wire)) {
    if (a.ref.kind() == ConnectionKind::wire) {
      const Segment &s_a = std::get<Segment>(a.geometry);
      const Point &p_b = std::get<Point>(b.geometry);

      return segment_edges_contains_point(s_a, p_b);
    } else {
      const Point &p_a = std::get<Point>(a.geometry);
      const Segment &s_b = std::get<Segment>(b.geometry);

      return segment_edges_contains_point(s_b, p_a);
    }
  }

  return false;
}

bool segments_connected(const Segment &a, const Segment &b) {
  return a[0] == b[0] || a[1] == b[1] || a[0] == b[1] || a[1] == b[0];
}

bool segment_contains_point(const Segment &s, const Point &p) {
  if (s[0].x == s[1].x && p.x == s[0].x) {
    if (s[0].y > s[1].y) {
      return p.y <= s[0].y && p.y >= s[1].y;
    } else {
      return p.y <= s[1].y && p.y >= s[0].y;
    }
  }

  if (s[0].y == s[1].y && p.y == s[0].y) {
    if (s[0].x > s[1].x) {
      return p.x <= s[0].x && p.x >= s[1].x;
    } else {
      return p.x <= s[1].x && p.x >= s[0].x;
    }
  }

  return false;
}

bool segment_edges_contains_point(const Segment &s, const Point &p) {
  return (s[0].x == p.x && s[0].y == p.y) || (s[1].x == p.x && s[1].y == p.y);
}

} // namespace ygn::spice::core
