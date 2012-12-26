#ifndef INTERPRETER_DEFINES_H_INCLUDED
#define INTERPRETER_DEFINES_H_INCLUDED

#include <string>
#include "context.hpp"

namespace Interpreter{
    std::string fixDefinesDialog(std::string text, Context& context);
    std::string fixDefinesMsgBox(std::string text, Context& context);
    std::string fixDefinesBook(std::string text, Context& context);
}

#endif
