#include "characterpreview.hpp"

#include <OgreSceneManager.h>
#include <OgreRoot.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreCamera.h>
#include <OgreSceneNode.h>
#include <OgreTextureManager.h>
#include <OgreViewport.h>
#include <OgreRenderTexture.h>

#include <libs/openengine/ogre/selectionbuffer.hpp>


#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"

#include "renderconst.hpp"
#include "npcanimation.hpp"

namespace MWRender
{

    CharacterPreview::CharacterPreview(MWWorld::Ptr character, int sizeX, int sizeY, const std::string& name,
                                       Ogre::Vector3 position, Ogre::Vector3 lookAt)
        : mSceneMgr (0)
        , mPosition(position)
        , mLookAt(lookAt)
        , mCharacter(character)
        , mAnimation(NULL)
        , mName(name)
        , mSizeX(sizeX)
        , mSizeY(sizeY)
        , mRenderTarget(NULL)
        , mViewport(NULL)
        , mCamera(NULL)
        , mNode(NULL)
    {
        mCharacter.mCell = NULL;
    }

    void CharacterPreview::onSetup()
    {

    }

    void CharacterPreview::setup ()
    {
        mSceneMgr = Ogre::Root::getSingleton().createSceneManager(Ogre::ST_GENERIC);

        // This is a dummy light to turn off shadows without having to use a separate set of shaders
        Ogre::Light* l = mSceneMgr->createLight();
        l->setType (Ogre::Light::LT_DIRECTIONAL);
        l->setDiffuseColour (Ogre::ColourValue(0,0,0));

        /// \todo Read the fallback values from INIImporter (Inventory:Directional*)
        l = mSceneMgr->createLight();
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
                                      0, true, (renderHeadOnly() ? NpcAnimation::VM_HeadOnly : NpcAnimation::VM_Normal));

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
        if (mSceneMgr)
        {
            mSceneMgr->destroyAllCameras();
            delete mAnimation;
            Ogre::Root::getSingleton().destroySceneManager(mSceneMgr);
        }
    }

    void CharacterPreview::rebuild()
    {
        assert(mAnimation);
        delete mAnimation;
        mAnimation = new NpcAnimation(mCharacter, mNode,
                                      0, true, (renderHeadOnly() ? NpcAnimation::VM_HeadOnly : NpcAnimation::VM_Normal));

        float scale=1.f;
        mCharacter.getClass().adjustScale(mCharacter, scale);
        mNode->setScale(Ogre::Vector3(scale));

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
        mAnimation->updateParts();

        MWWorld::InventoryStore &inv = mCharacter.getClass().getInventoryStore(mCharacter);
        MWWorld::ContainerStoreIterator iter = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        std::string groupname;
        if(iter == inv.end())
            groupname = "inventoryhandtohand";
        else
        {
            const std::string &type = iter->getTypeName();
            if(type == typeid(ESM::Lockpick).name() || type == typeid(ESM::Probe).name())
                groupname = "inventoryweapononehand";
            else if(type == typeid(ESM::Weapon).name())
            {
                MWWorld::LiveCellRef<ESM::Weapon> *ref = iter->get<ESM::Weapon>();

                int type = ref->mBase->mData.mType;
                if(type == ESM::Weapon::ShortBladeOneHand ||
                   type == ESM::Weapon::LongBladeOneHand ||
                   type == ESM::Weapon::BluntOneHand ||
                   type == ESM::Weapon::AxeOneHand ||
                   type == ESM::Weapon::MarksmanThrown ||
                   type == ESM::Weapon::MarksmanCrossbow ||
                   type == ESM::Weapon::MarksmanBow)
                    groupname = "inventoryweapononehand";
                else if(type == ESM::Weapon::LongBladeTwoHand ||
                        type == ESM::Weapon::BluntTwoClose ||
                        type == ESM::Weapon::AxeTwoHand)
                    groupname = "inventoryweapontwohand";
                else if(type == ESM::Weapon::BluntTwoWide ||
                        type == ESM::Weapon::SpearTwoWide)
                    groupname = "inventoryweapontwowide";
                else
                    groupname = "inventoryhandtohand";
            }
            else
                groupname = "inventoryhandtohand";
        }

        mCurrentAnimGroup = groupname;
        mAnimation->play(mCurrentAnimGroup, 1, Animation::Group_All, false, 1.0f, "start", "stop", 0.0f, 0);

        MWWorld::ContainerStoreIterator torch = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
        if(torch != inv.end() && torch->getTypeName() == typeid(ESM::Light).name())
        {
            if(!mAnimation->getInfo("torch"))
                mAnimation->play("torch", 2, MWRender::Animation::Group_LeftArm, false,
                                 1.0f, "start", "stop", 0.0f, ~0ul);
        }
        else if(mAnimation->getInfo("torch"))
            mAnimation->disable("torch");

        mAnimation->runAnimation(0.0f);

        mViewport->setDimensions (0, 0, std::min(1.f, float(sizeX) / float(512)), std::min(1.f, float(sizeY) / float(1024)));

        mNode->setOrientation (Ogre::Quaternion::IDENTITY);

        mRenderTarget->update();

        mSelectionBuffer->update();
    }

    int InventoryPreview::getSlotSelected (int posX, int posY)
    {
        return mSelectionBuffer->getSelected (posX, posY);
    }

    void InventoryPreview::onSetup ()
    {
        delete mSelectionBuffer;
        mSelectionBuffer = new OEngine::Render::SelectionBuffer(mCamera, 512, 1024, 0);

        mAnimation->showWeapons(true);

        mCurrentAnimGroup = "inventoryhandtohand";
        mAnimation->play(mCurrentAnimGroup, 1, Animation::Group_All, false, 1.0f, "start", "stop", 0.0f, 0);
    }

    // --------------------------------------------------------------------------------------------------

    RaceSelectionPreview::RaceSelectionPreview()
        : CharacterPreview(MWBase::Environment::get().getWorld()->getPlayerPtr(),
            512, 512, "CharacterHeadPreview", Ogre::Vector3(0, 6, -35), Ogre::Vector3(0,125,0))
        , mRef(&mBase)
    {
        mBase = *mCharacter.get<ESM::NPC>()->mBase;
        mCharacter = MWWorld::Ptr(&mRef, NULL);
    }

    void RaceSelectionPreview::update(float angle)
    {
        mAnimation->runAnimation(0.0f);
        mNode->roll(Ogre::Radian(angle), Ogre::SceneNode::TS_LOCAL);

        updateCamera();
    }

    void RaceSelectionPreview::render()
    {
        mRenderTarget->update();
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
        mAnimation->play("idle", 1, Animation::Group_All, false, 1.0f, "start", "stop", 0.0f, 0);

        updateCamera();
    }

    void RaceSelectionPreview::updateCamera()
    {
        Ogre::Vector3 scale = mNode->getScale();
        Ogre::Vector3 headOffset = mAnimation->getNode("Bip01 Head")->_getDerivedPosition();
        headOffset = mNode->convertLocalToWorldPosition(headOffset);

        mCamera->setPosition(headOffset + mPosition * scale);
        mCamera->lookAt(headOffset + mPosition*Ogre::Vector3(0,1,0) * scale);
    }
}
