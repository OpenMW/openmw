#ifndef MWRENDER_CHARACTERPREVIEW_H
#define MWRENDER_CHARACTERPREVIEW_H

#include <OgreRenderTarget.h>

namespace MWRender
{

    class NpcAnimation;

    class CharacterPreview
    {
    public:
        CharacterPreview(Ogre::SceneManager* sceneMgr, Ogre::SceneNode* node, int sizeX, int sizeY, const std::string& name,
                         Ogre::Vector3 position, Ogre::Vector3 lookAt);

    protected:
        Ogre::TexturePtr mTexture;
        Ogre::RenderTarget* mRenderTarget;
        Ogre::Viewport* mViewport;

        Ogre::Camera* mCamera;

        Ogre::SceneManager* mSceneMgr;
        Ogre::SceneNode* mNode;

        int mSizeX;
        int mSizeY;
    };

    class InventoryPreview : public CharacterPreview
    {
    public:
        InventoryPreview(Ogre::SceneManager* sceneMgr, Ogre::SceneNode* node);

        void update(int sizeX, int sizeY);

        void setNpcAnimation (NpcAnimation* anim);

    private:
        NpcAnimation* mAnimation;
    };

    class RaceSelectionPreview : public CharacterPreview
    {
    public:
        RaceSelectionPreview(Ogre::SceneManager* sceneMgr, Ogre::SceneNode* node);

        void update(float angle);
    };

}

#endif
