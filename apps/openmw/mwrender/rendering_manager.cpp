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
