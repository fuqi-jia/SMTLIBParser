/* -*- Source -*-
 * Command and Script implementation
 */

#include "somtparser/frontend/command.h"
#include "somtparser/frontend/parser.h"

namespace SOMTParser {

bool Command::isAssert() const { return type == CMD_TYPE::CT_ASSERT; }
bool Command::isPush() const { return type == CMD_TYPE::CT_PUSH; }
bool Command::isPop() const { return type == CMD_TYPE::CT_POP; }
bool Command::isReset() const { return type == CMD_TYPE::CT_RESET; }
bool Command::isResetAssertions() const { return type == CMD_TYPE::CT_RESET_ASSERTIONS; }

void Script::addCommand(const Command& cmd) {
    commands_.push_back(cmd);
}

void Script::addCommand(Command&& cmd) {
    commands_.push_back(std::move(cmd));
}

const std::vector<Command>& Script::commands() const {
    return commands_;
}

size_t Script::size() const {
    return commands_.size();
}

const Command& Script::operator[](size_t i) const {
    return commands_[i];
}

void Script::clear() {
    commands_.clear();
}

} // namespace SOMTParser
