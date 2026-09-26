#include "command_group.hpp"

namespace ygn::spice::core {

CommandGroup::CommandGroup(std::vector<std::unique_ptr<Command>> commands)
    : commands_(std::move(commands)) {
}

CommandResult CommandGroup::execute(Document &doc) {
  if (commands_.empty()) {
    return CommandResult::unchanged(CommandReason::NoChangeNeeded);
  }

  exec_ = 0;

  for (std::size_t i = exec_; i < commands_.size(); i++) {
    CommandResult e = commands_[i]->execute(doc);

    if (e.outcome() == CommandOutcome::Unchanged) {
      if (exec_ == 0) {
        return e;
      } else {
        undo(doc);

        return e;
      }
    }

    exec_++;
  }

  return CommandResult::completed();
}

void CommandGroup::undo(Document &doc) {
  if (exec_ == 0) {
    throw std::logic_error("Nothing to undo");
  }

  for (std::size_t i = exec_; i > 0; i--) {
    commands_[i - 1]->undo(doc);

    exec_--;
  }
}

} // namespace ygn::spice::core
