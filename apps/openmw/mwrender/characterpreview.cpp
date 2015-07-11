#include "characterpreview.hpp"

#include <iostream>

#include <osg/Texture2D>
#include <osg/Camera>
#include <osg/PositionAttitudeTransform>
#include <osgViewer/Viewer>
#include <osg/LightModel>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/LineSegmentIntersector>

#include <components/sceneutil/lightmanager.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"

#include "npcanimation.hpp"
#include "vismask.hpp"

namespace MWRender
{

    class DrawOnceCallback : public osg::NodeCallback
    {
    public:
        DrawOnceCallback ()
            : mRendered(false)
        {
        }

        virtual void operator () (osg::Node* node, osg::NodeVisitor* nv)
        {
            if (!mRendered)
            {
                mRendered = true;
            }
            else
            {
                node->setNodeMask(0);
            }

            traverse(node, nv);
        }

        void redrawNextFrame()
        {
            mRendered = false;
        }

    private:
        bool mRendered;
    };

    CharacterPreview::CharacterPreview(osgViewer::Viewer* viewer, Resource::ResourceSystem* resourceSystem,
                                       MWWorld::Ptr character, int sizeX, int sizeY, const osg::Vec3f& position, const osg::Vec3f& lookAt)
        : mViewer(viewer)
        , mResourceSystem(resourceSystem)
        , mPosition(position)
        , mLookAt(lookAt)
        , mCharacter(character)
        , mAnimation(nullptr)
        , mSizeX(sizeX)
        , mSizeY(sizeY)
    {
        mTexture = new osg::Texture2D;
        mTexture->setTextureSize(sizeX, sizeY);
        mTexture->setInternalFormat(GL_RGBA);
        mTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        mTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

        mCamera = new osg::Camera;
        // hints that the camera is not relative to the master camera
        mCamera->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
        mCamera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT, osg::Camera::PIXEL_BUFFER_RTT);
        mCamera->setClearColor(osg::Vec4(0.f, 0.f, 0.f, 0.f));
        mCamera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const float fovYDegrees = 12.3f;
        mCamera->setProjectionMatrixAsPerspective(fovYDegrees, sizeX/static_cast<float>(sizeY), 0.1f, 10000.f); // zNear and zFar are autocomputed
        mCamera->setViewport(0, 0, sizeX, sizeY);
        mCamera->setRenderOrder(osg::Camera::PRE_RENDER);
        mCamera->attach(osg::Camera::COLOR_BUFFER, mTexture);
        mCamera->setGraphicsContext(mViewer->getCamera()->getGraphicsContext());

        mCamera->setNodeMask(Mask_RenderToTexture);

        osg::ref_ptr<SceneUtil::LightManager> lightManager = new SceneUtil::LightManager;
        lightManager->setStartLight(1);
        osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet;
        stateset->setMode(GL_LIGHTING, osg::StateAttribute::ON);
        stateset->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
        stateset->setMode(GL_CULL_FACE, osg::StateAttribute::ON);

        osg::ref_ptr<osg::LightModel> lightmodel = new osg::LightModel;
        lightmodel->setAmbientIntensity(osg::Vec4(0.25, 0.25, 0.25, 1.0));
        stateset->setAttributeAndModes(lightmodel, osg::StateAttribute::ON);

        /// \todo Read the fallback values from INIImporter (Inventory:Directional*) ?
        osg::ref_ptr<osg::Light> light = new osg::Light;
        light->setPosition(osg::Vec4(-0.3,0.3,0.7, 0.0));
        light->setDiffuse(osg::Vec4(1,1,1,1));
        light->setAmbient(osg::Vec4(0,0,0,1));
        light->setSpecular(osg::Vec4(0,0,0,0));
        light->setLightNum(0);
        light->setConstantAttenuation(1.f);
        light->setLinearAttenuation(0.f);
        light->setQuadraticAttenuation(0.f);

        osg::ref_ptr<osg::LightSource> lightSource = new osg::LightSource;
        lightSource->setLight(light);

        lightSource->setStateSetModes(*stateset, osg::StateAttribute::ON);

        lightManager->setStateSet(stateset);
        lightManager->addChild(lightSource);

        mCamera->addChild(lightManager);

        mNode = new osg::PositionAttitudeTransform;
        lightManager->addChild(mNode);

        mDrawOnceCallback = new DrawOnceCallback;
        mCamera->addUpdateCallback(mDrawOnceCallback);

        mViewer->getSceneData()->asGroup()->addChild(mCamera);

        mCharacter.mCell = nullptr;
    }

    CharacterPreview::~CharacterPreview ()
    {
        mViewer->getSceneData()->asGroup()->removeChild(mCamera);
    }

    int CharacterPreview::getTextureWidth() const
    {
        return mSizeX;
    }

    int CharacterPreview::getTextureHeight() const
    {
        return mSizeY;
    }

    void CharacterPreview::onSetup()
    {
    }

    osg::ref_ptr<osg::Texture2D> CharacterPreview::getTexture()
    {
        return mTexture;
    }

    void CharacterPreview::rebuild()
    {
        delete mAnimation;
        mAnimation = nullptr;

        mAnimation = new NpcAnimation(mCharacter, mNode, mResourceSystem, true, true,
                                      (renderHeadOnly() ? NpcAnimation::VM_HeadOnly : NpcAnimation::VM_Normal));

        onSetup();

        redraw();
    }

    void CharacterPreview::redraw()
    {
        mCamera->setNodeMask(~0);
        mDrawOnceCallback->redrawNextFrame();
    }

    // --------------------------------------------------------------------------------------------------


    InventoryPreview::InventoryPreview(osgViewer::Viewer* viewer, Resource::ResourceSystem* resourceSystem, MWWorld::Ptr character)
        : CharacterPreview(viewer, resourceSystem, character, 512, 1024, osg::Vec3f(0, 700, 71), osg::Vec3f(0,0,71))
    {
    }

    void InventoryPreview::setViewport(int sizeX, int sizeY)
    {
        sizeX = std::max(sizeX, 0);
        sizeY = std::max(sizeY, 0);

        mCamera->setViewport(0, 0, std::min(mSizeX, sizeX), std::min(mSizeY, sizeY));

        redraw();
    }

    void InventoryPreview::update()
    {
        if (!mAnimation)
            return;

        mAnimation->showWeapons(true);
        mAnimation->updateParts();

        MWWorld::InventoryStore &inv = mCharacter.getClass().getInventoryStore(mCharacter);
        MWWorld::ContainerStoreIterator iter = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        std::string groupname;
        bool showCarriedLeft = true;
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

                showCarriedLeft = (iter->getClass().canBeEquipped(*iter, mCharacter).first != 2);
           }
            else
                groupname = "inventoryhandtohand";
        }

        mAnimation->showCarriedLeft(showCarriedLeft);

        mCurrentAnimGroup = groupname;
        mAnimation->play(mCurrentAnimGroup, 1, Animation::Group_All, false, 1.0f, "start", "stop", 0.0f, 0);

        MWWorld::ContainerStoreIterator torch = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
        if(torch != inv.end() && torch->getTypeName() == typeid(ESM::Light).name() && showCarriedLeft)
        {
            if(!mAnimation->getInfo("torch"))
                mAnimation->play("torch", 2, MWRender::Animation::Group_LeftArm, false,
                                 1.0f, "start", "stop", 0.0f, ~0ul, true);
        }
        else if(mAnimation->getInfo("torch"))
            mAnimation->disable("torch");

        mAnimation->runAnimation(0.0f);

        redraw();
    }

    int InventoryPreview::getSlotSelected (int posX, int posY)
    {
        osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector (new osgUtil::LineSegmentIntersector(osgUtil::Intersector::WINDOW, posX, posY));
        intersector->setIntersectionLimit(osgUtil::LineSegmentIntersector::LIMIT_ONE);
        osgUtil::IntersectionVisitor visitor(intersector);

        osg::Node::NodeMask nodeMask = mCamera->getNodeMask();
        mCamera->setNodeMask(~0);
        mCamera->accept(visitor);
        mCamera->setNodeMask(nodeMask);

        if (intersector->containsIntersections())
        {
            osgUtil::LineSegmentIntersector::Intersection intersection = intersector->getFirstIntersection();
            return mAnimation->getSlot(intersection.nodePath);
        }
        return -1;
    }

    void InventoryPreview::updatePtr(const MWWorld::Ptr &ptr)
    {
        mCharacter = MWWorld::Ptr(ptr.getBase(), nullptr);
    }

    void InventoryPreview::onSetup()
    {
        osg::Vec3f scale (1.f, 1.f, 1.f);
        mCharacter.getClass().adjustScale(mCharacter, scale);

        mNode->setScale(scale);

        mCamera->setViewMatrixAsLookAt(mPosition * scale.z(), mLookAt * scale.z(), osg::Vec3f(0,0,1));
    }

    // --------------------------------------------------------------------------------------------------

    RaceSelectionPreview::RaceSelectionPreview(osgViewer::Viewer* viewer, Resource::ResourceSystem* resourceSystem)
        : CharacterPreview(viewer, resourceSystem, MWBase::Environment::get().getWorld()->getPlayerPtr(),
            512, 512, osg::Vec3f(0, 125, 8), osg::Vec3f(0,0,8))
        , mBase (*mCharacter.get<ESM::NPC>()->mBase)
        , mRef(&mBase)
        , mPitchRadians(osg::DegreesToRadians(6.f))
    {
        mCharacter = MWWorld::Ptr(&mRef, nullptr);
    }

    RaceSelectionPreview::~RaceSelectionPreview()
    {
    }

    void RaceSelectionPreview::setAngle(float angleRadians)
    {
        mNode->setAttitude(osg::Quat(mPitchRadians, osg::Vec3(1,0,0))
                * osg::Quat(angleRadians, osg::Vec3(0,0,1)));
        redraw();
    }

    void RaceSelectionPreview::setPrototype(const ESM::NPC &proto)
    {
        mBase = proto;
        mBase.mId = "player";
        rebuild();
    }

    class UpdateCameraCallback : public osg::NodeCallback
    {
    public:
        UpdateCameraCallback(osg::ref_ptr<const osg::Node> nodeToFollow, const osg::Vec3& posOffset, const osg::Vec3& lookAtOffset)
            : mNodeToFollow(nodeToFollow)
            , mPosOffset(posOffset)
            , mLookAtOffset(lookAtOffset)
        {
        }

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            osg::Camera* cam = static_cast<osg::Camera*>(node);

            // Update keyframe controllers in the scene graph first...
            traverse(node, nv);

            // Now update camera utilizing the updated head position
            osg::MatrixList mats = mNodeToFollow->getWorldMatrices();
            if (!mats.size())
                return;
            osg::Matrix worldMat = mats[0];
            osg::Vec3 headOffset = worldMat.getTrans();

            cam->setViewMatrixAsLookAt(headOffset + mPosOffset, headOffset + mLookAtOffset, osg::Vec3(0,0,1));
        }

    private:
        osg::ref_ptr<const osg::Node> mNodeToFollow;
        osg::Vec3 mPosOffset;
        osg::Vec3 mLookAtOffset;
    };

    void RaceSelectionPreview::onSetup ()
    {
        mAnimation->play("idle", 1, Animation::Group_All, false, 1.0f, "start", "stop", 0.0f, 0);
        mAnimation->runAnimation(0.f);

        // attach camera to follow the head node
        if (mUpdateCameraCallback)
            mCamera->removeUpdateCallback(mUpdateCameraCallback);

        const osg::Node* head = mAnimation->getNode("Bip01 Head");
        if (head)
        {
            mUpdateCameraCallback = new UpdateCameraCallback(head, mPosition, mLookAt);
            mCamera->addUpdateCallback(mUpdateCameraCallback);
        }
        else
            std::cerr << "Error: Bip01 Head node not found" << std::endl;
    }

}
