#include "history.hpp"

namespace ygn::spice::core {
CommandHistory::CommandHistory(Document &doc) : document_(doc), cursor_(0) {
}

bool CommandHistory::can_redo() const {
  return cursor_ < history_.size();
}

bool CommandHistory::can_undo() const {
  return cursor_ > 0;
}

bool CommandHistory::execute(std::unique_ptr<Command> cmd) {
  if (cmd->execute(document_)) {
    cursor_++;

    history_.resize(cursor_);
    history_.at(cursor_ - 1) = std::move(cmd);

    return true;
  }

  return false;
}

bool CommandHistory::undo() {
  if (can_undo()) {
    if (history_.at(cursor_ - 1)->undo(document_)) {
      cursor_--;

      return true;
    }
  }

  return false;
}

bool CommandHistory::redo() {
  if (can_redo()) {
    if (history_.at(cursor_)->execute(document_)) {
      cursor_++;

      return true;
    }
  }

  return false;
}

bool CommandHistory::is_dirty() const {
  return false;
}

bool CommandHistory::mark_saved() const {
  return false;
}
} // namespace ygn::spice::core
