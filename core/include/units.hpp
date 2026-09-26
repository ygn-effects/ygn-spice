#pragma once

#include <cstdint>

namespace ygn::spice::core {

class Coordinate {
public:
  using rep = int64_t;

  friend auto operator<=>(const Coordinate &self, const Coordinate &other) = default;

  static Coordinate from_millimetres(const rep r);
  static Coordinate from_micrometres(const rep r);
  static Coordinate from_nanometres(const rep r);

  static Coordinate from_mils(const rep r);

  rep nanometres() const;

private:
  Coordinate(rep nano);

  rep rep_;

  static bool is_overflow(rep a, rep b);
};
} // namespace ygn::spice::core
