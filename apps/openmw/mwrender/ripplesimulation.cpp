#include "ripplesimulation.hpp"

#include <stdexcept>

#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreParticleSystem.h>
#include <OgreParticle.h>

#include <extern/shiny/Main/Factory.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/fallback.hpp"

#include "renderconst.hpp"

namespace MWRender
{

RippleSimulation::RippleSimulation(Ogre::SceneManager* mainSceneManager, const MWWorld::Fallback* fallback)
    : mSceneMgr(mainSceneManager)
    , mParticleSystem(NULL)
    , mSceneNode(NULL)
{
    mRippleLifeTime = fallback->getFallbackFloat("Water_RippleLifetime");
    mRippleRotSpeed = fallback->getFallbackFloat("Water_RippleRotSpeed");

    // Unknown:
    // fallback=Water_RippleScale,0.15, 6.5
    // fallback=Water_RippleAlphas,0.7, 0.1, 0.01

    // Instantiate from ripples.particle file
    mParticleSystem = mSceneMgr->createParticleSystem("openmw/Ripples", "openmw/Ripples");

    mParticleSystem->setRenderQueueGroup(RQG_Ripples);
    mParticleSystem->setVisibilityFlags(RV_Effects);

    int rippleFrameCount = fallback->getFallbackInt("Water_RippleFrameCount");
    std::string tex = fallback->getFallbackString("Water_RippleTexture");

    sh::MaterialInstance* mat = sh::Factory::getInstance().getMaterialInstance("openmw/Ripple");
    mat->setProperty("anim_texture2", sh::makeProperty(new sh::StringValue(std::string("textures\\water\\") + tex + ".dds "
                                                                           + Ogre::StringConverter::toString(rippleFrameCount)
                                                                           + " "
                                                                           + Ogre::StringConverter::toString(0.3))));

    // seems to be required to allocate mFreeParticles. TODO: patch Ogre to handle this better
    mParticleSystem->_update(0.f);

    mSceneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    mSceneNode->attachObject(mParticleSystem);
}

RippleSimulation::~RippleSimulation()
{
    if (mParticleSystem)
        mSceneMgr->destroyParticleSystem(mParticleSystem);
    mParticleSystem = NULL;

    if (mSceneNode)
        mSceneMgr->destroySceneNode(mSceneNode);
    mSceneNode = NULL;
}

void RippleSimulation::update(float dt, Ogre::Vector2 position)
{
    bool newParticle = false;
    for (std::vector<Emitter>::iterator it=mEmitters.begin(); it !=mEmitters.end(); ++it)
    {
        if (it->mPtr == MWBase::Environment::get().getWorld ()->getPlayerPtr())
        {
            // fetch a new ptr (to handle cell change etc)
            // for non-player actors this is done in updateObjectCell
            it->mPtr = MWBase::Environment::get().getWorld ()->getPlayerPtr();
        }
        Ogre::Vector3 currentPos (it->mPtr.getRefData().getPosition().pos);
        currentPos.z = 0;
        if ( (currentPos - it->mLastEmitPosition).length() > 10
             // Only emit when close to the water surface, not above it and not too deep in the water
            && MWBase::Environment::get().getWorld ()->isUnderwater (it->mPtr.getCell(),
                Ogre::Vector3(it->mPtr.getRefData().getPosition().pos))
             && !MWBase::Environment::get().getWorld()->isSubmerged(it->mPtr))
        {
            it->mLastEmitPosition = currentPos;

            newParticle = true;
            Ogre::Particle* created = mParticleSystem->createParticle();
            if (!created)
                break; // TODO: cleanup the oldest particle to make room
#if OGRE_VERSION >= (1 << 16 | 10 << 8 | 0)
            Ogre::Vector3& position = created->mPosition;
            Ogre::Vector3& direction = created->mDirection;
            Ogre::ColourValue& colour = created->mColour;
            float& totalTimeToLive = created->mTotalTimeToLive;
            float& timeToLive = created->mTimeToLive;
            Ogre::Radian& rotSpeed = created->mRotationSpeed;
            Ogre::Radian& rotation = created->mRotation;
#else
            Ogre::Vector3& position = created->position;
            Ogre::Vector3& direction = created->direction;
            Ogre::ColourValue& colour = created->colour;
            float& totalTimeToLive = created->totalTimeToLive;
            float& timeToLive = created->timeToLive;
            Ogre::Radian& rotSpeed = created->rotationSpeed;
            Ogre::Radian& rotation = created->rotation;
#endif
            timeToLive = totalTimeToLive = mRippleLifeTime;
            colour = Ogre::ColourValue(0.f, 0.f, 0.f, 0.7f); // Water_RippleAlphas.x?
            direction = Ogre::Vector3(0,0,0);
            position = currentPos;
            position.z = 0; // Z is set by the Scene Node
            rotSpeed = mRippleRotSpeed;
            rotation = Ogre::Radian(Ogre::Math::RangeRandom(-Ogre::Math::PI, Ogre::Math::PI));
            created->setDimensions(mParticleSystem->getDefaultWidth(), mParticleSystem->getDefaultHeight());
        }
    }

    if (newParticle) // now apparently needs another update, otherwise it won't render in the first frame after a particle is created. TODO: patch Ogre to handle this better
        mParticleSystem->_update(0.f);
}

void RippleSimulation::addEmitter(const MWWorld::Ptr& ptr, float scale, float force)
{
    Emitter newEmitter;
    newEmitter.mPtr = ptr;
    newEmitter.mScale = scale;
    newEmitter.mForce = force;
    newEmitter.mLastEmitPosition = Ogre::Vector3(0,0,0);
    mEmitters.push_back (newEmitter);
}

void RippleSimulation::removeEmitter (const MWWorld::Ptr& ptr)
{
    for (std::vector<Emitter>::iterator it = mEmitters.begin(); it != mEmitters.end(); ++it)
    {
        if (it->mPtr == ptr)
        {
            mEmitters.erase(it);
            return;
        }
    }
}

void RippleSimulation::updateEmitterPtr (const MWWorld::Ptr& old, const MWWorld::Ptr& ptr)
{
    for (std::vector<Emitter>::iterator it = mEmitters.begin(); it != mEmitters.end(); ++it)
    {
        if (it->mPtr == old)
        {
            it->mPtr = ptr;
            return;
        }
    }
}

void RippleSimulation::setWaterHeight(float height)
{
    mSceneNode->setPosition(0,0,height);
}

void RippleSimulation::clear()
{
    mParticleSystem->clear();
}


}
