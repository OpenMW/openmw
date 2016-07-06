#include "objects.hpp"

#include <iostream>

#include "movement.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

namespace MWMechanics
{

Objects::Objects()
{
}

Objects::~Objects()
{
  PtrControllerMap::iterator it(mObjects.begin());
  for (; it != mObjects.end();++it)
  {
    delete it->second;
    it->second = NULL;
  }
}

void Objects::addObject(const MWWorld::Ptr& ptr)
{
    removeObject(ptr);

    MWRender::Animation *anim = MWBase::Environment::get().getWorld()->getAnimation(ptr);
    if(anim) mObjects.insert(std::make_pair(ptr, new CharacterController(ptr, anim)));
}

void Objects::removeObject(const MWWorld::Ptr& ptr)
{
    PtrControllerMap::iterator iter = mObjects.find(ptr);
    if(iter != mObjects.end())
    {
        delete iter->second;
        mObjects.erase(iter);
    }
}

void Objects::updateObject(const MWWorld::Ptr &old, const MWWorld::Ptr &ptr)
{
    PtrControllerMap::iterator iter = mObjects.find(old);
    if(iter != mObjects.end())
    {
        CharacterController *ctrl = iter->second;
        mObjects.erase(iter);

        ctrl->updatePtr(ptr);
        mObjects.insert(std::make_pair(ptr, ctrl));
    }
}

void Objects::dropObjects (const MWWorld::CellStore *cellStore)
{
    PtrControllerMap::iterator iter = mObjects.begin();
    while(iter != mObjects.end())
    {
        if(iter->first.getCell()==cellStore)
        {
            delete iter->second;
            mObjects.erase(iter++);
        }
        else
            ++iter;
    }
}

void Objects::update(float duration, bool paused)
{
    if(!paused)
    {
        for(PtrControllerMap::iterator iter(mObjects.begin());iter != mObjects.end();++iter)
            iter->second->update(duration);
    }
}

bool Objects::playAnimationGroup(const MWWorld::Ptr& ptr, const std::string& groupName, int mode, int number)
{
    PtrControllerMap::iterator iter = mObjects.find(ptr);
    if(iter != mObjects.end())
    {
        return iter->second->playGroup(groupName, mode, number);
    }
    else
    {
        std::cerr<< "Error in Objects::playAnimationGroup:  Unable to find " << ptr.getCellRef().getRefId() << std::endl;
        return false;
    }
}
void Objects::skipAnimation(const MWWorld::Ptr& ptr)
{
    PtrControllerMap::iterator iter = mObjects.find(ptr);
    if(iter != mObjects.end())
        iter->second->skipAnim();
}

void Objects::getObjectsInRange(const osg::Vec3f& position, float radius, std::vector<MWWorld::Ptr>& out)
{
    for (PtrControllerMap::iterator iter = mObjects.begin(); iter != mObjects.end(); ++iter)
    {
        if ((position - iter->first.getRefData().getPosition().asVec3()).length2() <= radius*radius)
            out.push_back(iter->first);
    }
}

}
