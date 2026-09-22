#pragma once

#include <cstdint>

namespace ygn::spice::core {

class Coordinate {
public:
  typedef int64_t rep;

  friend bool operator==(const Coordinate &self, const Coordinate &other);
  friend bool operator<(const Coordinate &self, const Coordinate &other);
  friend bool operator>(const Coordinate &self, const Coordinate &other);
  friend bool operator<=(const Coordinate &self, const Coordinate &other);
  friend bool operator>=(const Coordinate &self, const Coordinate &other);

  static Coordinate from_millimetres(const rep r);
  static Coordinate from_micrometres(const rep r);
  static Coordinate from_nanometres(const rep r);

  static Coordinate from_mils(const rep r);

  rep nanometres() const;

private:
  Coordinate(rep nano);

  rep rep_;

  static bool is_overflow(const rep &a, const rep &b);
};
} // namespace ygn::spice::core
