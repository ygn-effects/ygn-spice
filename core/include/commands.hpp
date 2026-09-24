#pragma once

#include "document.hpp"
#include "uuid.hpp"

#include <format>
#include <optional>
#include <string>

namespace ygn::spice::core {
enum class CommandOutcome { Completed, Unchanged };
enum class CommandReason { NotFound, AlreadyExists, NoChangeNeeded };

struct CommandResult {
public:
  static CommandResult completed();
  static CommandResult unchanged(std::optional<CommandReason> reason);

  CommandOutcome outcome() const;
  std::optional<CommandReason> reason() const;

  explicit operator bool() const;

private:
  CommandResult(CommandOutcome outcome, std::optional<CommandReason> reason);

  CommandOutcome outcome_;
  std::optional<CommandReason> reason_;
};

class Command {
public:
  virtual CommandResult execute(Document &doc) = 0;
  virtual void undo(Document &doc) = 0;

  virtual ~Command() = default;
};

class RenameComponentCommand : public Command {
public:
  RenameComponentCommand(const Uuid id, const std::string new_name);

  CommandResult execute(Document &doc) override;
  void undo(Document &doc) override;

private:
  const Uuid id_;
  std::optional<std::string> name_;
  const std::string new_name_;
};

} // namespace ygn::spice::core
