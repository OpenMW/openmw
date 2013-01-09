#ifndef MWRENDER_CHARACTERPREVIEW_H
#define MWRENDER_CHARACTERPREVIEW_H

#include <OgreRenderTarget.h>
#include <OgreMaterialManager.h>

#include <components/esm/loadnpc.hpp>

#include "externalrendering.hpp"

#include "../mwworld/ptr.hpp"

namespace OEngine
{
namespace Render
{
class SelectionBuffer;
}
}

namespace MWRender
{

    class NpcAnimation;

    class CharacterPreview
    {
    public:
        CharacterPreview(MWWorld::Ptr character, int sizeX, int sizeY, const std::string& name,
                         Ogre::Vector3 position, Ogre::Vector3 lookAt);
        virtual ~CharacterPreview();

        virtual void setup ();
        virtual void onSetup();

        virtual void rebuild();

    protected:
        Ogre::TexturePtr mTexture;
        Ogre::RenderTarget* mRenderTarget;
        Ogre::Viewport* mViewport;

        Ogre::Camera* mCamera;

        Ogre::SceneManager* mSceneMgr;
        Ogre::SceneNode* mNode;

        Ogre::Vector3 mPosition;
        Ogre::Vector3 mLookAt;

        MWWorld::Ptr mCharacter;

        MWRender::NpcAnimation* mAnimation;

        std::string mName;

        int mSizeX;
        int mSizeY;
    };

    class InventoryPreview : public CharacterPreview
    {
    public:

        InventoryPreview(MWWorld::Ptr character);
        virtual ~InventoryPreview();
        virtual void onSetup();

        void update(int sizeX, int sizeY);

        int getSlotSelected(int posX, int posY);

        void setNpcAnimation (NpcAnimation* anim);

    private:

        OEngine::Render::SelectionBuffer* mSelectionBuffer;
    };

    class RaceSelectionPreview : public CharacterPreview
    {
        ESM::NPC                        mBase;
        MWWorld::LiveCellRef<ESM::NPC>  mRef;

    public:
        RaceSelectionPreview();

        void update(float angle);

        const ESM::NPC &getPrototype() const {
            return mBase;
        }

        void setPrototype(const ESM::NPC &proto);
    };

}

#endif
