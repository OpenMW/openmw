#include "characterpreview.hpp"

#include <cmath>

#include <osg/BlendFunc>
#include <osg/Texture2DArray>
#include <osg/Camera>
#include <osg/Fog>
#include <osg/LightModel>
#include <osg/LightSource>
#include <osg/Material>
#include <osg/PositionAttitudeTransform>
#include <osg/TexEnvCombine>
#include <osg/Texture2D>
#include <osg/ValueObject>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/LineSegmentIntersector>

#include <components/debug/debuglog.hpp>
#include <components/fallback/fallback.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/depth.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/nodecallback.hpp>
#include <components/sceneutil/rtt.hpp>
#include <components/sceneutil/shadow.hpp>
#include <components/settings/values.hpp>
#include <components/stereo/multiview.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/weapontype.hpp"

#include "npcanimation.hpp"
#include "util.hpp"
#include "vismask.hpp"

namespace MWRender
{

    class DrawOnceCallback : public SceneUtil::NodeCallback<DrawOnceCallback>
    {
    public:
        DrawOnceCallback(osg::Node* subgraph)
            : mRendered(false)
            , mLastRenderedFrame(0)
            , mSubgraph(subgraph)
        {
        }

        void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            if (!mRendered)
            {
                mRendered = true;

                mLastRenderedFrame = nv->getTraversalNumber();

                osg::ref_ptr<osg::FrameStamp> previousFramestamp = const_cast<osg::FrameStamp*>(nv->getFrameStamp());
                osg::FrameStamp* fs = new osg::FrameStamp(*previousFramestamp);
                fs->setSimulationTime(0.0);

                nv->setFrameStamp(fs);

                // Update keyframe controllers in the scene graph first...
                // RTTNode does not continue update traversal, so manually continue the update traversal since we need
                // it.
                mSubgraph->accept(*nv);
                traverse(node, nv);

                nv->setFrameStamp(previousFramestamp);
            }
            else
            {
                node->setNodeMask(0);
            }
        }

        void redrawNextFrame() { mRendered = false; }

        unsigned int getLastRenderedFrame() const { return mLastRenderedFrame; }

    private:
        bool mRendered;
        unsigned int mLastRenderedFrame;
        osg::ref_ptr<osg::Node> mSubgraph;
    };

    // Set up alpha blending mode to avoid issues caused by transparent objects writing onto the alpha value of the FBO
    // This makes the RTT have premultiplied alpha, though, so the source blend factor must be GL_ONE when it's applied
    class SetUpBlendVisitor : public osg::NodeVisitor
    {
    public:
        SetUpBlendVisitor()
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        {
        }

        void apply(osg::Node& node) override
        {
            if (osg::ref_ptr<osg::StateSet> stateset = node.getStateSet())
            {
                osg::ref_ptr<osg::StateSet> newStateSet;
                if (stateset->getAttribute(osg::StateAttribute::BLENDFUNC)
                    || stateset->getBinNumber() == osg::StateSet::TRANSPARENT_BIN)
                {
                    osg::BlendFunc* blendFunc
                        = static_cast<osg::BlendFunc*>(stateset->getAttribute(osg::StateAttribute::BLENDFUNC));

                    if (blendFunc)
                    {
                        newStateSet = new osg::StateSet(*stateset, osg::CopyOp::SHALLOW_COPY);
                        node.setStateSet(newStateSet);
                        osg::ref_ptr<osg::BlendFunc> newBlendFunc = new osg::BlendFunc(*blendFunc);
                        newStateSet->setAttribute(newBlendFunc, osg::StateAttribute::ON);
                        // I *think* (based on some by-hand maths) that the RGB and dest alpha factors are unchanged,
                        // and only dest determines source alpha factor This has the benefit of being idempotent if we
                        // assume nothing used glBlendFuncSeparate before we touched it
                        if (blendFunc->getDestination() == osg::BlendFunc::ONE_MINUS_SRC_ALPHA)
                            newBlendFunc->setSourceAlpha(osg::BlendFunc::ONE);
                        else if (blendFunc->getDestination() == osg::BlendFunc::ONE)
                            newBlendFunc->setSourceAlpha(osg::BlendFunc::ZERO);
                        // Other setups barely exist in the wild and aren't worth supporting as they're not equippable
                        // gear
                        else
                            Log(Debug::Info) << "Unable to adjust blend mode for character preview. Source factor 0x"
                                             << std::hex << blendFunc->getSource() << ", destination factor 0x"
                                             << blendFunc->getDestination() << std::dec;
                    }
                }
                if (stateset->getMode(GL_BLEND) & osg::StateAttribute::ON)
                {
                    if (!newStateSet)
                    {
                        newStateSet = new osg::StateSet(*stateset, osg::CopyOp::SHALLOW_COPY);
                        node.setStateSet(newStateSet);
                    }
                    // Disable noBlendAlphaEnv
                    newStateSet->setTextureMode(7, GL_TEXTURE_2D, osg::StateAttribute::OFF);
                    newStateSet->setDefine("FORCE_OPAQUE", "0", osg::StateAttribute::ON);
                }
            }
            traverse(node);
        }
    };

    class CharacterPreviewRTTNode : public SceneUtil::RTTNode
    {
        static constexpr float fovYDegrees = 12.3f;
        static constexpr float znear = 4.0f;
        static constexpr float zfar = 10000.f;

    public:
        CharacterPreviewRTTNode(uint32_t sizeX, uint32_t sizeY)
            : RTTNode(sizeX, sizeY, Settings::video().mAntialiasing, false, 0,
                StereoAwareness::Unaware_MultiViewShaders, shouldAddMSAAIntermediateTarget())
            , mAspectRatio(static_cast<float>(sizeX) / static_cast<float>(sizeY))
        {
            if (SceneUtil::AutoDepth::isReversed())
                mPerspectiveMatrix = static_cast<osg::Matrixf>(
                    SceneUtil::getReversedZProjectionMatrixAsPerspective(fovYDegrees, mAspectRatio, znear, zfar));
            else
                mPerspectiveMatrix = osg::Matrixf::perspective(fovYDegrees, mAspectRatio, znear, zfar);
            mGroup->getOrCreateStateSet()->addUniform(new osg::Uniform("projectionMatrix", mPerspectiveMatrix));
            mViewMatrix = osg::Matrixf::identity();
            setColorBufferInternalFormat(GL_RGBA);
            setDepthBufferInternalFormat(GL_DEPTH24_STENCIL8);
        }

        void setDefaults(osg::Camera* camera) override
        {
            camera->setName("CharacterPreview");
            camera->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
            camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT, osg::Camera::PIXEL_BUFFER_RTT);
            camera->setClearColor(osg::Vec4(0.f, 0.f, 0.f, 0.f));
            camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            camera->setProjectionMatrixAsPerspective(fovYDegrees, mAspectRatio, znear, zfar);
            camera->setViewport(0, 0, width(), height());
            camera->setRenderOrder(osg::Camera::PRE_RENDER);
            camera->setCullMask(~(Mask_UpdateVisitor));
            camera->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
            SceneUtil::setCameraClearDepth(camera);

            camera->setNodeMask(Mask_RenderToTexture);
            camera->addChild(mGroup);
        }

        void apply(osg::Camera* camera) override
        {
            if (mCameraStateset)
                camera->setStateSet(mCameraStateset);
            camera->setViewMatrix(mViewMatrix);

            if (shouldDoTextureArray())
                Stereo::setMultiviewMatrices(mGroup->getOrCreateStateSet(), { mPerspectiveMatrix, mPerspectiveMatrix });
        }

        void addChild(osg::Node* node) { mGroup->addChild(node); }

        void setCameraStateset(osg::StateSet* stateset) { mCameraStateset = stateset; }

        void setViewMatrix(const osg::Matrixf& viewMatrix) { mViewMatrix = viewMatrix; }

        osg::ref_ptr<osg::Group> mGroup = new osg::Group;
        osg::Matrixf mPerspectiveMatrix;
        osg::Matrixf mViewMatrix;
        osg::ref_ptr<osg::StateSet> mCameraStateset;
        float mAspectRatio;
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
        mTextureStateSet = new osg::StateSet;
        mTextureStateSet->setAttribute(new osg::BlendFunc(osg::BlendFunc::ONE, osg::BlendFunc::ONE_MINUS_SRC_ALPHA));

        mRTTNode = new CharacterPreviewRTTNode(sizeX, sizeY);
        mRTTNode->setNodeMask(Mask_RenderToTexture);

        osg::ref_ptr<SceneUtil::LightManager> lightManager = new SceneUtil::LightManager(SceneUtil::LightSettings{
            .mLightingMethod = mResourceSystem->getSceneManager()->getLightingMethod(),
            .mMaxLights = Settings::shaders().mMaxLights,
            .mMaximumLightDistance = Settings::shaders().mMaximumLightDistance,
            .mLightFadeStart = Settings::shaders().mLightFadeStart,
            .mLightBoundsMultiplier = Settings::shaders().mLightBoundsMultiplier,
        });
        lightManager->setStartLight(1);
        osg::ref_ptr<osg::StateSet> stateset = lightManager->getOrCreateStateSet();
        stateset->setDefine("FORCE_OPAQUE", "1", osg::StateAttribute::ON);
        stateset->setMode(GL_LIGHTING, osg::StateAttribute::ON);
        stateset->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
        stateset->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
        osg::ref_ptr<osg::Material> defaultMat(new osg::Material);
        defaultMat->setColorMode(osg::Material::OFF);
        defaultMat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(1, 1, 1, 1));
        defaultMat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(1, 1, 1, 1));
        defaultMat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 0.f));
        stateset->setAttribute(defaultMat);

        SceneUtil::ShadowManager::instance().disableShadowsForStateSet(*stateset);

        // assign large value to effectively turn off fog
        // shaders don't respect glDisable(GL_FOG)
        osg::ref_ptr<osg::Fog> fog(new osg::Fog);
        fog->setStart(10000000);
        fog->setEnd(10000000);
        stateset->setAttributeAndModes(fog, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);

        // TODO: Clean up this mess of loose uniforms that shaders depend on.
        // turn off sky blending
        int skyTextureSlot = mResourceSystem->getSceneManager()->getShaderManager().reserveGlobalTextureUnits(Shader::ShaderManager::Slot::SkyTexture);
        stateset->addUniform(new osg::Uniform("far", 10000000.0f));
        stateset->addUniform(new osg::Uniform("skyBlendingStart", 8000000.0f));
        stateset->addUniform(new osg::Uniform("sky", skyTextureSlot));
        stateset->addUniform(new osg::Uniform("screenRes", osg::Vec2f{ 1, 1 }));
        if (Stereo::getMultiview())
        {
            // Multiview needs a texture2DArray dummy texture applied here
            osg::Texture2DArray* textureArray = new osg::Texture2DArray;
            textureArray->setTextureSize(1, 1, 2);
            textureArray->setName("fakeSkyTexture");
            textureArray->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
            textureArray->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
            textureArray->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            textureArray->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            textureArray->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
            stateset->setTextureAttributeAndModes(skyTextureSlot, textureArray, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE | osg::StateAttribute::PROTECTED);
        }

        stateset->addUniform(new osg::Uniform("emissiveMult", 1.f));

        // Opaque stuff must have 1 as its fragment alpha as the FBO is translucent, so having blending off isn't enough
        osg::ref_ptr<osg::TexEnvCombine> noBlendAlphaEnv = new osg::TexEnvCombine();
        noBlendAlphaEnv->setCombine_Alpha(osg::TexEnvCombine::REPLACE);
        noBlendAlphaEnv->setSource0_Alpha(osg::TexEnvCombine::CONSTANT);
        noBlendAlphaEnv->setConstantColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
        noBlendAlphaEnv->setCombine_RGB(osg::TexEnvCombine::REPLACE);
        noBlendAlphaEnv->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
        osg::ref_ptr<osg::Texture2D> dummyTexture = new osg::Texture2D();
        dummyTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        dummyTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        dummyTexture->setInternalFormat(GL_DEPTH_COMPONENT);
        dummyTexture->setTextureSize(1, 1);
        // This might clash with a shadow map, so make sure it doesn't cast shadows
        dummyTexture->setShadowComparison(true);
        dummyTexture->setShadowCompareFunc(osg::Texture::ShadowCompareFunc::ALWAYS);
        stateset->setTextureAttributeAndModes(7, dummyTexture, osg::StateAttribute::ON);
        stateset->setTextureAttribute(7, noBlendAlphaEnv, osg::StateAttribute::ON);

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
        light->setPosition(osg::Vec4(positionX, positionY, positionZ, 0.0));
        light->setDiffuse(osg::Vec4(diffuseR, diffuseG, diffuseB, 1));
        osg::Vec4 ambientRGBA = osg::Vec4(ambientR, ambientG, ambientB, 1);
        if (mResourceSystem->getSceneManager()->getForceShaders())
        {
            // When using shaders, we now skip the ambient sun calculation as this is the only place it's used.
            // Using the scene ambient will give identical results.
            lightmodel->setAmbientIntensity(ambientRGBA);
            light->setAmbient(osg::Vec4(0, 0, 0, 1));
        }
        else
            light->setAmbient(ambientRGBA);
        light->setSpecular(osg::Vec4(0, 0, 0, 0));
        light->setLightNum(0);
        light->setConstantAttenuation(1.f);
        light->setLinearAttenuation(0.f);
        light->setQuadraticAttenuation(0.f);
        lightManager->setSunlight(light);

        osg::ref_ptr<osg::LightSource> lightSource = new osg::LightSource;
        lightSource->setLight(light);

        lightSource->setStateSetModes(*stateset, osg::StateAttribute::ON);

        lightManager->addChild(lightSource);

        mRTTNode->addChild(lightManager);

        mNode = new osg::PositionAttitudeTransform;
        lightManager->addChild(mNode);

        mDrawOnceCallback = new DrawOnceCallback(mRTTNode->mGroup);
        mRTTNode->addUpdateCallback(mDrawOnceCallback);

        mParent->addChild(mRTTNode);

        mCharacter.mCell = nullptr;
    }

    CharacterPreview::~CharacterPreview()
    {
        mParent->removeChild(mRTTNode);
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
        return static_cast<osg::Texture2D*>(mRTTNode->getColorTexture(nullptr));
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
        mRTTNode->setNodeMask(Mask_RenderToTexture);
        mDrawOnceCallback->redrawNextFrame();
    }

    // --------------------------------------------------------------------------------------------------

    InventoryPreview::InventoryPreview(
        osg::Group* parent, Resource::ResourceSystem* resourceSystem, const MWWorld::Ptr& character)
        : CharacterPreview(parent, resourceSystem, character, 512, 1024, osg::Vec3f(0, 700, 71), osg::Vec3f(0, 0, 71))
    {
    }

    void InventoryPreview::setViewport(int sizeX, int sizeY)
    {
        sizeX = std::max(sizeX, 0);
        sizeY = std::max(sizeY, 0);

        // NB Camera::setViewport has threading issues
        osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet;
        mViewport = new osg::Viewport(0, mSizeY - sizeY, std::min(mSizeX, sizeX), std::min(mSizeY, sizeY));
        stateset->setAttributeAndModes(mViewport);
        mRTTNode->setCameraStateset(stateset);

        redraw();
    }

    void InventoryPreview::update()
    {
        if (!mAnimation.get())
            return;

        mAnimation->showWeapons(true);
        mAnimation->updateParts();

        MWWorld::InventoryStore& inv = mCharacter.getClass().getInventoryStore(mCharacter);
        MWWorld::ContainerStoreIterator iter = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        std::string groupname = "inventoryhandtohand";
        bool showCarriedLeft = true;
        if (iter != inv.end())
        {
            groupname = "inventoryweapononehand";
            if (iter->getType() == ESM::Weapon::sRecordId)
            {
                MWWorld::LiveCellRef<ESM::Weapon>* ref = iter->get<ESM::Weapon>();
                int type = ref->mBase->mData.mType;
                const ESM::WeaponType* weaponInfo = MWMechanics::getWeaponType(type);
                showCarriedLeft = !(weaponInfo->mFlags & ESM::WeaponType::TwoHanded);

                std::string inventoryGroup = weaponInfo->mLongGroup;
                inventoryGroup = "inventory" + inventoryGroup;

                // We still should use one-handed animation as fallback
                if (mAnimation->hasAnimation(inventoryGroup))
                    groupname = std::move(inventoryGroup);
                else
                {
                    static const std::string oneHandFallback
                        = "inventory" + MWMechanics::getWeaponType(ESM::Weapon::LongBladeOneHand)->mLongGroup;
                    static const std::string twoHandFallback
                        = "inventory" + MWMechanics::getWeaponType(ESM::Weapon::LongBladeTwoHand)->mLongGroup;

                    // For real two-handed melee weapons use 2h swords animations as fallback, otherwise use the 1h ones
                    if (weaponInfo->mFlags & ESM::WeaponType::TwoHanded
                        && weaponInfo->mWeaponClass == ESM::WeaponType::Melee)
                        groupname = twoHandFallback;
                    else
                        groupname = oneHandFallback;
                }
            }
        }

        mAnimation->showCarriedLeft(showCarriedLeft);

        mCurrentAnimGroup = std::move(groupname);
        mAnimation->play(mCurrentAnimGroup, 1, BlendMask::BlendMask_All, false, 1.0f, "start", "stop", 0.0f, 0);

        MWWorld::ConstContainerStoreIterator torch = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
        if (torch != inv.end() && torch->getType() == ESM::Light::sRecordId && showCarriedLeft)
        {
            if (!mAnimation->getInfo("torch"))
                mAnimation->play("torch", 2, BlendMask::BlendMask_LeftArm, false, 1.0f, "start", "stop", 0.0f,
                    std::numeric_limits<uint32_t>::max(), true);
        }
        else if (mAnimation->getInfo("torch"))
            mAnimation->disable("torch");

        mAnimation->runAnimation(0.0f);

        setBlendMode();

        redraw();
    }

    int InventoryPreview::getSlotSelected(int posX, int posY)
    {
        if (!mViewport)
            return -1;
        float projX = (posX / mViewport->width()) * 2 - 1.f;
        float projY = (posY / mViewport->height()) * 2 - 1.f;
        // With Intersector::WINDOW, the intersection ratios are slightly inaccurate. Seems to be a
        // precision issue - compiling with OSG_USE_FLOAT_MATRIX=0, Intersector::WINDOW works ok.
        // Using Intersector::PROJECTION results in better precision because the start/end points and the model matrices
        // don't go through as many transformations.
        osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector(
            new osgUtil::LineSegmentIntersector(osgUtil::Intersector::PROJECTION, projX, projY));

        intersector->setIntersectionLimit(osgUtil::LineSegmentIntersector::LIMIT_NEAREST);
        osgUtil::IntersectionVisitor visitor(intersector);
        visitor.setTraversalMode(osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN);
        // Set the traversal number from the last draw, so that the frame switch used for RigGeometry double buffering
        // works correctly
        visitor.setTraversalNumber(mDrawOnceCallback->getLastRenderedFrame());

        auto* camera = mRTTNode->getCamera(nullptr);
        osg::Node::NodeMask nodeMask = camera->getNodeMask();
        camera->setNodeMask(~0u);
        camera->accept(visitor);
        camera->setNodeMask(nodeMask);

        if (intersector->containsIntersections())
        {
            osgUtil::LineSegmentIntersector::Intersection intersection = intersector->getFirstIntersection();
            return mAnimation->getSlot(intersection.nodePath);
        }
        return -1;
    }

    void InventoryPreview::updatePtr(const MWWorld::Ptr& ptr)
    {
        mCharacter = MWWorld::Ptr(ptr.getBase(), nullptr);
    }

    void InventoryPreview::onSetup()
    {
        CharacterPreview::onSetup();
        osg::Vec3f scale(1.f, 1.f, 1.f);
        mCharacter.getClass().adjustScale(mCharacter, scale, true);

        mNode->setScale(scale);

        auto viewMatrix = osg::Matrixf::lookAt(mPosition * scale.z(), mLookAt * scale.z(), osg::Vec3f(0, 0, 1));
        mRTTNode->setViewMatrix(viewMatrix);
    }

    // --------------------------------------------------------------------------------------------------

    RaceSelectionPreview::RaceSelectionPreview(osg::Group* parent, Resource::ResourceSystem* resourceSystem)
        : CharacterPreview(
            parent, resourceSystem, MWMechanics::getPlayer(), 512, 512, osg::Vec3f(0, 125, 8), osg::Vec3f(0, 0, 8))
        , mBase(*mCharacter.get<ESM::NPC>()->mBase)
        , mRef(ESM::makeBlankCellRef(), &mBase)
        , mPitchRadians(osg::DegreesToRadians(6.f))
    {
        mCharacter = MWWorld::Ptr(&mRef, nullptr);
    }

    RaceSelectionPreview::~RaceSelectionPreview() {}

    void RaceSelectionPreview::setAngle(float angleRadians)
    {
        mNode->setAttitude(osg::Quat(mPitchRadians, osg::Vec3(1, 0, 0)) * osg::Quat(angleRadians, osg::Vec3(0, 0, 1)));
        redraw();
    }

    void RaceSelectionPreview::setPrototype(const ESM::NPC& proto)
    {
        mBase = proto;
        mBase.mId = ESM::RefId::stringRefId("Player");
        rebuild();
    }

    class UpdateCameraCallback : public SceneUtil::NodeCallback<UpdateCameraCallback, CharacterPreviewRTTNode*>
    {
    public:
        UpdateCameraCallback(
            osg::ref_ptr<const osg::Node> nodeToFollow, const osg::Vec3& posOffset, const osg::Vec3& lookAtOffset)
            : mNodeToFollow(std::move(nodeToFollow))
            , mPosOffset(posOffset)
            , mLookAtOffset(lookAtOffset)
        {
        }

        void operator()(CharacterPreviewRTTNode* node, osg::NodeVisitor* nv)
        {
            // Update keyframe controllers in the scene graph first...
            traverse(node, nv);

            // Now update camera utilizing the updated head position
            osg::NodePathList nodepaths = mNodeToFollow->getParentalNodePaths();
            if (nodepaths.empty())
                return;
            osg::Matrix worldMat = osg::computeLocalToWorld(nodepaths[0]);
            osg::Vec3 headOffset = worldMat.getTrans();

            auto viewMatrix
                = osg::Matrixf::lookAt(headOffset + mPosOffset, headOffset + mLookAtOffset, osg::Vec3(0, 0, 1));
            node->setViewMatrix(viewMatrix);
        }

    private:
        osg::ref_ptr<const osg::Node> mNodeToFollow;
        osg::Vec3 mPosOffset;
        osg::Vec3 mLookAtOffset;
    };

    void RaceSelectionPreview::onSetup()
    {
        CharacterPreview::onSetup();
        mAnimation->play("idle", 1, BlendMask::BlendMask_All, false, 1.0f, "start", "stop", 0.0f, 0);
        mAnimation->runAnimation(0.f);

        // attach camera to follow the head node
        if (mUpdateCameraCallback)
            mRTTNode->removeUpdateCallback(mUpdateCameraCallback);

        const osg::Node* head = mAnimation->getNode("Bip01 Head");
        if (head)
        {
            mUpdateCameraCallback = new UpdateCameraCallback(head, mPosition, mLookAt);
            mRTTNode->addUpdateCallback(mUpdateCameraCallback);
        }
        else
            Log(Debug::Error) << "Error: Bip01 Head node not found";
    }

}
