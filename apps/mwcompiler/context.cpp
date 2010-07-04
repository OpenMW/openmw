
#include "context.hpp"

namespace SACompiler
{
    bool Context::canDeclareLocals() const
    {
        return true;
    }
 
    char Context::getGlobalType (const std::string& name) const
    {
        return ' ';
    }
}

