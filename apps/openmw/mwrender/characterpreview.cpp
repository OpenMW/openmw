#include "characterpreview.hpp"


#include <OgreSceneManager.h>
#include <OgreHardwarePixelBuffer.h>

#include <libs/openengine/ogre/selectionbuffer.hpp>


#include "renderconst.hpp"
#include "npcanimation.hpp"

namespace MWRender
{

    CharacterPreview::CharacterPreview(Ogre::SceneManager *sceneMgr, Ogre::SceneNode *node, int sizeX, int sizeY, const std::string& name,
                                       Ogre::Vector3 position, Ogre::Vector3 lookAt)
        : mSceneMgr(sceneMgr)
        , mSizeX(sizeX)
        , mSizeY(sizeY)
    {
        mCamera = mSceneMgr->createCamera (name);
        mCamera->setAspectRatio (float(sizeX) / float(sizeY));

        mNode = node;
        mNode->setVisible (false);

        mCamera->setPosition(position);
        mCamera->lookAt(lookAt);

        mCamera->setNearClipDistance (0.01);
        mCamera->setFarClipDistance (1000);

        mTexture = Ogre::TextureManager::getSingleton().createManual(name,
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, sizeX, sizeY, 0, Ogre::PF_A8R8G8B8, Ogre::TU_RENDERTARGET);

        mRenderTarget = mTexture->getBuffer()->getRenderTarget();
        mViewport = mRenderTarget->addViewport(mCamera);
        mViewport->setOverlaysEnabled(false);
        mViewport->setBackgroundColour(Ogre::ColourValue(0, 0, 0, 0));
        mViewport->setShadowsEnabled(false);
        mViewport->setMaterialScheme("local_map");
        mViewport->setVisibilityMask (RV_PlayerPreview);
        mRenderTarget->setActive(true);
        mRenderTarget->setAutoUpdated (false);
    }

    // --------------------------------------------------------------------------------------------------


    InventoryPreview::InventoryPreview(Ogre::SceneManager *sceneMgr, Ogre::SceneNode *node)
        : CharacterPreview(sceneMgr, node, 512, 1024, "CharacterPreview", Ogre::Vector3(0, 65, -180), Ogre::Vector3(0,65,0))
        , mAnimation(NULL)
    {
        mSelectionBuffer = new OEngine::Render::SelectionBuffer(mCamera, 512, 1024, RV_PlayerPreview);
    }

    InventoryPreview::~InventoryPreview()
    {
        delete mSelectionBuffer;
    }

    void InventoryPreview::update(int sizeX, int sizeY)
    {
        if (mAnimation)
            mAnimation->forceUpdate ();

        mViewport->setDimensions (0, 0, std::min(1.f, float(sizeX) / float(512)), std::min(1.f, float(sizeY) / float(1024)));

        mNode->setOrientation (Ogre::Quaternion::IDENTITY);

        mNode->setVisible (true);

        mRenderTarget->update();
        mSelectionBuffer->update();

        mNode->setVisible (false);
    }

    void InventoryPreview::setNpcAnimation (NpcAnimation *anim)
    {
        mAnimation = anim;
    }

    int InventoryPreview::getSlotSelected (int posX, int posY)
    {
        std::cout << posX << " " << posY << std::endl;
        return mSelectionBuffer->getSelected (posX, posY);
    }

    // --------------------------------------------------------------------------------------------------

    RaceSelectionPreview::RaceSelectionPreview(Ogre::SceneManager *sceneMgr, Ogre::SceneNode *node)
        : CharacterPreview(sceneMgr, node, 512, 512, "CharacterHeadPreview", Ogre::Vector3(0, 120, -35), Ogre::Vector3(0,125,0))
    {

    }

    void RaceSelectionPreview::update(float angle)
    {
        mNode->roll(Ogre::Radian(angle), Ogre::SceneNode::TS_LOCAL);

        mNode->setVisible (true);
        mRenderTarget->update();
        mNode->setVisible (false);
    }

}
