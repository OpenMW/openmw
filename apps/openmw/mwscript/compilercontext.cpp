
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
        return mEnvironment.mWorld->getGlobalVariableType (name);
    }    
    
    bool CompilerContext::isId (const std::string& name) const
    {
        return
            mEnvironment.mWorld->getStore().activators.search (name) ||
            mEnvironment.mWorld->getStore().potions.search (name) || 
            mEnvironment.mWorld->getStore().appas.search (name) || 
            mEnvironment.mWorld->getStore().armors.search (name) || 
            mEnvironment.mWorld->getStore().books.search (name) || 
            mEnvironment.mWorld->getStore().clothes.search (name) || 
            mEnvironment.mWorld->getStore().containers.search (name) || 
            mEnvironment.mWorld->getStore().creatures.search (name) || 
            mEnvironment.mWorld->getStore().doors.search (name) || 
            mEnvironment.mWorld->getStore().ingreds.search (name) || 
            mEnvironment.mWorld->getStore().creatureLists.search (name) || 
            mEnvironment.mWorld->getStore().itemLists.search (name) || 
            mEnvironment.mWorld->getStore().lights.search (name) || 
            mEnvironment.mWorld->getStore().lockpicks.search (name) || 
            mEnvironment.mWorld->getStore().miscItems.search (name) || 
            mEnvironment.mWorld->getStore().npcs.search (name) || 
            mEnvironment.mWorld->getStore().probes.search (name) || 
            mEnvironment.mWorld->getStore().repairs.search (name) || 
            mEnvironment.mWorld->getStore().statics.search (name) || 
            mEnvironment.mWorld->getStore().weapons.search (name);
    }
}

