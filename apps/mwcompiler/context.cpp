
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
    
    bool Context::isId (const std::string& name) const
    {
        return false;
    }
}

