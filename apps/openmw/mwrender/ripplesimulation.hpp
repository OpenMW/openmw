#ifndef RIPPLE_SIMULATION_H
#define RIPPLE_SIMULATION_H

#include <OgreTexture.h>
#include <OgreMaterial.h>
#include <OgreVector2.h>
#include <OgreVector3.h>

#include "../mwworld/ptr.hpp"

namespace Ogre
{
    class RenderTexture;
    class Camera;
    class SceneManager;
    class Rectangle2D;
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
    RippleSimulation(Ogre::SceneManager* mainSceneManager);
    ~RippleSimulation();

    void update(float dt, Ogre::Vector2 position);

    /// adds an emitter, position will be tracked automatically
    void addEmitter (const MWWorld::Ptr& ptr, float scale = 1.f, float force = 1.f);
    void removeEmitter (const MWWorld::Ptr& ptr);
    void updateEmitterPtr (const MWWorld::Ptr& old, const MWWorld::Ptr& ptr);

private:
    std::vector<Emitter> mEmitters;

    Ogre::RenderTexture* mRenderTargets[4];
    Ogre::TexturePtr mTextures[4];

    int mTextureSize;
    float mRippleAreaLength;
    float mImpulseSize;

    bool mFirstUpdate;

    Ogre::Camera* mCamera;

    // own scenemanager to render our simulation
    Ogre::SceneManager* mSceneMgr;
    Ogre::Rectangle2D* mRectangle;

    // scenemanager to create the debug overlays on
    Ogre::SceneManager* mMainSceneMgr;

    static const int TEX_NORMAL = 3;

    Ogre::Rectangle2D* mImpulse;

    void addImpulses();
    void heightMapToNormalMap();
    void waterSimulation();
    void swapHeightMaps();

    float mTime;

    Ogre::Vector2 mRippleCenter;

    Ogre::Vector2 mTexelOffset;

    Ogre::Vector2 mCurrentFrameOffset;
    Ogre::Vector2 mPreviousFrameOffset;
};

}

#endif
