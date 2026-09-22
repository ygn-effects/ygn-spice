#pragma once

#include "document.hpp"
#include "uuid.hpp"

#include <optional>
#include <string>

namespace ygn::spice::core {
class Command {
public:
  virtual bool execute(Document &doc) = 0;
  virtual bool undo(Document &doc) = 0;

  virtual ~Command() = default;
};

class RenameComponentCommand : public Command {
public:
  RenameComponentCommand(const Uuid id, const std::string new_name);

  bool execute(Document &doc) override;
  bool undo(Document &doc) override;

private:
  const Uuid id_;
  std::optional<std::string> name_;
  const std::string new_name_;
};

} // namespace ygn::spice::core
