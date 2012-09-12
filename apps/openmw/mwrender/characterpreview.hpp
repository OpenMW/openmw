#ifndef MWRENDER_CHARACTERPREVIEW_H
#define MWRENDER_CHARACTERPREVIEW_H

#include <OgreRenderTarget.h>

namespace MWRender
{

    class CharacterPreview
    {
    public:
        CharacterPreview(Ogre::SceneManager* sceneMgr, Ogre::SceneNode* node);

        void update(int sizeX, int sizeY);

    private:
        Ogre::TexturePtr mTexture;
        Ogre::RenderTarget* mRenderTarget;
        Ogre::Viewport* mViewport;

        Ogre::Camera* mCamera;

        Ogre::SceneManager* mSceneMgr;
        Ogre::SceneNode* mNode;
    };

}

#endif
