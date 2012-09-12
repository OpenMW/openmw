#include "characterpreview.hpp"


#include <OgreSceneManager.h>
#include <OgreHardwarePixelBuffer.h>


#include "renderconst.hpp"

namespace MWRender
{

    CharacterPreview::CharacterPreview(Ogre::SceneManager *sceneMgr, Ogre::SceneNode *node)
        : mSceneMgr(sceneMgr)
    {
        mCamera = mSceneMgr->createCamera ("CharacterPreviewCamera");
        mCamera->setAspectRatio (0.5);

        mNode = node->createChildSceneNode ();
        mNode->attachObject (mCamera);

        mNode->setPosition(0, 185, 70);
        mNode->roll(Ogre::Degree(180));

        mNode->pitch(Ogre::Degree(90));

        mTexture = Ogre::TextureManager::getSingleton().createManual("CharacterPreview",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, 512, 1024, 0, Ogre::PF_A8R8G8B8, Ogre::TU_RENDERTARGET);

        mRenderTarget = mTexture->getBuffer()->getRenderTarget();
        mViewport = mRenderTarget->addViewport(mCamera);
        mViewport->setOverlaysEnabled(false);
        mViewport->setBackgroundColour(Ogre::ColourValue(0, 0, 0, 0));
        mViewport->setShadowsEnabled(false);
        mViewport->setMaterialScheme("local_map");
        mViewport->setVisibilityMask (RV_Player);
        mRenderTarget->setActive(true);
        mRenderTarget->setAutoUpdated (false);
    }

    void CharacterPreview::update(int sizeX, int sizeY)
    {
        bool wasVisible = mNode->getParentSceneNode()->getAttachedObject(0)->getVisible ();
        mNode->getParentSceneNode()->setVisible(true, false);
        //mViewport->setDimensions (0, 0, float(sizeX) / float(512), float(sizeY) / float(1024));

        mRenderTarget->update();

        mNode->getParentSceneNode()->setVisible(wasVisible, false);
    }

}
