#include "activators.hpp"

#include <OgreVector3.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

namespace MWMechanics
{

Activators::Activators()
{
}

void Activators::addActivator(const MWWorld::Ptr& ptr)
{
    MWRender::Animation *anim = MWBase::Environment::get().getWorld()->getAnimation(ptr);
    if(anim != NULL)
        mActivators.insert(std::make_pair(ptr, CharacterController(ptr, anim, CharState_Idle, true)));
}

void Activators::removeActivator (const MWWorld::Ptr& ptr)
{
    PtrControllerMap::iterator iter = mActivators.find(ptr);
    if(iter != mActivators.end())
        mActivators.erase(iter);
}

void Activators::updateActivatorCell(const MWWorld::Ptr &ptr)
{
    PtrControllerMap::iterator iter = mActivators.find(ptr);
    if(iter != mActivators.end())
    {
        CharacterController ctrl = iter->second;
        mActivators.erase(iter);
        mActivators.insert(std::make_pair(ptr, ctrl));
    }
}

void Activators::dropActivators (const MWWorld::Ptr::CellStore *cellStore)
{
    PtrControllerMap::iterator iter = mActivators.begin();
    while(iter != mActivators.end())
    {
        if(iter->first.getCell()==cellStore)
            mActivators.erase(iter++);
        else
            ++iter;
    }
}

void Activators::update(float duration, bool paused)
{
    if(!paused)
    {
        for(PtrControllerMap::iterator iter(mActivators.begin());iter != mActivators.end();++iter)
            iter->second.update(duration);
    }
}

void Activators::playAnimationGroup(const MWWorld::Ptr& ptr, const std::string& groupName, int mode, int number)
{
    PtrControllerMap::iterator iter = mActivators.find(ptr);
    if(iter != mActivators.end())
        iter->second.playGroup(groupName, mode, number);
}
void Activators::skipAnimation(const MWWorld::Ptr& ptr)
{
    PtrControllerMap::iterator iter = mActivators.find(ptr);
    if(iter != mActivators.end())
        iter->second.skipAnim();
}

}
