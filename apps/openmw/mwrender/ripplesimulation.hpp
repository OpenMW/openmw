#ifndef RIPPLE_SIMULATION_H
#define RIPPLE_SIMULATION_H

#include <OgreTexture.h>
#include <OgreMaterial.h>
#include <OgreVector2.h>

namespace Ogre
{
    class RenderTexture;
    class Camera;
    class SceneManager;
    class Rectangle2D;
}

namespace MWRender
{

class RippleSimulation
{
public:
    RippleSimulation(Ogre::SceneManager* mainSceneManager);
    ~RippleSimulation();

    void update(float dt, Ogre::Vector2 position);

    void addImpulse (Ogre::Vector2 position);

private:
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

    std::queue <Ogre::Vector2> mImpulses;

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
