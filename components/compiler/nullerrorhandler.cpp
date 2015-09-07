#include "nullerrorhandler.hpp"

void Compiler::NullErrorHandler::report (const std::string& message, const TokenLoc& loc, Type type) {}

void Compiler::NullErrorHandler::report (const std::string& message, Type type) {}
