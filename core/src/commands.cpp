#include "commands.hpp"

#include <stdexcept>

namespace ygn::spice::core {

CommandResult CommandResult::completed() {
  return CommandResult(CommandOutcome::Completed, std::nullopt);
}

CommandResult CommandResult::unchanged(std::optional<CommandReason> reason) {
  return CommandResult(CommandOutcome::Unchanged, reason);
}

CommandOutcome CommandResult::outcome() const {
  return outcome_;
}

std::optional<CommandReason> CommandResult::reason() const {
  return reason_;
}

CommandResult::CommandResult(CommandOutcome outcome, std::optional<CommandReason> reason)
    : outcome_(outcome), reason_(reason) {
}

CommandResult::operator bool() const {
  return outcome_ == CommandOutcome::Completed;
}

RenameComponentCommand::RenameComponentCommand(Uuid id, std::string new_name)
    : id_(id), new_name_(new_name) {
}

CommandResult RenameComponentCommand::execute(Document &doc) {
  const ComponentInstance *comp = doc.lookup(id_);

  if (comp == nullptr) {
    return CommandResult::unchanged(CommandReason::NotFound);
  }

  std::string name = comp->designator();

  if (name == new_name_) {
    return CommandResult::unchanged(CommandReason::NoChangeNeeded);
  }

  if (!doc.rename_component(id_, new_name_)) {
    return CommandResult::unchanged(CommandReason::NotFound);
  }

  name_ = std::move(name);

  return CommandResult::completed();
}

void RenameComponentCommand::undo(Document &doc) {
  if (!name_.has_value()) {
    throw std::logic_error("RenameComponentCommand::undo: nothing to undo.");
  }

  if (!doc.rename_component(id_, name_.value())) {
    throw std::logic_error(
      std::format("RenameComponentCommand::undo: component {} not found", id_.to_string()));
  }

  name_ = std::nullopt;
}

} // namespace ygn::spice::core
