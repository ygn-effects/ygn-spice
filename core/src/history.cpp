#include "history.hpp"

namespace ygn::spice::core {
CommandHistory::CommandHistory(Document &doc) : document_(doc), saved_cursor_(0), cursor_(0) {
}

bool CommandHistory::can_redo() const {
  return cursor_ < history_.size();
}

bool CommandHistory::can_undo() const {
  return cursor_ > 0;
}

bool CommandHistory::execute(std::unique_ptr<Command> cmd) {
  if (cmd->execute(document_)) {
    if (saved_cursor_ > cursor_) {
      saved_cursor_ = std::nullopt;
    }

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
  return !saved_cursor_.has_value() || saved_cursor_ != cursor_;
}

void CommandHistory::mark_saved() {
  saved_cursor_ = cursor_;
}
} // namespace ygn::spice::core
