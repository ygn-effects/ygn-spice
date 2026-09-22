#include <cstring>
#include <functional>

#include "uuid.hpp"

namespace ygn::spice::core {
std::size_t Uuid::UuidHash::operator()(const Uuid &uuid) const {
  size_t r = 0;

  for (size_t i = 0; i < sizeof(uuid.uuid_); i++) {
    r ^= ((static_cast<size_t>(uuid.uuid_[i])) << ((i % sizeof(size_t)) * 8));
  }

  return r;
}

std::optional<Uuid> Uuid::parse(const std::string &str) {
  Uuid uuid;

  if ((uuid_parse(str.c_str(), uuid.uuid_) == 0)) {
    return uuid;
  }

  return {};
}

Uuid Uuid::random() {
  uuid_t uu;
  uuid_generate_random(uu);

  Uuid uuid;
  std::memcpy(uuid.uuid_, uu, sizeof(uu));

  return uuid;
}

Uuid Uuid::nil() {
  Uuid uuid;

  return uuid;
}

std::string Uuid::to_string() const {
  char str[40];
  uuid_unparse(uuid_, str);

  return std::string(str);
}

bool Uuid::is_nil() const {
  return uuid_is_null(uuid_);
}

Uuid::Uuid() {
  static_assert(sizeof(uuid_) == 16, "UUID size must be 16.");
  static_assert(sizeof(uuid_[0]) == 1, "UUID element size must be 1");

  std::memset(uuid_, 0, sizeof(uuid_));
}

bool Uuid::operator<(const Uuid &other) const {
  return std::memcmp(this->uuid_, other.uuid_, sizeof(this->uuid_)) < 0;
}

bool operator==(const Uuid &self, const Uuid &other) {
  return std::memcmp(self.uuid_, other.uuid_, sizeof(uuid_t)) == 0;
}
} // namespace ygn::spice::core
