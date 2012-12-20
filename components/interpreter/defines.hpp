#ifndef GAME_MWMECHANICS_DEFINES_H
#define GAME_MWMECHANICS_DEFINES_H

#include <string>
#include "context.hpp"

namespace Interpreter{
    std::string fixDefinesDialog(std::string text, Context& context);
    std::string fixDefinesMsgBox(std::string text, Context& context);
}

#endif
