#pragma once

#include "commands.hpp"
#include "document.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace ygn::spice::core {

class CommandGroup : public Command {
public:
  CommandGroup(std::vector<std::unique_ptr<Command>> commands);

  bool execute(Document &doc) override;
  bool undo(Document &doc) override;

private:
  std::vector<std::unique_ptr<Command>> commands_;
  std::size_t exec_;
};

} // namespace ygn::spice::core
