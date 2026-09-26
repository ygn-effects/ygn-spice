#include "history.hpp"

namespace ygn::spice::core {
CommandHistory::CommandHistory(Document &doc) : document_(doc) {
}

bool CommandHistory::can_redo() const {
  return cursor_ < history_.size();
}

bool CommandHistory::can_undo() const {
  return cursor_ > 0;
}

CommandResult CommandHistory::execute(std::unique_ptr<Command> cmd) {
  if (CommandResult r = cmd->execute(document_)) {
    if (saved_cursor_ > cursor_) {
      saved_cursor_ = std::nullopt;
    }

    cursor_++;

    history_.erase(history_.begin() + cursor_ - 1, history_.end());
    history_.push_back(std::move(cmd));

    return r;
  } else {
    return r;
  }
}

bool CommandHistory::undo() {
  if (!can_undo()) {
    return false;
  }

  history_[cursor_ - 1]->undo(document_);
  cursor_--;

  return true;
}

bool CommandHistory::redo() {
  if (!can_redo()) {
    return false;
  }

  if (history_[cursor_]->execute(document_)) {
    cursor_++;

    return true;
  } else {
    throw std::logic_error("Error replaying history.");
  }
}

bool CommandHistory::is_dirty() const {
  return saved_cursor_ != cursor_;
}

void CommandHistory::mark_saved() {
  saved_cursor_ = cursor_;
}
} // namespace ygn::spice::core
