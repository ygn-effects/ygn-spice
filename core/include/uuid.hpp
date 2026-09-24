#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <uuid/uuid.h>

namespace ygn::spice::core {
class Uuid {
public:
  struct UuidHash {
    std::size_t operator()(const Uuid &uuid) const;
  };

  static std::optional<Uuid> parse(const std::string &str);
  static Uuid random();
  static Uuid nil();

  std::string to_string() const;
  bool is_nil() const;

  friend auto operator<=>(const Uuid &self, const Uuid &other) = default;

private:
  Uuid();

  uuid_t uuid_;
};
} // namespace ygn::spice::core
