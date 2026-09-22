#pragma once

#include "commands.hpp"
#include "document.hpp"

#include <memory>

namespace ygn::spice::core {

class CommandHistory {
public:
  CommandHistory(Document &doc);

  bool can_undo() const;
  bool can_redo() const;

  bool execute(std::unique_ptr<Command> cmd);
  bool undo();
  bool redo();

  bool mark_saved() const;
  bool is_dirty() const;

private:
  Document &document_;
  std::size_t cursor_;
  std::vector<std::unique_ptr<Command>> history_;
};

} // namespace ygn::spice::core
