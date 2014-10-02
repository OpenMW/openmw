#ifndef GAME_SHADOWS_H
#define GAME_SHADOWS_H

// forward declares
namespace Ogre
{
    class SceneManager;
    class PSSMShadowCameraSetup;
}
namespace OEngine{
    namespace Render{
        class OgreRenderer;
    }
}

namespace MWRender
{
    class Shadows
    {
    public:
        Shadows(OEngine::Render::OgreRenderer* rend);

        void recreate();

        Ogre::PSSMShadowCameraSetup* getPSSMSetup();

    protected:
        OEngine::Render::OgreRenderer* mRendering;
        Ogre::SceneManager* mSceneMgr;

        Ogre::PSSMShadowCameraSetup* mPSSMSetup;
        float mShadowFar;
        float mFadeStart;
    };
}

#endif
