#ifndef INTERPRETER_DEFINES_H_INCLUDED
#define INTERPRETER_DEFINES_H_INCLUDED

#include <string>

namespace Interpreter
{
    class Context;

    std::string fixDefinesDialog(const std::string& text, Context& context);
    std::string fixDefinesMsgBox(const std::string& text, Context& context);
    std::string fixDefinesBook(const std::string& text, Context& context);
}

#endif
