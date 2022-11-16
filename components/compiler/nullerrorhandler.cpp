#include "nullerrorhandler.hpp"

#include <components/compiler/errorhandler.hpp>

namespace Compiler
{
    struct TokenLoc;
}

void Compiler::NullErrorHandler::report(const std::string& message, const TokenLoc& loc, Type type) {}

void Compiler::NullErrorHandler::report(const std::string& message, Type type) {}
