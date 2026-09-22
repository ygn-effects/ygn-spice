#include "command_group.hpp"

namespace ygn::spice::core {

CommandGroup::CommandGroup(std::vector<std::unique_ptr<Command>> commands)
    : commands_(std::move(commands)), exec_(0) {
}

bool CommandGroup::execute(Document &doc) {
  if (commands_.size() == 0) {
    return false;
  }

  for (std::size_t i = exec_; i < commands_.size(); i++) {
    if (!commands_.at(i)->execute(doc)) {
      undo(doc);

      return false;
    }

    exec_++;
  }

  return true;
}

bool CommandGroup::undo(Document &doc) {
  if (exec_ == 0) {
    return false;
  }

  for (std::size_t i = exec_; i > 0; i--) {
    if (!commands_.at(i - 1)->undo(doc)) {
      return false;
    }

    exec_--;
  }

  return true;
}

} // namespace ygn::spice::core
