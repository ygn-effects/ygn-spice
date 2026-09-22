#include "commands.hpp"

namespace ygn::spice::core {
RenameComponentCommand::RenameComponentCommand(const Uuid id, const std::string new_name)
    : id_(id), new_name_(new_name) {
}

bool RenameComponentCommand::execute(Document &doc) {
  const ComponentInstance *comp = doc.lookup(id_);

  if (comp != nullptr) {
    std::string name = comp->designator();

    if (name != new_name_) {
      if (doc.rename_component(id_, new_name_)) {
        name_ = name;

        return true;
      }
    }
  }

  return false;
}

bool RenameComponentCommand::undo(Document &doc) {
  const ComponentInstance *comp = doc.lookup(id_);

  if (comp != nullptr) {
    if (comp->designator() == new_name_ && name_.has_value()) {
      if (doc.rename_component(id_, name_.value())) {
        return true;
      }
    }
  }

  return false;
}

} // namespace ygn::spice::core
