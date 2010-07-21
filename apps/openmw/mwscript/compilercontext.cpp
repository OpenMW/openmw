
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
            mEnvironment.mWorld->getStore().activators.find (name) ||
            mEnvironment.mWorld->getStore().potions.find (name) || 
            mEnvironment.mWorld->getStore().appas.find (name) || 
            mEnvironment.mWorld->getStore().armors.find (name) || 
            mEnvironment.mWorld->getStore().books.find (name) || 
            mEnvironment.mWorld->getStore().clothes.find (name) || 
            mEnvironment.mWorld->getStore().containers.find (name) || 
            mEnvironment.mWorld->getStore().creatures.find (name) || 
            mEnvironment.mWorld->getStore().doors.find (name) || 
            mEnvironment.mWorld->getStore().ingreds.find (name) || 
            mEnvironment.mWorld->getStore().creatureLists.find (name) || 
            mEnvironment.mWorld->getStore().itemLists.find (name) || 
            mEnvironment.mWorld->getStore().lights.find (name) || 
            mEnvironment.mWorld->getStore().lockpicks.find (name) || 
            mEnvironment.mWorld->getStore().miscItems.find (name) || 
            mEnvironment.mWorld->getStore().npcs.find (name) || 
            mEnvironment.mWorld->getStore().probes.find (name) || 
            mEnvironment.mWorld->getStore().repairs.find (name) || 
            mEnvironment.mWorld->getStore().statics.find (name) || 
            mEnvironment.mWorld->getStore().weapons.find (name);
    }
}

