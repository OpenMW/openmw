#include "rendering_manager.hpp"

namespace MWRender {



RenderingManager::RenderingManager (SkyManager *skyManager) :
    mSkyManager(skyManager)
{

}

RenderingManager::~RenderingManager ()
{
    delete mSkyManager;
}

void RenderingManager::removeCell (MWWorld::Ptr::CellStore *store){

}
void RenderingManager::addObject (const MWWorld::Ptr& ptr, MWWorld::Ptr::CellStore *store){

}
void RenderingManager::removeObject (const MWWorld::Ptr& ptr, MWWorld::Ptr::CellStore *store){

}
void RenderingManager::moveObject (const MWWorld::Ptr& ptr, const Ogre::Vector3& position){

}
void RenderingManager::scaleObject (const MWWorld::Ptr& ptr, const Ogre::Vector3& scale){

}
void RenderingManager::rotateObject (const MWWorld::Ptr& ptr, const::Ogre::Quaternion& orientation){

}
void RenderingManager::moveObjectToCell (const MWWorld::Ptr& ptr, const Ogre::Vector3& position, MWWorld::Ptr::CellStore *store){

}
void RenderingManager::setPhysicsDebugRendering (bool){

}
bool RenderingManager::getPhysicsDebugRendering() const{
	return true;
}
void RenderingManager::update (float duration){


}

void RenderingManager::skyEnable ()
{
    mSkyManager->enable();
}

void RenderingManager::skyDisable ()
{
    mSkyManager->disable();
}

void RenderingManager::skySetHour (double hour)
{
    mSkyManager->setHour(hour);
}


void RenderingManager::skySetDate (int day, int month)
{
    mSkyManager->setDate(day, month);
}

int RenderingManager::skyGetMasserPhase() const
{
    return mSkyManager->getMasserPhase();
}

int RenderingManager::skyGetSecundaPhase() const
{
    return mSkyManager->getSecundaPhase();
}

void RenderingManager::skySetMoonColour (bool red)
{
    mSkyManager->setMoonColour(red);
}

}
