#include "characterpreview.hpp"


#include <OgreSceneManager.h>
#include <OgreRoot.h>
#include <OgreHardwarePixelBuffer.h>

#include <libs/openengine/ogre/selectionbuffer.hpp>


#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"

#include "renderconst.hpp"
#include "npcanimation.hpp"

namespace MWRender
{

    CharacterPreview::CharacterPreview(MWWorld::Ptr character, int sizeX, int sizeY, const std::string& name,
                                       Ogre::Vector3 position, Ogre::Vector3 lookAt)
        : mSizeX(sizeX)
        , mSizeY(sizeY)
        , mName(name)
        , mPosition(position)
        , mLookAt(lookAt)
        , mCharacter(character)
        , mAnimation(NULL)
    {

    }

    void CharacterPreview::onSetup()
    {

    }

    void CharacterPreview::setup ()
    {
        mSceneMgr = Ogre::Root::getSingleton().createSceneManager(Ogre::ST_GENERIC);

        /// \todo Read the fallback values from INIImporter (Inventory:Directional*)
        Ogre::Light* l = mSceneMgr->createLight();
        l->setType (Ogre::Light::LT_DIRECTIONAL);
        l->setDirection (Ogre::Vector3(0.3, -0.7, 0.3));
        l->setDiffuseColour (Ogre::ColourValue(1,1,1));

        mSceneMgr->setAmbientLight (Ogre::ColourValue(0.5, 0.5, 0.5));

        mCamera = mSceneMgr->createCamera (mName);
        mCamera->setAspectRatio (float(mSizeX) / float(mSizeY));

        Ogre::SceneNode* renderRoot = mSceneMgr->getRootSceneNode()->createChildSceneNode("renderRoot");

        //we do this with mwRoot in renderingManager, do it here too.
        renderRoot->pitch(Ogre::Degree(-90));

        mNode = renderRoot->createChildSceneNode();

        mAnimation = new NpcAnimation(mCharacter, mNode,
            MWWorld::Class::get(mCharacter).getInventoryStore (mCharacter), 0, renderHeadOnly());

        mNode->setVisible (false);

        Ogre::Vector3 scale = mNode->getScale();
        mCamera->setPosition(mPosition * scale);
        mCamera->lookAt(mLookAt * scale);

        mCamera->setNearClipDistance (0.01);
        mCamera->setFarClipDistance (1000);

        mTexture = Ogre::TextureManager::getSingleton().getByName (mName);
        if (mTexture.isNull ())
            mTexture = Ogre::TextureManager::getSingleton().createManual(mName,
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, mSizeX, mSizeY, 0, Ogre::PF_A8R8G8B8, Ogre::TU_RENDERTARGET);

        mRenderTarget = mTexture->getBuffer()->getRenderTarget();
        mRenderTarget->removeAllViewports ();
        mViewport = mRenderTarget->addViewport(mCamera);
        mViewport->setOverlaysEnabled(false);
        mViewport->setBackgroundColour(Ogre::ColourValue(0, 0, 0, 0));
        mViewport->setShadowsEnabled(false);
        mRenderTarget->setActive(true);
        mRenderTarget->setAutoUpdated (false);

        onSetup ();
    }

    CharacterPreview::~CharacterPreview ()
    {
        //Ogre::TextureManager::getSingleton().remove(mName);
        mSceneMgr->destroyCamera (mName);
        delete mAnimation;
        Ogre::Root::getSingleton().destroySceneManager(mSceneMgr);
    }

    void CharacterPreview::rebuild()
    {
        assert(mAnimation);
        delete mAnimation;

        mAnimation = new NpcAnimation(mCharacter, mNode,
            MWWorld::Class::get(mCharacter).getInventoryStore (mCharacter), 0, renderHeadOnly());

        float scale=1.f;
        MWWorld::Class::get(mCharacter).adjustScale(mCharacter, scale);
        mNode->setScale(Ogre::Vector3(scale));

        mNode->setVisible (false);

        mCamera->setPosition(mPosition * mNode->getScale());
        mCamera->lookAt(mLookAt * mNode->getScale());

        onSetup();
    }

    // --------------------------------------------------------------------------------------------------


    InventoryPreview::InventoryPreview(MWWorld::Ptr character)
        : CharacterPreview(character, 512, 1024, "CharacterPreview", Ogre::Vector3(0, 65, -180), Ogre::Vector3(0,65,0))
        , mSelectionBuffer(NULL)
    {
    }

    InventoryPreview::~InventoryPreview()
    {
        delete mSelectionBuffer;
    }

    void InventoryPreview::update(int sizeX, int sizeY)
    {
        mAnimation->forceUpdate();
        mAnimation->runAnimation(0.0f);

        mViewport->setDimensions (0, 0, std::min(1.f, float(sizeX) / float(512)), std::min(1.f, float(sizeY) / float(1024)));

        mNode->setOrientation (Ogre::Quaternion::IDENTITY);

        mNode->setVisible (true);

        mRenderTarget->update();
        mSelectionBuffer->update();

        mNode->setVisible (false);
    }

    int InventoryPreview::getSlotSelected (int posX, int posY)
    {
        return mSelectionBuffer->getSelected (posX, posY);
    }

    void InventoryPreview::onSetup ()
    {
        if (!mSelectionBuffer)
            mSelectionBuffer = new OEngine::Render::SelectionBuffer(mCamera, 512, 1024, 0);

        mAnimation->play("inventoryhandtohand", "start", "stop", false);
    }

    // --------------------------------------------------------------------------------------------------

    RaceSelectionPreview::RaceSelectionPreview()
        : CharacterPreview(MWBase::Environment::get().getWorld()->getPlayer().getPlayer(),
            512, 512, "CharacterHeadPreview", Ogre::Vector3(0, 6, -35), Ogre::Vector3(0,125,0))
        , mRef(&mBase)
    {
        mBase = *mCharacter.get<ESM::NPC>()->mBase;
        mCharacter = MWWorld::Ptr(&mRef, mCharacter.getCell());
    }

    void RaceSelectionPreview::update(float angle)
    {
        mAnimation->runAnimation(0.0f);
        mNode->roll(Ogre::Radian(angle), Ogre::SceneNode::TS_LOCAL);

        updateCamera();

        mNode->setVisible (true);
        mRenderTarget->update();
        mNode->setVisible (false);
    }

    void RaceSelectionPreview::setPrototype(const ESM::NPC &proto)
    {
        mBase = proto;
        mBase.mId = "player";
        rebuild();
        update(0);
    }

    void RaceSelectionPreview::onSetup ()
    {
        mAnimation->play("idle", "start", "stop", false);

        updateCamera();
    }

    void RaceSelectionPreview::updateCamera()
    {
        Ogre::Vector3 scale = mNode->getScale();
        Ogre::Vector3 headOffset = mAnimation->getHeadNode()->_getDerivedPosition();
        headOffset = mNode->convertLocalToWorldPosition(headOffset);

        mCamera->setPosition(headOffset + mPosition * scale);
        mCamera->lookAt(headOffset + mPosition*Ogre::Vector3(0,1,0) * scale);
    }
}
