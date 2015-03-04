#ifndef RIPPLE_SIMULATION_H
#define RIPPLE_SIMULATION_H

#include <OgreVector3.h>

#include "../mwworld/ptr.hpp"

namespace Ogre
{
    class SceneManager;
    class ParticleSystem;
}

namespace MWWorld
{
    class Fallback;
}

namespace MWRender
{

struct Emitter
{
    MWWorld::Ptr mPtr;
    Ogre::Vector3 mLastEmitPosition;
    float mScale;
    float mForce;
};

class RippleSimulation
{
public:
    RippleSimulation(Ogre::SceneManager* mainSceneManager, const MWWorld::Fallback* fallback);
    ~RippleSimulation();

    /// @param dt Time since the last frame
    /// @param position Position of the player
    void update(float dt, Ogre::Vector2 position);

    /// adds an emitter, position will be tracked automatically
    void addEmitter (const MWWorld::Ptr& ptr, float scale = 1.f, float force = 1.f);
    void removeEmitter (const MWWorld::Ptr& ptr);
    void updateEmitterPtr (const MWWorld::Ptr& old, const MWWorld::Ptr& ptr);

    /// Change the height of the water surface, thus moving all ripples with it
    void setWaterHeight(float height);

    /// Remove all active ripples
    void clear();

private:
    Ogre::SceneManager* mSceneMgr;
    Ogre::ParticleSystem* mParticleSystem;
    Ogre::SceneNode* mSceneNode;

    std::vector<Emitter> mEmitters;

    float mRippleLifeTime;
    float mRippleRotSpeed;
};

}

#endif
