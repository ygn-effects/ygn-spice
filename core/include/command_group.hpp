#pragma once

#include "commands.hpp"
#include "document.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace ygn::spice::core {

class CommandGroup : public Command {
public:
  explicit CommandGroup(std::vector<std::unique_ptr<Command>> commands);

  CommandResult execute(Document &doc) override;
  void undo(Document &doc) override;

private:
  std::vector<std::unique_ptr<Command>> commands_;
  std::size_t exec_ = 0;
};

} // namespace ygn::spice::core
