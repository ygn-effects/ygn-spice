#include <limits>
#include <stdexcept>

#include "units.hpp"

namespace ygn::spice::core {
Coordinate::Coordinate(rep nano) : rep_(nano) {
}

Coordinate Coordinate::from_millimetres(const rep r) {
  const rep mult = 1000000;

  if (is_overflow(r, mult)) {
    throw std::overflow_error("Overflow error.");
  }

  rep nano = r * mult;
  Coordinate rep(nano);

  return rep;
}

Coordinate Coordinate::from_micrometres(const rep r) {
  const rep mult = 1000;

  if (is_overflow(r, mult)) {
    throw std::overflow_error("Overflow error.");
  }

  rep micro = r * mult;
  Coordinate rep(micro);

  return rep;
}

Coordinate Coordinate::from_nanometres(const rep r) {
  Coordinate rep(r);

  return rep;
}

Coordinate Coordinate::from_mils(const rep r) {
  const rep mult = 25400;

  if (is_overflow(r, mult)) {
    throw std::overflow_error("Overflow error.");
  }

  rep mils = r * mult;
  Coordinate rep(mils);

  return rep;
}

Coordinate::rep Coordinate::nanometres() const {
  return rep_;
}

bool Coordinate::is_overflow(const rep &a, const rep &b) {
  if (b != 0 && a > std::numeric_limits<rep>::max() / b) {
    return true;
  }

  if (b != 0 && a < std::numeric_limits<rep>::min() / b) {
    return true;
  }

  return false;
}

bool operator==(const Coordinate &self, const Coordinate &other) {
  return self.rep_ == other.rep_;
}

bool operator<(const Coordinate &self, const Coordinate &other) {
  return self.rep_ < other.rep_;
}

bool operator>(const Coordinate &self, const Coordinate &other) {
  return operator<(other, self);
}

bool operator<=(const Coordinate &self, const Coordinate &other) {
  return !(operator>(self, other));
}

bool operator>=(const Coordinate &self, const Coordinate &other) {
  return !(operator<(self, other));
}
} // namespace ygn::spice::core
