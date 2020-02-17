#include "characterpreview.hpp"

#include <cmath>

#include <osg/Material>
#include <osg/Fog>
#include <osg/BlendFunc>
#include <osg/Texture2D>
#include <osg/Camera>
#include <osg/PositionAttitudeTransform>
#include <osg/LightModel>
#include <osg/LightSource>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/LineSegmentIntersector>

#include <components/debug/debuglog.hpp>
#include <components/fallback/fallback.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/shadow.hpp>
#include <components/sceneutil/vismask.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/weapontype.hpp"

#include "npcanimation.hpp"

namespace MWRender
{

    class DrawOnceCallback : public osg::NodeCallback
    {
    public:
        DrawOnceCallback ()
            : mRendered(false)
            , mLastRenderedFrame(0)
        {
        }

        virtual void operator () (osg::Node* node, osg::NodeVisitor* nv)
        {
            if (!mRendered)
            {
                mRendered = true;

                mLastRenderedFrame = nv->getTraversalNumber();

                osg::ref_ptr<osg::FrameStamp> previousFramestamp = const_cast<osg::FrameStamp*>(nv->getFrameStamp());
                osg::FrameStamp* fs = new osg::FrameStamp(*previousFramestamp);
                fs->setSimulationTime(0.0);

                nv->setFrameStamp(fs);

                traverse(node, nv);

                nv->setFrameStamp(previousFramestamp);
            }
            else
            {
                node->setNodeMask(SceneUtil::Mask_Disabled);
            }
        }

        void redrawNextFrame()
        {
            mRendered = false;
        }

        unsigned int getLastRenderedFrame() const
        {
            return mLastRenderedFrame;
        }

    private:
        bool mRendered;
        unsigned int mLastRenderedFrame;
    };


    // Set up alpha blending to Additive mode to avoid issues caused by transparent objects writing onto the alpha value of the FBO
    class SetUpBlendVisitor : public osg::NodeVisitor
    {
    public:
        SetUpBlendVisitor(): osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        {
        }

        virtual void apply(osg::Node& node)
        {
            if (osg::StateSet* stateset = node.getStateSet())
            {
                if (stateset->getAttribute(osg::StateAttribute::BLENDFUNC) || stateset->getBinNumber() == osg::StateSet::TRANSPARENT_BIN)
                {
                    osg::ref_ptr<osg::StateSet> newStateSet = new osg::StateSet(*stateset, osg::CopyOp::SHALLOW_COPY);
                    osg::BlendFunc* blendFunc = static_cast<osg::BlendFunc*>(stateset->getAttribute(osg::StateAttribute::BLENDFUNC));
                    osg::ref_ptr<osg::BlendFunc> newBlendFunc = blendFunc ? new osg::BlendFunc(*blendFunc) : new osg::BlendFunc;
                    newBlendFunc->setDestinationAlpha(osg::BlendFunc::ONE);
                    newStateSet->setAttribute(newBlendFunc, osg::StateAttribute::ON);
                    node.setStateSet(newStateSet);
                }

            }
            traverse(node);
        }
    };

    CharacterPreview::CharacterPreview(osg::Group* parent, Resource::ResourceSystem* resourceSystem,
                                       const MWWorld::Ptr& character, int sizeX, int sizeY, const osg::Vec3f& position, const osg::Vec3f& lookAt)
        : mParent(parent)
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
        mCamera->setName("CharacterPreview");
        mCamera->setComputeNearFarMode(osg::Camera::COMPUTE_NEAR_FAR_USING_BOUNDING_VOLUMES);
        mCamera->setCullMask(~(SceneUtil::Mask_UpdateVisitor));

        mCamera->setNodeMask(SceneUtil::Mask_RenderToTexture);

        osg::ref_ptr<SceneUtil::LightManager> lightManager = new SceneUtil::LightManager;
        lightManager->setStartLight(1);
        osg::ref_ptr<osg::StateSet> stateset = lightManager->getOrCreateStateSet();
        stateset->setMode(GL_LIGHTING, osg::StateAttribute::ON);
        stateset->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
        stateset->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
        osg::ref_ptr<osg::Material> defaultMat (new osg::Material);
        defaultMat->setColorMode(osg::Material::OFF);
        defaultMat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,1));
        defaultMat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,1));
        defaultMat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 0.f));
        stateset->setAttribute(defaultMat);

        SceneUtil::ShadowManager::disableShadowsForStateSet(stateset);

        // assign large value to effectively turn off fog
        // shaders don't respect glDisable(GL_FOG)
        osg::ref_ptr<osg::Fog> fog (new osg::Fog);
        fog->setStart(10000000);
        fog->setEnd(10000000);
        stateset->setAttributeAndModes(fog, osg::StateAttribute::OFF|osg::StateAttribute::OVERRIDE);

        osg::ref_ptr<osg::LightModel> lightmodel = new osg::LightModel;
        lightmodel->setAmbientIntensity(osg::Vec4(0.0, 0.0, 0.0, 1.0));
        stateset->setAttributeAndModes(lightmodel, osg::StateAttribute::ON);

        osg::ref_ptr<osg::Light> light = new osg::Light;
        float diffuseR = Fallback::Map::getFloat("Inventory_DirectionalDiffuseR");
        float diffuseG = Fallback::Map::getFloat("Inventory_DirectionalDiffuseG");
        float diffuseB = Fallback::Map::getFloat("Inventory_DirectionalDiffuseB");
        float ambientR = Fallback::Map::getFloat("Inventory_DirectionalAmbientR");
        float ambientG = Fallback::Map::getFloat("Inventory_DirectionalAmbientG");
        float ambientB = Fallback::Map::getFloat("Inventory_DirectionalAmbientB");
        float azimuth = osg::DegreesToRadians(Fallback::Map::getFloat("Inventory_DirectionalRotationX"));
        float altitude = osg::DegreesToRadians(Fallback::Map::getFloat("Inventory_DirectionalRotationY"));
        float positionX = -std::cos(azimuth) * std::sin(altitude);
        float positionY = std::sin(azimuth) * std::sin(altitude);
        float positionZ = std::cos(altitude);
        light->setPosition(osg::Vec4(positionX,positionY,positionZ, 0.0));
        light->setDiffuse(osg::Vec4(diffuseR,diffuseG,diffuseB,1));
        light->setAmbient(osg::Vec4(ambientR,ambientG,ambientB,1));
        light->setSpecular(osg::Vec4(0,0,0,0));
        light->setLightNum(0);
        light->setConstantAttenuation(1.f);
        light->setLinearAttenuation(0.f);
        light->setQuadraticAttenuation(0.f);

        osg::ref_ptr<osg::LightSource> lightSource = new osg::LightSource;
        lightSource->setLight(light);

        lightSource->setStateSetModes(*stateset, osg::StateAttribute::ON);

        lightManager->addChild(lightSource);

        mCamera->addChild(lightManager);

        mNode = new osg::PositionAttitudeTransform;
        lightManager->addChild(mNode);

        mDrawOnceCallback = new DrawOnceCallback;
        mCamera->addUpdateCallback(mDrawOnceCallback);

        mParent->addChild(mCamera);

        mCharacter.mCell = nullptr;
    }

    CharacterPreview::~CharacterPreview ()
    {
        mCamera->removeChildren(0, mCamera->getNumChildren());
        mParent->removeChild(mCamera);
    }

    int CharacterPreview::getTextureWidth() const
    {
        return mSizeX;
    }

    int CharacterPreview::getTextureHeight() const
    {
        return mSizeY;
    }

    void CharacterPreview::setBlendMode()
    {
        SetUpBlendVisitor visitor;
        mNode->accept(visitor);
    }

    void CharacterPreview::onSetup()
    {
        setBlendMode();
    }

    osg::ref_ptr<osg::Texture2D> CharacterPreview::getTexture()
    {
        return mTexture;
    }

    void CharacterPreview::rebuild()
    {
        mAnimation = nullptr;

        mAnimation = new NpcAnimation(mCharacter, mNode, mResourceSystem, true,
                                      (renderHeadOnly() ? NpcAnimation::VM_HeadOnly : NpcAnimation::VM_Normal));

        onSetup();

        redraw();
    }

    void CharacterPreview::redraw()
    {
        mCamera->setNodeMask(SceneUtil::Mask_RenderToTexture);
        mDrawOnceCallback->redrawNextFrame();
    }

    // --------------------------------------------------------------------------------------------------


    InventoryPreview::InventoryPreview(osg::Group* parent, Resource::ResourceSystem* resourceSystem, const MWWorld::Ptr& character)
        : CharacterPreview(parent, resourceSystem, character, 512, 1024, osg::Vec3f(0, 700, 71), osg::Vec3f(0,0,71))
    {
    }

    void InventoryPreview::setViewport(int sizeX, int sizeY)
    {
        sizeX = std::max(sizeX, 0);
        sizeY = std::max(sizeY, 0);

        // NB Camera::setViewport has threading issues
        osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet;
        mViewport = new osg::Viewport(0, mSizeY-sizeY, std::min(mSizeX, sizeX), std::min(mSizeY, sizeY));
        stateset->setAttributeAndModes(mViewport);
        mCamera->setStateSet(stateset);

        redraw();
    }

    void InventoryPreview::update()
    {
        if (!mAnimation.get())
            return;

        mAnimation->showWeapons(true);
        mAnimation->updateParts();

        MWWorld::InventoryStore &inv = mCharacter.getClass().getInventoryStore(mCharacter);
        MWWorld::ContainerStoreIterator iter = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        std::string groupname = "inventoryhandtohand";
        bool showCarriedLeft = true;
        if(iter != inv.end())
        {
            groupname = "inventoryweapononehand";
            if(iter->getTypeName() == typeid(ESM::Weapon).name())
            {
                MWWorld::LiveCellRef<ESM::Weapon> *ref = iter->get<ESM::Weapon>();
                int type = ref->mBase->mData.mType;
                const ESM::WeaponType* weaponInfo = MWMechanics::getWeaponType(type);
                showCarriedLeft = !(weaponInfo->mFlags & ESM::WeaponType::TwoHanded);

                std::string inventoryGroup = weaponInfo->mLongGroup;
                inventoryGroup = "inventory" + inventoryGroup;

                // We still should use one-handed animation as fallback
                if (mAnimation->hasAnimation(inventoryGroup))
                    groupname = inventoryGroup;
                else
                {
                    static const std::string oneHandFallback = "inventory" + MWMechanics::getWeaponType(ESM::Weapon::LongBladeOneHand)->mLongGroup;
                    static const std::string twoHandFallback = "inventory" + MWMechanics::getWeaponType(ESM::Weapon::LongBladeTwoHand)->mLongGroup;

                    // For real two-handed melee weapons use 2h swords animations as fallback, otherwise use the 1h ones
                    if (weaponInfo->mFlags & ESM::WeaponType::TwoHanded && weaponInfo->mWeaponClass == ESM::WeaponType::Melee)
                        groupname = twoHandFallback;
                    else
                        groupname = oneHandFallback;
                }
           }
        }

        mAnimation->showCarriedLeft(showCarriedLeft);

        mCurrentAnimGroup = groupname;
        mAnimation->play(mCurrentAnimGroup, 1, Animation::BlendMask_All, false, 1.0f, "start", "stop", 0.0f, 0);

        MWWorld::ConstContainerStoreIterator torch = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
        if(torch != inv.end() && torch->getTypeName() == typeid(ESM::Light).name() && showCarriedLeft)
        {
            if(!mAnimation->getInfo("torch"))
                mAnimation->play("torch", 2, Animation::BlendMask_LeftArm, false,
                                 1.0f, "start", "stop", 0.0f, ~0ul, true);
        }
        else if(mAnimation->getInfo("torch"))
            mAnimation->disable("torch");

        mAnimation->runAnimation(0.0f);

        setBlendMode();

        redraw();
    }

    int InventoryPreview::getSlotSelected (int posX, int posY)
    {
        if (!mViewport)
            return -1;
        float projX = (posX / mViewport->width()) * 2 - 1.f;
        float projY = (posY / mViewport->height()) * 2 - 1.f;
        // With Intersector::WINDOW, the intersection ratios are slightly inaccurate. Seems to be a
        // precision issue - compiling with OSG_USE_FLOAT_MATRIX=0, Intersector::WINDOW works ok.
        // Using Intersector::PROJECTION results in better precision because the start/end points and the model matrices
        // don't go through as many transformations.
        osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector (new osgUtil::LineSegmentIntersector(osgUtil::Intersector::PROJECTION, projX, projY));

        intersector->setIntersectionLimit(osgUtil::LineSegmentIntersector::LIMIT_NEAREST);
        osgUtil::IntersectionVisitor visitor(intersector);
        visitor.setTraversalMode(osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN);
        // Set the traversal number from the last draw, so that the frame switch used for RigGeometry double buffering works correctly
        visitor.setTraversalNumber(mDrawOnceCallback->getLastRenderedFrame());

        osg::Node::NodeMask nodeMask = mCamera->getNodeMask();
        mCamera->setNodeMask(SceneUtil::Mask_Default);
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
        CharacterPreview::onSetup();
        osg::Vec3f scale (1.f, 1.f, 1.f);
        mCharacter.getClass().adjustScale(mCharacter, scale, true);

        mNode->setScale(scale);

        mCamera->setViewMatrixAsLookAt(mPosition * scale.z(), mLookAt * scale.z(), osg::Vec3f(0,0,1));
    }

    // --------------------------------------------------------------------------------------------------

    RaceSelectionPreview::RaceSelectionPreview(osg::Group* parent, Resource::ResourceSystem* resourceSystem)
        : CharacterPreview(parent, resourceSystem, MWMechanics::getPlayer(),
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
            osg::NodePathList nodepaths = mNodeToFollow->getParentalNodePaths();
            if (nodepaths.empty())
                return;
            osg::Matrix worldMat = osg::computeLocalToWorld(nodepaths[0]);
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
        CharacterPreview::onSetup();
        mAnimation->play("idle", 1, Animation::BlendMask_All, false, 1.0f, "start", "stop", 0.0f, 0);
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
            Log(Debug::Error) << "Error: Bip01 Head node not found";
    }

}
