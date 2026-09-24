#pragma once

#include "commands.hpp"
#include "document.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace ygn::spice::core {

class CommandHistory {
public:
  CommandHistory(Document &doc);

  bool can_undo() const;
  bool can_redo() const;

  CommandResult execute(std::unique_ptr<Command> cmd);
  bool undo();
  bool redo();

  void mark_saved();
  bool is_dirty() const;

private:
  Document &document_;
  std::optional<std::size_t> saved_cursor_;
  std::size_t cursor_;
  std::vector<std::unique_ptr<Command>> history_;
};

} // namespace ygn::spice::core
