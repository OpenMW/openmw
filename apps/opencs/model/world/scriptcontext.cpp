
#include "scriptcontext.hpp"

bool CSMWorld::ScriptContext::canDeclareLocals() const
{
    return false;
}

char CSMWorld::ScriptContext::getGlobalType (const std::string& name) const
{
    return ' ';
}

char CSMWorld::ScriptContext::getMemberType (const std::string& name, const std::string& id) const
{
    return ' ';
}

bool CSMWorld::ScriptContext::isId (const std::string& name) const
{
    return false;
}