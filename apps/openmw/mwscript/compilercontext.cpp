
#include "compilercontext.hpp"

#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"

namespace MWScript
{
    CompilerContext::CompilerContext (Type type, const MWWorld::Environment& environment)
    : mType (type), mEnvironment (environment)
    {}

    bool CompilerContext::canDeclareLocals() const
    {
        return mType==Type_Full;
    }   
    
    char CompilerContext::getGlobalType (const std::string& name) const
    {
        if (const ESM::Global *global = mEnvironment.mWorld->getStore().globals.find (name))
            return global->type;

        return ' ';
    }    
}

