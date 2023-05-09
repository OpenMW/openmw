#include "renderingmanager.hpp"

#include <cstdlib>
#include <limits>

#include <osg/ClipControl>
#include <osg/ComputeBoundsVisitor>
#include <osg/Fog>
#include <osg/Group>
#include <osg/Light>
#include <osg/LightModel>
#include <osg/Material>
#include <osg/PolygonMode>
#include <osg/UserDataContainer>

#include <osgUtil/LineSegmentIntersector>

#include <osgViewer/Viewer>

#include <components/nifosg/nifloader.hpp>

#include <components/debug/debuglog.hpp>

#include <components/stereo/multiview.hpp>
#include <components/stereo/stereomanager.hpp>

#include <components/resource/imagemanager.hpp>
#include <components/resource/keyframemanager.hpp>
#include <components/resource/resourcesystem.hpp>

#include <components/shader/removedalphafunc.hpp>
#include <components/shader/shadermanager.hpp>

#include <components/settings/settings.hpp>

#include <components/sceneutil/depth.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/rtt.hpp>
#include <components/sceneutil/shadow.hpp>
#include <components/sceneutil/statesetupdater.hpp>
#include <components/sceneutil/visitor.hpp>
#include <components/sceneutil/workqueue.hpp>
#include <components/sceneutil/writescene.hpp>

#include <components/misc/constants.hpp>

#include <components/terrain/quadtreeworld.hpp>
#include <components/terrain/terraingrid.hpp>

#include <components/esm3/loadcell.hpp>
#include <components/esm4/loadcell.hpp>

#include <components/debug/debugdraw.hpp>
#include <components/detournavigator/navigator.hpp>
#include <components/detournavigator/navmeshcacheitem.hpp>

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/groundcoverstore.hpp"

#include "../mwgui/postprocessorhud.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "actorspaths.hpp"
#include "camera.hpp"
#include "effectmanager.hpp"
#include "fogmanager.hpp"
#include "groundcover.hpp"
#include "navmesh.hpp"
#include "npcanimation.hpp"
#include "objectpaging.hpp"
#include "pathgrid.hpp"
#include "postprocessor.hpp"
#include "recastmesh.hpp"
#include "screenshotmanager.hpp"
#include "sky.hpp"
#include "terrainstorage.hpp"
#include "vismask.hpp"
#include "water.hpp"

namespace MWRender
{
    class PerViewUniformStateUpdater final : public SceneUtil::StateSetUpdater
    {
    public:
        PerViewUniformStateUpdater(Resource::SceneManager* sceneManager)
            : mSceneManager(sceneManager)
        {
            mOpaqueTextureUnit = mSceneManager->getShaderManager().reserveGlobalTextureUnits(
                Shader::ShaderManager::Slot::OpaqueDepthTexture);
        }

        void setDefaults(osg::StateSet* stateset) override
        {
            stateset->addUniform(new osg::Uniform("projectionMatrix", osg::Matrixf{}));
            if (mSkyRTT)
                stateset->addUniform(new osg::Uniform("sky", mSkyTextureUnit));
        }

        void apply(osg::StateSet* stateset, osg::NodeVisitor* nv) override
        {
            stateset->getUniform("projectionMatrix")->set(mProjectionMatrix);
            if (mSkyRTT && nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
            {
                osg::Texture* skyTexture = mSkyRTT->getColorTexture(static_cast<osgUtil::CullVisitor*>(nv));
                stateset->setTextureAttribute(
                    mSkyTextureUnit, skyTexture, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            }

            stateset->setTextureAttribute(mOpaqueTextureUnit,
                mSceneManager->getOpaqueDepthTex(nv->getTraversalNumber()), osg::StateAttribute::ON);
        }

        void applyLeft(osg::StateSet* stateset, osgUtil::CullVisitor* nv) override
        {
            stateset->getUniform("projectionMatrix")->set(getEyeProjectionMatrix(0));
        }

        void applyRight(osg::StateSet* stateset, osgUtil::CullVisitor* nv) override
        {
            stateset->getUniform("projectionMatrix")->set(getEyeProjectionMatrix(1));
        }

        void setProjectionMatrix(const osg::Matrixf& projectionMatrix) { mProjectionMatrix = projectionMatrix; }

        const osg::Matrixf& getProjectionMatrix() const { return mProjectionMatrix; }

        void enableSkyRTT(int skyTextureUnit, SceneUtil::RTTNode* skyRTT)
        {
            mSkyTextureUnit = skyTextureUnit;
            mSkyRTT = skyRTT;
        }

    private:
        osg::Matrixf getEyeProjectionMatrix(int view)
        {
            return Stereo::Manager::instance().computeEyeProjection(view, SceneUtil::AutoDepth::isReversed());
        }

        osg::Matrixf mProjectionMatrix;
        int mSkyTextureUnit = -1;
        SceneUtil::RTTNode* mSkyRTT = nullptr;

        Resource::SceneManager* mSceneManager;
        int mOpaqueTextureUnit = -1;
    };

    class SharedUniformStateUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        SharedUniformStateUpdater()
            : mNear(0.f)
            , mFar(0.f)
            , mWindSpeed(0.f)
            , mSkyBlendingStartCoef(Settings::Manager::getFloat("sky blending start", "Fog"))
            , mGroundcoverFadeEnd(Settings::Manager::getFloat("rendering distance", "Groundcover"))
            , mGroundcoverStormDirection(osg::Vec2f(0.f, 0.f))
        {
        }

        void setDefaults(osg::StateSet* stateset) override
        {
            stateset->addUniform(new osg::Uniform("near", 0.f));
            stateset->addUniform(new osg::Uniform("far", 0.f));
            stateset->addUniform(new osg::Uniform("skyBlendingStart", 0.f));
            stateset->addUniform(new osg::Uniform("screenRes", osg::Vec2f{}));
            stateset->addUniform(new osg::Uniform("isReflection", false));
            stateset->addUniform(new osg::Uniform("windSpeed", 0.0f));
            stateset->addUniform(new osg::Uniform("playerPos", osg::Vec3f(0.f, 0.f, 0.f)));
            stateset->addUniform(new osg::Uniform("groundcoverFadeEnd", 0.f));
            stateset->addUniform(new osg::Uniform("stormDir", osg::Vec2f(0.f, 0.f)));
        }

        void apply(osg::StateSet* stateset, osg::NodeVisitor* nv) override
        {
            stateset->getUniform("near")->set(mNear);
            stateset->getUniform("far")->set(mFar);
            stateset->getUniform("skyBlendingStart")->set(mFar * mSkyBlendingStartCoef);
            stateset->getUniform("screenRes")->set(mScreenRes);
            stateset->getUniform("windSpeed")->set(mWindSpeed);
            stateset->getUniform("playerPos")->set(mPlayerPos);
            stateset->getUniform("groundcoverFadeEnd")->set(mGroundcoverFadeEnd);
            stateset->getUniform("stormDir")->set(mGroundcoverStormDirection);
        }

        void setNear(float near) { mNear = near; }

        void setFar(float far) { mFar = far; }

        void setScreenRes(float width, float height) { mScreenRes = osg::Vec2f(width, height); }

        void setWindSpeed(float windSpeed) { mWindSpeed = windSpeed; }

        void setPlayerPos(osg::Vec3f playerPos) { mPlayerPos = playerPos; }

        void setGroundcoverFadeEnd(float fadeEnd) { mGroundcoverFadeEnd = fadeEnd; }

        void setGroundcoverStormDirection(osg::Vec2f direction) { mGroundcoverStormDirection = direction; }

    private:
        float mNear;
        float mFar;
        float mWindSpeed;
        float mSkyBlendingStartCoef;
        osg::Vec3f mPlayerPos;
        osg::Vec2f mScreenRes;
        float mGroundcoverFadeEnd;
        osg::Vec2f mGroundcoverStormDirection;
    };

    class StateUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        StateUpdater()
            : mFogStart(0.f)
            , mFogEnd(0.f)
            , mWireframe(false)
        {
        }

        void setDefaults(osg::StateSet* stateset) override
        {
            osg::LightModel* lightModel = new osg::LightModel;
            stateset->setAttribute(lightModel, osg::StateAttribute::ON);
            osg::Fog* fog = new osg::Fog;
            fog->setMode(osg::Fog::LINEAR);
            stateset->setAttributeAndModes(fog, osg::StateAttribute::ON);
            if (mWireframe)
            {
                osg::PolygonMode* polygonmode = new osg::PolygonMode;
                polygonmode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
                stateset->setAttributeAndModes(polygonmode, osg::StateAttribute::ON);
            }
            else
                stateset->removeAttribute(osg::StateAttribute::POLYGONMODE);
        }

        void apply(osg::StateSet* stateset, osg::NodeVisitor*) override
        {
            osg::LightModel* lightModel
                = static_cast<osg::LightModel*>(stateset->getAttribute(osg::StateAttribute::LIGHTMODEL));
            lightModel->setAmbientIntensity(mAmbientColor);
            osg::Fog* fog = static_cast<osg::Fog*>(stateset->getAttribute(osg::StateAttribute::FOG));
            fog->setColor(mFogColor);
            fog->setStart(mFogStart);
            fog->setEnd(mFogEnd);
        }

        void setAmbientColor(const osg::Vec4f& col) { mAmbientColor = col; }

        void setFogColor(const osg::Vec4f& col) { mFogColor = col; }

        void setFogStart(float start) { mFogStart = start; }

        void setFogEnd(float end) { mFogEnd = end; }

        void setWireframe(bool wireframe)
        {
            if (mWireframe != wireframe)
            {
                mWireframe = wireframe;
                reset();
            }
        }

        bool getWireframe() const { return mWireframe; }

    private:
        osg::Vec4f mAmbientColor;
        osg::Vec4f mFogColor;
        float mFogStart;
        float mFogEnd;
        bool mWireframe;
    };

    class PreloadCommonAssetsWorkItem : public SceneUtil::WorkItem
    {
    public:
        PreloadCommonAssetsWorkItem(Resource::ResourceSystem* resourceSystem)
            : mResourceSystem(resourceSystem)
        {
        }

        void doWork() override
        {
            try
            {
                for (std::vector<std::string>::const_iterator it = mModels.begin(); it != mModels.end(); ++it)
                    mResourceSystem->getSceneManager()->getTemplate(*it);
                for (std::vector<std::string>::const_iterator it = mTextures.begin(); it != mTextures.end(); ++it)
                    mResourceSystem->getImageManager()->getImage(*it);
                for (std::vector<std::string>::const_iterator it = mKeyframes.begin(); it != mKeyframes.end(); ++it)
                    mResourceSystem->getKeyframeManager()->get(*it);
            }
            catch (std::exception&)
            {
                // ignore error (will be shown when these are needed proper)
            }
        }

        std::vector<std::string> mModels;
        std::vector<std::string> mTextures;
        std::vector<std::string> mKeyframes;

    private:
        Resource::ResourceSystem* mResourceSystem;
    };

    RenderingManager::RenderingManager(osgViewer::Viewer* viewer, osg::ref_ptr<osg::Group> rootNode,
        Resource::ResourceSystem* resourceSystem, SceneUtil::WorkQueue* workQueue,
        DetourNavigator::Navigator& navigator, const MWWorld::GroundcoverStore& groundcoverStore,
        SceneUtil::UnrefQueue& unrefQueue)
        : mSkyBlending(Settings::Manager::getBool("sky blending", "Fog"))
        , mViewer(viewer)
        , mRootNode(rootNode)
        , mResourceSystem(resourceSystem)
        , mWorkQueue(workQueue)
        , mNavigator(navigator)
        , mMinimumAmbientLuminance(0.f)
        , mNightEyeFactor(0.f)
        // TODO: Near clip should not need to be bounded like this, but too small values break OSG shadow calculations
        // CPU-side. See issue: #6072
        , mNearClip(std::max(0.005f, Settings::Manager::getFloat("near clip", "Camera")))
        , mViewDistance(Settings::Manager::getFloat("viewing distance", "Camera"))
        , mFieldOfViewOverridden(false)
        , mFieldOfViewOverride(0.f)
        , mFieldOfView(std::clamp(Settings::Manager::getFloat("field of view", "Camera"), 1.f, 179.f))
        , mFirstPersonFieldOfView(
              std::clamp(Settings::Manager::getFloat("first person field of view", "Camera"), 1.f, 179.f))
    {
        bool reverseZ = SceneUtil::AutoDepth::isReversed();
        auto lightingMethod = SceneUtil::LightManager::getLightingMethodFromString(
            Settings::Manager::getString("lighting method", "Shaders"));

        resourceSystem->getSceneManager()->setParticleSystemMask(MWRender::Mask_ParticleSystem);
        // Shadows and radial fog have problems with fixed-function mode.
        bool forceShaders = Settings::Manager::getBool("radial fog", "Fog")
            || Settings::Manager::getBool("exponential fog", "Fog")
            || Settings::Manager::getBool("soft particles", "Shaders")
            || Settings::Manager::getBool("force shaders", "Shaders")
            || Settings::Manager::getBool("enable shadows", "Shadows")
            || lightingMethod != SceneUtil::LightingMethod::FFP || reverseZ || mSkyBlending || Stereo::getMultiview();
        resourceSystem->getSceneManager()->setForceShaders(forceShaders);

        // FIXME: calling dummy method because terrain needs to know whether lighting is clamped
        resourceSystem->getSceneManager()->setClampLighting(Settings::Manager::getBool("clamp lighting", "Shaders"));
        resourceSystem->getSceneManager()->setAutoUseNormalMaps(
            Settings::Manager::getBool("auto use object normal maps", "Shaders"));
        resourceSystem->getSceneManager()->setNormalMapPattern(
            Settings::Manager::getString("normal map pattern", "Shaders"));
        resourceSystem->getSceneManager()->setNormalHeightMapPattern(
            Settings::Manager::getString("normal height map pattern", "Shaders"));
        resourceSystem->getSceneManager()->setAutoUseSpecularMaps(
            Settings::Manager::getBool("auto use object specular maps", "Shaders"));
        resourceSystem->getSceneManager()->setSpecularMapPattern(
            Settings::Manager::getString("specular map pattern", "Shaders"));
        resourceSystem->getSceneManager()->setApplyLightingToEnvMaps(
            Settings::Manager::getBool("apply lighting to environment maps", "Shaders"));
        resourceSystem->getSceneManager()->setConvertAlphaTestToAlphaToCoverage(
            Settings::Manager::getBool("antialias alpha test", "Shaders")
            && Settings::Manager::getInt("antialiasing", "Video") > 1);
        resourceSystem->getSceneManager()->setAdjustCoverageForAlphaTest(
            Settings::Manager::getBool("adjust coverage for alpha test", "Shaders"));

        // Let LightManager choose which backend to use based on our hint. For methods besides legacy lighting, this
        // depends on support for various OpenGL extensions.
        osg::ref_ptr<SceneUtil::LightManager> sceneRoot
            = new SceneUtil::LightManager(lightingMethod == SceneUtil::LightingMethod::FFP);
        resourceSystem->getSceneManager()->setLightingMethod(sceneRoot->getLightingMethod());
        resourceSystem->getSceneManager()->setSupportedLightingMethods(sceneRoot->getSupportedLightingMethods());
        mMinimumAmbientLuminance
            = std::clamp(Settings::Manager::getFloat("minimum interior brightness", "Shaders"), 0.f, 1.f);

        sceneRoot->setLightingMask(Mask_Lighting);
        mSceneRoot = sceneRoot;
        sceneRoot->setStartLight(1);
        sceneRoot->setNodeMask(Mask_Scene);
        sceneRoot->setName("Scene Root");

        int shadowCastingTraversalMask = Mask_Scene;
        if (Settings::Manager::getBool("actor shadows", "Shadows"))
            shadowCastingTraversalMask |= Mask_Actor;
        if (Settings::Manager::getBool("player shadows", "Shadows"))
            shadowCastingTraversalMask |= Mask_Player;

        int indoorShadowCastingTraversalMask = shadowCastingTraversalMask;
        if (Settings::Manager::getBool("object shadows", "Shadows"))
            shadowCastingTraversalMask |= (Mask_Object | Mask_Static);
        if (Settings::Manager::getBool("terrain shadows", "Shadows"))
            shadowCastingTraversalMask |= Mask_Terrain;

        mShadowManager = std::make_unique<SceneUtil::ShadowManager>(sceneRoot, mRootNode, shadowCastingTraversalMask,
            indoorShadowCastingTraversalMask, Mask_Terrain | Mask_Object | Mask_Static,
            mResourceSystem->getSceneManager()->getShaderManager());

        Shader::ShaderManager::DefineMap shadowDefines = mShadowManager->getShadowDefines();
        Shader::ShaderManager::DefineMap lightDefines = sceneRoot->getLightDefines();
        Shader::ShaderManager::DefineMap globalDefines
            = mResourceSystem->getSceneManager()->getShaderManager().getGlobalDefines();

        for (auto itr = shadowDefines.begin(); itr != shadowDefines.end(); itr++)
            globalDefines[itr->first] = itr->second;

        globalDefines["forcePPL"] = Settings::Manager::getBool("force per pixel lighting", "Shaders") ? "1" : "0";
        globalDefines["clamp"] = Settings::Manager::getBool("clamp lighting", "Shaders") ? "1" : "0";
        globalDefines["preLightEnv"]
            = Settings::Manager::getBool("apply lighting to environment maps", "Shaders") ? "1" : "0";
        bool exponentialFog = Settings::Manager::getBool("exponential fog", "Fog");
        globalDefines["radialFog"] = (exponentialFog || Settings::Manager::getBool("radial fog", "Fog")) ? "1" : "0";
        globalDefines["exponentialFog"] = exponentialFog ? "1" : "0";
        globalDefines["skyBlending"] = mSkyBlending ? "1" : "0";
        globalDefines["refraction_enabled"] = "0";
        globalDefines["useGPUShader4"] = "0";
        globalDefines["useOVR_multiview"] = "0";
        globalDefines["numViews"] = "1";
        globalDefines["disableNormals"] = "1";

        for (auto itr = lightDefines.begin(); itr != lightDefines.end(); itr++)
            globalDefines[itr->first] = itr->second;

        // Refactor this at some point - most shaders don't care about these defines
        float groundcoverDistance = std::max(0.f, Settings::Manager::getFloat("rendering distance", "Groundcover"));
        globalDefines["groundcoverFadeStart"] = std::to_string(groundcoverDistance * 0.9f);
        globalDefines["groundcoverFadeEnd"] = std::to_string(groundcoverDistance);
        globalDefines["groundcoverStompMode"]
            = std::to_string(std::clamp(Settings::Manager::getInt("stomp mode", "Groundcover"), 0, 2));
        globalDefines["groundcoverStompIntensity"]
            = std::to_string(std::clamp(Settings::Manager::getInt("stomp intensity", "Groundcover"), 0, 2));

        globalDefines["reverseZ"] = reverseZ ? "1" : "0";

        // It is unnecessary to stop/start the viewer as no frames are being rendered yet.
        mResourceSystem->getSceneManager()->getShaderManager().setGlobalDefines(globalDefines);

        mNavMesh = std::make_unique<NavMesh>(mRootNode, mWorkQueue,
            Settings::Manager::getBool("enable nav mesh render", "Navigator"),
            parseNavMeshMode(Settings::Manager::getString("nav mesh render mode", "Navigator")));
        mActorsPaths = std::make_unique<ActorsPaths>(
            mRootNode, Settings::Manager::getBool("enable agents paths render", "Navigator"));
        mRecastMesh = std::make_unique<RecastMesh>(
            mRootNode, Settings::Manager::getBool("enable recast mesh render", "Navigator"));
        mPathgrid = std::make_unique<Pathgrid>(mRootNode);

        mObjects = std::make_unique<Objects>(mResourceSystem, sceneRoot, unrefQueue);

        if (getenv("OPENMW_DONT_PRECOMPILE") == nullptr)
        {
            mViewer->setIncrementalCompileOperation(new osgUtil::IncrementalCompileOperation);
            mViewer->getIncrementalCompileOperation()->setTargetFrameRate(
                Settings::Manager::getFloat("target framerate", "Cells"));
        }

        mDebugDraw
            = std::make_unique<Debug::DebugDrawer>(mResourceSystem->getSceneManager()->getShaderManager(), mRootNode);
        mResourceSystem->getSceneManager()->setIncrementalCompileOperation(mViewer->getIncrementalCompileOperation());

        mEffectManager = std::make_unique<EffectManager>(sceneRoot, mResourceSystem);

        const std::string& normalMapPattern = Settings::Manager::getString("normal map pattern", "Shaders");
        const std::string& heightMapPattern = Settings::Manager::getString("normal height map pattern", "Shaders");
        const std::string& specularMapPattern = Settings::Manager::getString("terrain specular map pattern", "Shaders");
        const bool useTerrainNormalMaps = Settings::Manager::getBool("auto use terrain normal maps", "Shaders");
        const bool useTerrainSpecularMaps = Settings::Manager::getBool("auto use terrain specular maps", "Shaders");

        mTerrainStorage = std::make_unique<TerrainStorage>(mResourceSystem, normalMapPattern, heightMapPattern,
            useTerrainNormalMaps, specularMapPattern, useTerrainSpecularMaps);
        const float lodFactor = Settings::Manager::getFloat("lod factor", "Terrain");

        bool groundcover = Settings::Manager::getBool("enabled", "Groundcover");
        bool groundcoverPaging = Settings::Manager::getBool("paging", "Groundcover");
        bool distantTerrain = Settings::Manager::getBool("distant terrain", "Terrain");
        if (distantTerrain || groundcover || groundcoverPaging)
        {
            const int compMapResolution = Settings::Manager::getInt("composite map resolution", "Terrain");
            int compMapPower = Settings::Manager::getInt("composite map level", "Terrain");
            compMapPower = std::max(-3, compMapPower);
            float compMapLevel = pow(2, compMapPower);
            const int vertexLodMod = Settings::Manager::getInt("vertex lod mod", "Terrain");
            float maxCompGeometrySize = Settings::Manager::getFloat("max composite geometry size", "Terrain");
            maxCompGeometrySize = std::max(maxCompGeometrySize, 1.f);
            bool debugChunks = Settings::Manager::getBool("debug chunks", "Terrain");
            mTerrain = std::make_unique<Terrain::QuadTreeWorld>(sceneRoot, mRootNode, mResourceSystem,
                mTerrainStorage.get(), Mask_Terrain, Mask_PreCompile, Mask_Debug, compMapResolution, compMapLevel,
                lodFactor, vertexLodMod, maxCompGeometrySize, debugChunks);
            if (Settings::Manager::getBool("object paging", "Terrain"))
            {
                mObjectPaging = std::make_unique<ObjectPaging>(mResourceSystem->getSceneManager(), false, groundcoverStore);
                static_cast<Terrain::QuadTreeWorld*>(mTerrain.get())->addChunkManager(mObjectPaging.get());
                mResourceSystem->addResourceManager(mObjectPaging.get());
            }
        }
        else
            mTerrain = std::make_unique<Terrain::TerrainGrid>(sceneRoot, mRootNode, mResourceSystem,
                mTerrainStorage.get(), Mask_Terrain, Mask_PreCompile, Mask_Debug);

        mTerrain->setTargetFrameRate(Settings::Manager::getFloat("target framerate", "Cells"));

        if (groundcover)
        {
            float density = Settings::Manager::getFloat("density", "Groundcover")/100.f;
            density = std::clamp(density, 0.f, 1.f);

            mGroundcover = std::make_unique<Groundcover>(
                mResourceSystem->getSceneManager(), density, groundcoverDistance, groundcoverStore);
            static_cast<Terrain::QuadTreeWorld*>(mTerrain.get())->addChunkManager(mGroundcover.get());
            mResourceSystem->addResourceManager(mGroundcover.get());
        }

        if (groundcoverPaging)
        {
            osg::ref_ptr<osg::Group> groundcoverRoot = new osg::Group;
            groundcoverRoot->setNodeMask(Mask_Groundcover);
            groundcoverRoot->setName("Groundcover Root");
            sceneRoot->addChild(groundcoverRoot);

            float chunkSize = Settings::Manager::getFloat("min chunk size", "Groundcover");
            if (chunkSize >= 1.0f)
                chunkSize = 1.0f;
            else if (chunkSize >= 0.5f)
                chunkSize = 0.5f;
            else if (chunkSize >= 0.25f)
                chunkSize = 0.25f;
            else if (chunkSize != 0.125f)
                chunkSize = 0.125f;

            mGroundcoverWorld = std::make_unique<Terrain::QuadTreeWorld>(groundcoverRoot, mTerrainStorage.get(), Mask_Groundcover, lodFactor, chunkSize);
            mGroundcoverPaging = std::make_unique<ObjectPaging>(mResourceSystem->getSceneManager(), true, groundcoverStore);
            static_cast<Terrain::QuadTreeWorld*>(mGroundcoverWorld.get())->addChunkManager(mGroundcoverPaging.get());
            mResourceSystem->addResourceManager(mGroundcoverPaging.get());

            mGroundcoverWorld->setActiveGrid(osg::Vec4i(0, 0, 0, 0));
        }

        mStateUpdater = new StateUpdater;
        sceneRoot->addUpdateCallback(mStateUpdater);

        mSharedUniformStateUpdater = new SharedUniformStateUpdater();

        rootNode->addUpdateCallback(mSharedUniformStateUpdater);

        mPerViewUniformStateUpdater = new PerViewUniformStateUpdater(mResourceSystem->getSceneManager());
        rootNode->addCullCallback(mPerViewUniformStateUpdater);

        mPostProcessor = new PostProcessor(*this, viewer, mRootNode, resourceSystem->getVFS());
        resourceSystem->getSceneManager()->setOpaqueDepthTex(
            mPostProcessor->getTexture(PostProcessor::Tex_Depth, 0),
            mPostProcessor->getTexture(PostProcessor::Tex_Depth, 1));
        resourceSystem->getSceneManager()->setSoftParticles(mPostProcessor->softParticlesEnabled());
        resourceSystem->getSceneManager()->setSupportsNormalsRT(mPostProcessor->getSupportsNormalsRT());

        // water goes after terrain for correct waterculling order
        mWater = std::make_unique<Water>(
            sceneRoot->getParent(0), sceneRoot, mResourceSystem, mViewer->getIncrementalCompileOperation());

        mCamera = std::make_unique<Camera>(mViewer->getCamera());

        mScreenshotManager
            = std::make_unique<ScreenshotManager>(viewer, mRootNode, sceneRoot, mResourceSystem, mWater.get());

        mViewer->setLightingMode(osgViewer::View::NO_LIGHT);

        osg::ref_ptr<osg::LightSource> source = new osg::LightSource;
        source->setNodeMask(Mask_Lighting);
        mSunLight = new osg::Light;
        source->setLight(mSunLight);
        mSunLight->setDiffuse(osg::Vec4f(0, 0, 0, 1));
        mSunLight->setAmbient(osg::Vec4f(0, 0, 0, 1));
        mSunLight->setSpecular(osg::Vec4f(0, 0, 0, 0));
        mSunLight->setConstantAttenuation(1.f);
        sceneRoot->setSunlight(mSunLight);
        sceneRoot->addChild(source);

        sceneRoot->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
        sceneRoot->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::ON);
        sceneRoot->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
        osg::ref_ptr<osg::Material> defaultMat(new osg::Material);
        defaultMat->setColorMode(osg::Material::OFF);
        defaultMat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(1, 1, 1, 1));
        defaultMat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(1, 1, 1, 1));
        defaultMat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 0.f));
        sceneRoot->getOrCreateStateSet()->setAttribute(defaultMat);
        sceneRoot->getOrCreateStateSet()->addUniform(new osg::Uniform("emissiveMult", 1.f));
        sceneRoot->getOrCreateStateSet()->addUniform(new osg::Uniform("specStrength", 1.f));

        mFog = std::make_unique<FogManager>();

        mSky = std::make_unique<SkyManager>(
            sceneRoot, mRootNode, mViewer->getCamera(), resourceSystem->getSceneManager(), mSkyBlending);
        if (mSkyBlending)
        {
            int skyTextureUnit = mResourceSystem->getSceneManager()->getShaderManager().reserveGlobalTextureUnits(
                Shader::ShaderManager::Slot::SkyTexture);
            mPerViewUniformStateUpdater->enableSkyRTT(skyTextureUnit, mSky->getSkyRTT());
        }

        source->setStateSetModes(*mRootNode->getOrCreateStateSet(), osg::StateAttribute::ON);

        osg::Camera::CullingMode cullingMode = osg::Camera::DEFAULT_CULLING | osg::Camera::FAR_PLANE_CULLING;

        if (!Settings::Manager::getBool("small feature culling", "Camera"))
            cullingMode &= ~(osg::CullStack::SMALL_FEATURE_CULLING);
        else
        {
            mViewer->getCamera()->setSmallFeatureCullingPixelSize(
                Settings::Manager::getFloat("small feature culling pixel size", "Camera"));
            cullingMode |= osg::CullStack::SMALL_FEATURE_CULLING;
        }

        mViewer->getCamera()->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
        mViewer->getCamera()->setCullingMode(cullingMode);
        mViewer->getCamera()->setName(Constants::SceneCamera);

        auto mask = ~(Mask_UpdateVisitor | Mask_SimpleWater);
        MWBase::Environment::get().getWindowManager()->setCullMask(mask);
        NifOsg::Loader::setHiddenNodeMask(Mask_UpdateVisitor);
        NifOsg::Loader::setIntersectionDisabledNodeMask(Mask_Effect);
        Nif::Reader::setLoadUnsupportedFiles(Settings::Manager::getBool("load unsupported nif files", "Models"));
        Nif::Reader::setWriteNifDebugLog(Settings::Manager::getBool("write nif debug log", "Models"));

        mStateUpdater->setFogEnd(mViewDistance);

        // Hopefully, anything genuinely requiring the default alpha func of GL_ALWAYS explicitly sets it
        mRootNode->getOrCreateStateSet()->setAttribute(Shader::RemovedAlphaFunc::getInstance(GL_ALWAYS));
        // The transparent renderbin sets alpha testing on because that was faster on old GPUs. It's now slower and
        // breaks things.
        mRootNode->getOrCreateStateSet()->setMode(GL_ALPHA_TEST, osg::StateAttribute::OFF);

        if (reverseZ)
        {
            osg::ref_ptr<osg::ClipControl> clipcontrol
                = new osg::ClipControl(osg::ClipControl::LOWER_LEFT, osg::ClipControl::ZERO_TO_ONE);
            mRootNode->getOrCreateStateSet()->setAttributeAndModes(new SceneUtil::AutoDepth, osg::StateAttribute::ON);
            mRootNode->getOrCreateStateSet()->setAttributeAndModes(clipcontrol, osg::StateAttribute::ON);
        }

        SceneUtil::setCameraClearDepth(mViewer->getCamera());

        updateProjectionMatrix();

        mViewer->getCamera()->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }

    RenderingManager::~RenderingManager()
    {
        // let background loading thread finish before we delete anything else
        mWorkQueue = nullptr;
    }

    osgUtil::IncrementalCompileOperation* RenderingManager::getIncrementalCompileOperation()
    {
        return mViewer->getIncrementalCompileOperation();
    }

    MWRender::Objects& RenderingManager::getObjects()
    {
        return *mObjects.get();
    }

    Resource::ResourceSystem* RenderingManager::getResourceSystem()
    {
        return mResourceSystem;
    }

    SceneUtil::WorkQueue* RenderingManager::getWorkQueue()
    {
        return mWorkQueue.get();
    }

    Terrain::World* RenderingManager::getTerrain()
    {
        return mTerrain.get();
    }

    void RenderingManager::preloadCommonAssets()
    {
        osg::ref_ptr<PreloadCommonAssetsWorkItem> workItem(new PreloadCommonAssetsWorkItem(mResourceSystem));
        mSky->listAssetsToPreload(workItem->mModels, workItem->mTextures);
        mWater->listAssetsToPreload(workItem->mTextures);

        workItem->mModels.push_back(Settings::Manager::getString("xbaseanim", "Models"));
        workItem->mModels.push_back(Settings::Manager::getString("xbaseanim1st", "Models"));
        workItem->mModels.push_back(Settings::Manager::getString("xbaseanimfemale", "Models"));
        workItem->mModels.push_back(Settings::Manager::getString("xargonianswimkna", "Models"));

        workItem->mKeyframes.push_back(Settings::Manager::getString("xbaseanimkf", "Models"));
        workItem->mKeyframes.push_back(Settings::Manager::getString("xbaseanim1stkf", "Models"));
        workItem->mKeyframes.push_back(Settings::Manager::getString("xbaseanimfemalekf", "Models"));
        workItem->mKeyframes.push_back(Settings::Manager::getString("xargonianswimknakf", "Models"));

        workItem->mTextures.emplace_back("textures/_land_default.dds");

        mWorkQueue->addWorkItem(std::move(workItem));
    }

    double RenderingManager::getReferenceTime() const
    {
        return mViewer->getFrameStamp()->getReferenceTime();
    }

    SceneUtil::LightManager* RenderingManager::getLightRoot()
    {
        return mSceneRoot.get();
    }

    void RenderingManager::setNightEyeFactor(float factor)
    {
        if (factor != mNightEyeFactor)
        {
            mNightEyeFactor = factor;
            updateAmbient();
        }
    }

    void RenderingManager::setAmbientColour(const osg::Vec4f& colour)
    {
        mAmbientColor = colour;
        updateAmbient();
    }

    void RenderingManager::skySetDate(int day, int month)
    {
        mSky->setDate(day, month);
    }

    int RenderingManager::skyGetMasserPhase() const
    {
        return mSky->getMasserPhase();
    }

    int RenderingManager::skyGetSecundaPhase() const
    {
        return mSky->getSecundaPhase();
    }

    void RenderingManager::skySetMoonColour(bool red)
    {
        mSky->setMoonColour(red);
    }

    void RenderingManager::configureAmbient(const MWWorld::Cell& cell)
    {
        bool isInterior = !cell.isExterior() && !cell.isQuasiExterior();
        bool needsAdjusting = false;
        if (mResourceSystem->getSceneManager()->getLightingMethod() != SceneUtil::LightingMethod::FFP)
            needsAdjusting = isInterior;

        osg::Vec4f ambient = SceneUtil::colourFromRGB(cell.getMood().mAmbiantColor);

        if (needsAdjusting)
        {
            constexpr float pR = 0.2126;
            constexpr float pG = 0.7152;
            constexpr float pB = 0.0722;

            // we already work in linear RGB so no conversions are needed for the luminosity function
            float relativeLuminance = pR * ambient.r() + pG * ambient.g() + pB * ambient.b();
            if (relativeLuminance < mMinimumAmbientLuminance)
            {
                // brighten ambient so it reaches the minimum threshold but no more, we want to mess with content data
                // as least we can
                if (ambient.r() == 0.f && ambient.g() == 0.f && ambient.b() == 0.f)
                    ambient = osg::Vec4(
                        mMinimumAmbientLuminance, mMinimumAmbientLuminance, mMinimumAmbientLuminance, ambient.a());
                else
                    ambient *= mMinimumAmbientLuminance / relativeLuminance;
            }
        }

        setAmbientColour(ambient);

        osg::Vec4f diffuse = SceneUtil::colourFromRGB(cell.getMood().mDirectionalColor);

        setSunColour(diffuse, diffuse, 1.f);

        const osg::Vec4f interiorSunPos = osg::Vec4f(-0.15f, 0.15f, 1.f, 0.f);
        mPostProcessor->getStateUpdater()->setSunPos(interiorSunPos, false);
        mSunLight->setPosition(interiorSunPos);
    }

    void RenderingManager::setSunColour(const osg::Vec4f& diffuse, const osg::Vec4f& specular, float sunVis)
    {
        // need to wrap this in a StateUpdater?
        mSunLight->setDiffuse(diffuse);
        mSunLight->setSpecular(specular);

        mPostProcessor->getStateUpdater()->setSunColor(diffuse);
        mPostProcessor->getStateUpdater()->setSunVis(sunVis);
    }

    void RenderingManager::setSunDirection(const osg::Vec3f& direction)
    {
        osg::Vec3 position = direction * -1;
        // need to wrap this in a StateUpdater?
        mSunLight->setPosition(osg::Vec4(position.x(), position.y(), position.z(), 0));

        mSky->setSunDirection(position);

        mPostProcessor->getStateUpdater()->setSunPos(mSunLight->getPosition(), mNight);
    }

    void RenderingManager::addCell(const MWWorld::CellStore* store)
    {
        mPathgrid->addCell(store);

        mWater->changeCell(store);

        if (store->getCell()->isExterior())
        {
            mTerrain->loadCell(store->getCell()->getGridX(), store->getCell()->getGridY());
            if (mGroundcoverWorld)
                mGroundcoverWorld->loadCell(store->getCell()->getGridX(), store->getCell()->getGridY());
        }
    }
    void RenderingManager::removeCell(const MWWorld::CellStore* store)
    {
        mPathgrid->removeCell(store);
        mActorsPaths->removeCell(store);
        mObjects->removeCell(store);

        if (store->getCell()->isExterior())
        {
            mTerrain->unloadCell(store->getCell()->getGridX(), store->getCell()->getGridY());
            if (mGroundcoverWorld)
                mGroundcoverWorld->unloadCell(store->getCell()->getGridX(), store->getCell()->getGridY());
        }

        mWater->removeCell(store);
    }

    void RenderingManager::enableTerrain(bool enable)
    {
        if (!enable)
            mWater->setCullCallback(nullptr);
        mTerrain->enable(enable);

        if (mGroundcoverWorld)
            mGroundcoverWorld->enable(enable);
    }

    void RenderingManager::setSkyEnabled(bool enabled)
    {
        mSky->setEnabled(enabled);
        if (enabled)
            mShadowManager->enableOutdoorMode();
        else
            mShadowManager->enableIndoorMode();
        mPostProcessor->getStateUpdater()->setIsInterior(!enabled);
    }

    bool RenderingManager::toggleBorders()
    {
        bool borders = !mTerrain->getBordersVisible();
        mTerrain->setBordersVisible(borders);
        return borders;
    }

    bool RenderingManager::toggleRenderMode(RenderMode mode)
    {
        if (mode == Render_CollisionDebug || mode == Render_Pathgrid)
            return mPathgrid->toggleRenderMode(mode);
        else if (mode == Render_Wireframe)
        {
            bool wireframe = !mStateUpdater->getWireframe();
            mStateUpdater->setWireframe(wireframe);
            return wireframe;
        }
        else if (mode == Render_Water)
        {
            return mWater->toggle();
        }
        else if (mode == Render_Scene)
        {
            const auto wm = MWBase::Environment::get().getWindowManager();
            unsigned int mask = wm->getCullMask();
            bool enabled = !(mask & sToggleWorldMask);
            if (enabled)
                mask |= sToggleWorldMask;
            else
                mask &= ~sToggleWorldMask;
            mWater->showWorld(enabled);
            wm->setCullMask(mask);
            return enabled;
        }
        else if (mode == Render_NavMesh)
        {
            return mNavMesh->toggle();
        }
        else if (mode == Render_ActorsPaths)
        {
            return mActorsPaths->toggle();
        }
        else if (mode == Render_RecastMesh)
        {
            return mRecastMesh->toggle();
        }
        return false;
    }

    void RenderingManager::configureFog(const MWWorld::Cell& cell)
    {
        mFog->configure(mViewDistance, cell);
    }

    void RenderingManager::configureFog(
        float fogDepth, float underwaterFog, float dlFactor, float dlOffset, const osg::Vec4f& color)
    {
        mFog->configure(mViewDistance, fogDepth, underwaterFog, dlFactor, dlOffset, color);
    }

    SkyManager* RenderingManager::getSkyManager()
    {
        return mSky.get();
    }

    void RenderingManager::update(float dt, bool paused)
    {
        reportStats();

        mResourceSystem->getSceneManager()->getShaderManager().update(*mViewer);

        float rainIntensity = mSky->getPrecipitationAlpha();
        mWater->setRainIntensity(rainIntensity);

        mWater->update(dt, paused);
        if (!paused)
        {
            mEffectManager->update(dt);
            mSky->update(dt);

            const MWWorld::Ptr& player = mPlayerAnimation->getPtr();
            osg::Vec3f playerPos(player.getRefData().getPosition().asVec3());

            float windSpeed = mSky->getBaseWindSpeed();
            mSharedUniformStateUpdater->setWindSpeed(windSpeed);
            mSharedUniformStateUpdater->setPlayerPos(playerPos);

            osg::Vec2f stormDirection = mSky->getSmoothedStormDirection();
            mSharedUniformStateUpdater->setGroundcoverStormDirection(stormDirection);
        }

        updateNavMesh();
        updateRecastMesh();

        if (mUpdateProjectionMatrix)
        {
            mUpdateProjectionMatrix = false;
            updateProjectionMatrix();
        }
        mCamera->update(dt, paused);

        bool isUnderwater = mWater->isUnderwater(mCamera->getPosition());

        float fogStart = mFog->getFogStart(isUnderwater);
        float fogEnd = mFog->getFogEnd(isUnderwater);
        osg::Vec4f fogColor = mFog->getFogColor(isUnderwater);

        mStateUpdater->setFogStart(fogStart);
        mStateUpdater->setFogEnd(fogEnd);
        setFogColor(fogColor);

        auto world = MWBase::Environment::get().getWorld();
        const auto& stateUpdater = mPostProcessor->getStateUpdater();

        stateUpdater->setFogRange(fogStart, fogEnd);
        stateUpdater->setNearFar(mNearClip, mViewDistance);
        stateUpdater->setIsUnderwater(isUnderwater);
        stateUpdater->setFogColor(fogColor);
        stateUpdater->setGameHour(world->getTimeStamp().getHour());
        stateUpdater->setWeatherId(world->getCurrentWeather());
        stateUpdater->setNextWeatherId(world->getNextWeather());
        stateUpdater->setWeatherTransition(world->getWeatherTransition());
        stateUpdater->setWindSpeed(world->getWindSpeed());
        stateUpdater->setSkyColor(mSky->getSkyColor());
        mPostProcessor->setUnderwaterFlag(isUnderwater);
    }

    void RenderingManager::updatePlayerPtr(const MWWorld::Ptr& ptr)
    {
        if (mPlayerAnimation.get())
        {
            setupPlayer(ptr);
            mPlayerAnimation->updatePtr(ptr);
        }
        mCamera->attachTo(ptr);
    }

    void RenderingManager::removePlayer(const MWWorld::Ptr& player)
    {
        mWater->removeEmitter(player);
    }

    void RenderingManager::rotateObject(const MWWorld::Ptr& ptr, const osg::Quat& rot)
    {
        if (ptr == mCamera->getTrackingPtr() && !mCamera->isVanityOrPreviewModeEnabled())
        {
            mCamera->rotateCameraToTrackingPtr();
        }

        ptr.getRefData().getBaseNode()->setAttitude(rot);
    }

    void RenderingManager::moveObject(const MWWorld::Ptr& ptr, const osg::Vec3f& pos)
    {
        ptr.getRefData().getBaseNode()->setPosition(pos);
    }

    void RenderingManager::scaleObject(const MWWorld::Ptr& ptr, const osg::Vec3f& scale)
    {
        ptr.getRefData().getBaseNode()->setScale(scale);

        if (ptr == mCamera->getTrackingPtr()) // update height of camera
            mCamera->processViewChange();
    }

    void RenderingManager::removeObject(const MWWorld::Ptr& ptr)
    {
        mActorsPaths->remove(ptr);
        mObjects->removeObject(ptr);
        mWater->removeEmitter(ptr);
    }

    void RenderingManager::setWaterEnabled(bool enabled)
    {
        mWater->setEnabled(enabled);
        mSky->setWaterEnabled(enabled);

        mPostProcessor->getStateUpdater()->setIsWaterEnabled(enabled);
    }

    void RenderingManager::setWaterHeight(float height)
    {
        mWater->setCullCallback(mTerrain->getHeightCullCallback(height, Mask_Water));
        mWater->setHeight(height);
        mSky->setWaterHeight(height);

        mPostProcessor->getStateUpdater()->setWaterHeight(height);
    }

    void RenderingManager::screenshot(osg::Image* image, int w, int h)
    {
        mScreenshotManager->screenshot(image, w, h);
    }

    bool RenderingManager::screenshot360(osg::Image* image)
    {
        if (mCamera->isVanityOrPreviewModeEnabled())
        {
            Log(Debug::Warning) << "Spherical screenshots are not allowed in preview mode.";
            return false;
        }

        mScreenshotManager->screenshot360(image);

        return true;
    }

    osg::Vec4f RenderingManager::getScreenBounds(const osg::BoundingBox& worldbb)
    {
        if (!worldbb.valid())
            return osg::Vec4f();
        osg::Matrix viewProj = mViewer->getCamera()->getViewMatrix() * mViewer->getCamera()->getProjectionMatrix();
        float min_x = 1.0f, max_x = 0.0f, min_y = 1.0f, max_y = 0.0f;
        for (int i = 0; i < 8; ++i)
        {
            osg::Vec3f corner = worldbb.corner(i);
            corner = corner * viewProj;

            float x = (corner.x() + 1.f) * 0.5f;
            float y = (corner.y() - 1.f) * (-0.5f);

            if (x < min_x)
                min_x = x;

            if (x > max_x)
                max_x = x;

            if (y < min_y)
                min_y = y;

            if (y > max_y)
                max_y = y;
        }

        return osg::Vec4f(min_x, min_y, max_x, max_y);
    }

    RenderingManager::RayResult getIntersectionResult(osgUtil::LineSegmentIntersector* intersector)
    {
        RenderingManager::RayResult result;
        result.mHit = false;
        result.mRatio = 0;
        if (intersector->containsIntersections())
        {
            result.mHit = true;
            osgUtil::LineSegmentIntersector::Intersection intersection = intersector->getFirstIntersection();

            result.mHitPointWorld = intersection.getWorldIntersectPoint();
            result.mHitNormalWorld = intersection.getWorldIntersectNormal();
            result.mRatio = intersection.ratio;

            PtrHolder* ptrHolder = nullptr;
            std::vector<RefnumMarker*> refnumMarkers;
            for (osg::NodePath::const_iterator it = intersection.nodePath.begin(); it != intersection.nodePath.end();
                 ++it)
            {
                osg::UserDataContainer* userDataContainer = (*it)->getUserDataContainer();
                if (!userDataContainer)
                    continue;
                for (unsigned int i = 0; i < userDataContainer->getNumUserObjects(); ++i)
                {
                    if (PtrHolder* p = dynamic_cast<PtrHolder*>(userDataContainer->getUserObject(i)))
                        ptrHolder = p;
                    if (RefnumMarker* r = dynamic_cast<RefnumMarker*>(userDataContainer->getUserObject(i)))
                        refnumMarkers.push_back(r);
                }
            }

            if (ptrHolder)
                result.mHitObject = ptrHolder->mPtr;

            unsigned int vertexCounter = 0;
            for (unsigned int i = 0; i < refnumMarkers.size(); ++i)
            {
                unsigned int intersectionIndex = intersection.indexList.empty() ? 0 : intersection.indexList[0];
                if (!refnumMarkers[i]->mNumVertices
                    || (intersectionIndex >= vertexCounter
                        && intersectionIndex < vertexCounter + refnumMarkers[i]->mNumVertices))
                {
                    result.mHitRefnum = refnumMarkers[i]->mRefnum;
                    break;
                }
                vertexCounter += refnumMarkers[i]->mNumVertices;
            }
        }

        return result;
    }

    osg::ref_ptr<osgUtil::IntersectionVisitor> RenderingManager::getIntersectionVisitor(
        osgUtil::Intersector* intersector, bool ignorePlayer, bool ignoreActors)
    {
        if (!mIntersectionVisitor)
            mIntersectionVisitor = new osgUtil::IntersectionVisitor;

        mIntersectionVisitor->setTraversalNumber(mViewer->getFrameStamp()->getFrameNumber());
        mIntersectionVisitor->setFrameStamp(mViewer->getFrameStamp());
        mIntersectionVisitor->setIntersector(intersector);

        unsigned int mask = ~0u;
        mask &= ~(Mask_RenderToTexture | Mask_Sky | Mask_Debug | Mask_Effect | Mask_Water | Mask_SimpleWater
            | Mask_Groundcover);
        if (ignorePlayer)
            mask &= ~(Mask_Player);
        if (ignoreActors)
            mask &= ~(Mask_Actor | Mask_Player);

        mIntersectionVisitor->setTraversalMask(mask);
        return mIntersectionVisitor;
    }

    RenderingManager::RayResult RenderingManager::castRay(
        const osg::Vec3f& origin, const osg::Vec3f& dest, bool ignorePlayer, bool ignoreActors)
    {
        osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector(
            new osgUtil::LineSegmentIntersector(osgUtil::LineSegmentIntersector::MODEL, origin, dest));
        intersector->setIntersectionLimit(osgUtil::LineSegmentIntersector::LIMIT_NEAREST);

        mRootNode->accept(*getIntersectionVisitor(intersector, ignorePlayer, ignoreActors));

        return getIntersectionResult(intersector);
    }

    RenderingManager::RayResult RenderingManager::castCameraToViewportRay(
        const float nX, const float nY, float maxDistance, bool ignorePlayer, bool ignoreActors)
    {
        osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector(new osgUtil::LineSegmentIntersector(
            osgUtil::LineSegmentIntersector::PROJECTION, nX * 2.f - 1.f, nY * (-2.f) + 1.f));

        osg::Vec3d dist(0.f, 0.f, -maxDistance);

        dist = dist * mViewer->getCamera()->getProjectionMatrix();

        osg::Vec3d end = intersector->getEnd();
        end.z() = dist.z();
        intersector->setEnd(end);
        intersector->setIntersectionLimit(osgUtil::LineSegmentIntersector::LIMIT_NEAREST);

        mViewer->getCamera()->accept(*getIntersectionVisitor(intersector, ignorePlayer, ignoreActors));

        return getIntersectionResult(intersector);
    }

    void RenderingManager::updatePtr(const MWWorld::Ptr& old, const MWWorld::Ptr& updated)
    {
        mObjects->updatePtr(old, updated);
        mActorsPaths->updatePtr(old, updated);
    }

    void RenderingManager::spawnEffect(const std::string& model, std::string_view texture,
        const osg::Vec3f& worldPosition, float scale, bool isMagicVFX)
    {
        mEffectManager->addEffect(model, texture, worldPosition, scale, isMagicVFX);
    }

    void RenderingManager::notifyWorldSpaceChanged()
    {
        mEffectManager->clear();
        mWater->clearRipples();
    }

    void RenderingManager::clear()
    {
        mSky->setMoonColour(false);

        notifyWorldSpaceChanged();
        if (mObjectPaging)
            mObjectPaging->clear();

        if (mGroundcoverPaging)
            mGroundcoverPaging->clear();
    }

    MWRender::Animation* RenderingManager::getAnimation(const MWWorld::Ptr& ptr)
    {
        if (mPlayerAnimation.get() && ptr == mPlayerAnimation->getPtr())
            return mPlayerAnimation.get();

        return mObjects->getAnimation(ptr);
    }

    const MWRender::Animation* RenderingManager::getAnimation(const MWWorld::ConstPtr& ptr) const
    {
        if (mPlayerAnimation.get() && ptr == mPlayerAnimation->getPtr())
            return mPlayerAnimation.get();

        return mObjects->getAnimation(ptr);
    }

    PostProcessor* RenderingManager::getPostProcessor()
    {
        return mPostProcessor;
    }

    void RenderingManager::setupPlayer(const MWWorld::Ptr& player)
    {
        if (!mPlayerNode)
        {
            mPlayerNode = new SceneUtil::PositionAttitudeTransform;
            mPlayerNode->setNodeMask(Mask_Player);
            mPlayerNode->setName("Player Root");
            mSceneRoot->addChild(mPlayerNode);
        }

        mPlayerNode->setUserDataContainer(new osg::DefaultUserDataContainer);
        mPlayerNode->getUserDataContainer()->addUserObject(new PtrHolder(player));

        player.getRefData().setBaseNode(mPlayerNode);

        mWater->removeEmitter(player);
        mWater->addEmitter(player);
    }

    void RenderingManager::renderPlayer(const MWWorld::Ptr& player)
    {
        mPlayerAnimation = new NpcAnimation(player, player.getRefData().getBaseNode(), mResourceSystem, 0,
            NpcAnimation::VM_Normal, mFirstPersonFieldOfView);

        mCamera->setAnimation(mPlayerAnimation.get());
        mCamera->attachTo(player);
    }

    void RenderingManager::rebuildPtr(const MWWorld::Ptr& ptr)
    {
        NpcAnimation* anim = nullptr;
        if (ptr == mPlayerAnimation->getPtr())
            anim = mPlayerAnimation.get();
        else
            anim = dynamic_cast<NpcAnimation*>(mObjects->getAnimation(ptr));
        if (anim)
        {
            anim->rebuild();
            if (mCamera->getTrackingPtr() == ptr)
            {
                mCamera->attachTo(ptr);
                mCamera->setAnimation(anim);
            }
        }
    }

    void RenderingManager::addWaterRippleEmitter(const MWWorld::Ptr& ptr)
    {
        mWater->addEmitter(ptr);
    }

    void RenderingManager::removeWaterRippleEmitter(const MWWorld::Ptr& ptr)
    {
        mWater->removeEmitter(ptr);
    }

    void RenderingManager::emitWaterRipple(const osg::Vec3f& pos)
    {
        mWater->emitRipple(pos);
    }

    void RenderingManager::updateProjectionMatrix()
    {
        if (mNearClip < 0.0f)
            throw std::runtime_error("Near clip is less than zero");
        if (mViewDistance < mNearClip)
            throw std::runtime_error("Viewing distance is less than near clip");

        double width = Settings::Manager::getInt("resolution x", "Video");
        double height = Settings::Manager::getInt("resolution y", "Video");

        double aspect = (height == 0.0) ? 1.0 : width / height;
        float fov = mFieldOfView;
        if (mFieldOfViewOverridden)
            fov = mFieldOfViewOverride;

        mViewer->getCamera()->setProjectionMatrixAsPerspective(fov, aspect, mNearClip, mViewDistance);

        if (SceneUtil::AutoDepth::isReversed())
        {
            mPerViewUniformStateUpdater->setProjectionMatrix(
                SceneUtil::getReversedZProjectionMatrixAsPerspective(fov, aspect, mNearClip, mViewDistance));
        }
        else
            mPerViewUniformStateUpdater->setProjectionMatrix(mViewer->getCamera()->getProjectionMatrix());

        mSharedUniformStateUpdater->setNear(mNearClip);
        mSharedUniformStateUpdater->setFar(mViewDistance);
        if (Stereo::getStereo())
        {
            auto res = Stereo::Manager::instance().eyeResolution();
            mSharedUniformStateUpdater->setScreenRes(res.x(), res.y());
            Stereo::Manager::instance().setMasterProjectionMatrix(mPerViewUniformStateUpdater->getProjectionMatrix());
        }
        else if (!mPostProcessor->isEnabled())
        {
            mSharedUniformStateUpdater->setScreenRes(width, height);
        }

        // Since our fog is not radial yet, we should take FOV in account, otherwise terrain near viewing distance may
        // disappear. Limit FOV here just for sure, otherwise viewing distance can be too high.
        float distanceMult = std::cos(osg::DegreesToRadians(std::min(fov, 140.f)) / 2.f);
        mTerrain->setViewDistance(mViewDistance * (distanceMult ? 1.f / distanceMult : 1.f));

        if (mGroundcoverWorld)
        {
            float groundcoverDistance = std::max(0.f, Settings::Manager::getFloat("rendering distance", "Groundcover"));
            mGroundcoverWorld->setViewDistance(groundcoverDistance);
        }

        if (mPostProcessor)
        {
            mPostProcessor->getStateUpdater()->setProjectionMatrix(mPerViewUniformStateUpdater->getProjectionMatrix());
            mPostProcessor->getStateUpdater()->setFov(fov);
        }
    }

    void RenderingManager::setScreenRes(int width, int height)
    {
        mSharedUniformStateUpdater->setScreenRes(width, height);
    }

    void RenderingManager::updateTextureFiltering()
    {
        mViewer->stopThreading();

        mResourceSystem->getSceneManager()->setFilterSettings(
            Settings::Manager::getString("texture mag filter", "General"),
            Settings::Manager::getString("texture min filter", "General"),
            Settings::Manager::getString("texture mipmap", "General"),
            Settings::Manager::getInt("anisotropy", "General"));

        mTerrain->updateTextureFiltering();
        mWater->processChangedSettings({});

        mViewer->startThreading();
    }

    void RenderingManager::updateAmbient()
    {
        osg::Vec4f color = mAmbientColor;

        if (mNightEyeFactor > 0.f)
            color += osg::Vec4f(0.7, 0.7, 0.7, 0.0) * mNightEyeFactor;

        mPostProcessor->getStateUpdater()->setAmbientColor(color);
        mStateUpdater->setAmbientColor(color);
    }

    void RenderingManager::setFogColor(const osg::Vec4f& color)
    {
        mViewer->getCamera()->setClearColor(color);

        mStateUpdater->setFogColor(color);
    }

    void RenderingManager::reportStats() const
    {
        osg::Stats* stats = mViewer->getViewerStats();
        unsigned int frameNumber = mViewer->getFrameStamp()->getFrameNumber();
        if (stats->collectStats("resource"))
        {
            mTerrain->reportStats(frameNumber, stats);
        }
    }

    void RenderingManager::processChangedSettings(const Settings::CategorySettingVector& changed)
    {
        // Only perform a projection matrix update once if a relevant setting is changed.
        bool updateProjection = false;

        for (Settings::CategorySettingVector::const_iterator it = changed.begin(); it != changed.end(); ++it)
        {
            if (it->first == "Camera" && it->second == "field of view")
            {
                mFieldOfView = Settings::Manager::getFloat("field of view", "Camera");
                updateProjection = true;
            }
            else if (it->first == "Video" && (it->second == "resolution x" || it->second == "resolution y"))
            {
                updateProjection = true;
            }
            else if (it->first == "Camera" && it->second == "viewing distance")
            {
                setViewDistance(Settings::Manager::getFloat("viewing distance", "Camera"));
            }
            else if (it->first == "Groundcover" && it->second == "rendering distance")
            {
                if (mGroundcoverPaging)
                {
            	    float groundcoverDistance = std::max(0.f, Settings::Manager::getFloat("rendering distance", "Groundcover"));
            	    mGroundcoverWorld->setViewDistance(groundcoverDistance);
                    mSharedUniformStateUpdater->setGroundcoverFadeEnd(groundcoverDistance);
                }
            }
            else if (it->first == "Groundcover" && it->second == "density")
            {
                if (mGroundcoverPaging)
                {
            	    float groundcoverDensity = std::max(0.f, Settings::Manager::getFloat("density", "Groundcover")/100.f);
            	    mGroundcoverPaging->setGroundcoverDensity(groundcoverDensity);
            	    mGroundcoverPaging->clearCache();
                    mGroundcoverWorld->rebuildViews();
                }
            }
            else if (it->first == "General"
                && (it->second == "texture filter" || it->second == "texture mipmap" || it->second == "anisotropy"))
            {
                updateTextureFiltering();
            }
            else if (it->first == "Water")
            {
                mWater->processChangedSettings(changed);
            }
            else if (it->first == "Shaders" && it->second == "minimum interior brightness")
            {
                mMinimumAmbientLuminance
                    = std::clamp(Settings::Manager::getFloat("minimum interior brightness", "Shaders"), 0.f, 1.f);
                if (MWMechanics::getPlayer().isInCell())
                    configureAmbient(*MWMechanics::getPlayer().getCell()->getCell());
            }
            else if (it->first == "Shaders"
                && (it->second == "light bounds multiplier" || it->second == "maximum light distance"
                    || it->second == "light fade start" || it->second == "max lights"))
            {
                auto* lightManager = getLightRoot();
                lightManager->processChangedSettings(changed);

                if (it->second == "max lights" && !lightManager->usingFFP())
                {
//changing make android crash
/*
                    mViewer->stopThreading();

                    lightManager->updateMaxLights();

                    auto defines = mResourceSystem->getSceneManager()->getShaderManager().getGlobalDefines();
                    for (const auto& [name, key] : lightManager->getLightDefines())
                        defines[name] = key;
                    mResourceSystem->getSceneManager()->getShaderManager().setGlobalDefines(defines);

                    mStateUpdater->reset();

                    mViewer->startThreading();
*/
                }
            }
            else if (it->first == "Post Processing" && it->second == "enabled")
            {
                if (Settings::Manager::getBool("enabled", "Post Processing"))
                    mPostProcessor->enable();
                else
                {
                    mPostProcessor->disable();
                    if (auto* hud = MWBase::Environment::get().getWindowManager()->getPostProcessorHud())
                        hud->setVisible(false);
                }
            }
        }

        if (updateProjection)
        {
            updateProjectionMatrix();
        }
    }

    void RenderingManager::setViewDistance(float distance, bool delay)
    {
        mViewDistance = distance;

        if (delay)
        {
            mUpdateProjectionMatrix = true;
            return;
        }

        updateProjectionMatrix();
    }

    float RenderingManager::getTerrainHeightAt(const osg::Vec3f& pos)
    {
        return mTerrain->getHeightAt(pos);
    }

    void RenderingManager::overrideFieldOfView(float val)
    {
        if (mFieldOfViewOverridden != true || mFieldOfViewOverride != val)
        {
            mFieldOfViewOverridden = true;
            mFieldOfViewOverride = val;
            updateProjectionMatrix();
        }
    }

    void RenderingManager::setFieldOfView(float val)
    {
        mFieldOfView = val;
        mUpdateProjectionMatrix = true;
    }

    float RenderingManager::getFieldOfView() const
    {
        return mFieldOfViewOverridden ? mFieldOfViewOverridden : mFieldOfView;
    }

    osg::Vec3f RenderingManager::getHalfExtents(const MWWorld::ConstPtr& object) const
    {
        osg::Vec3f halfExtents(0, 0, 0);
        std::string modelName = object.getClass().getModel(object);
        if (modelName.empty())
            return halfExtents;

        osg::ref_ptr<const osg::Node> node = mResourceSystem->getSceneManager()->getTemplate(modelName);
        osg::ComputeBoundsVisitor computeBoundsVisitor;
        computeBoundsVisitor.setTraversalMask(~(MWRender::Mask_ParticleSystem | MWRender::Mask_Effect));
        const_cast<osg::Node*>(node.get())->accept(computeBoundsVisitor);
        osg::BoundingBox bounds = computeBoundsVisitor.getBoundingBox();

        if (bounds.valid())
        {
            halfExtents[0] = std::abs(bounds.xMax() - bounds.xMin()) / 2.f;
            halfExtents[1] = std::abs(bounds.yMax() - bounds.yMin()) / 2.f;
            halfExtents[2] = std::abs(bounds.zMax() - bounds.zMin()) / 2.f;
        }

        return halfExtents;
    }

    void RenderingManager::resetFieldOfView()
    {
        if (mFieldOfViewOverridden == true)
        {
            mFieldOfViewOverridden = false;

            updateProjectionMatrix();
        }
    }
    void RenderingManager::exportSceneGraph(
        const MWWorld::Ptr& ptr, const std::filesystem::path& filename, const std::string& format)
    {
        osg::Node* node = mViewer->getSceneData();
        if (!ptr.isEmpty())
            node = ptr.getRefData().getBaseNode();

        SceneUtil::writeScene(node, filename, format);
    }

    LandManager* RenderingManager::getLandManager() const
    {
        return mTerrainStorage->getLandManager();
    }

    void RenderingManager::updateActorPath(const MWWorld::ConstPtr& actor, const std::deque<osg::Vec3f>& path,
        const DetourNavigator::AgentBounds& agentBounds, const osg::Vec3f& start, const osg::Vec3f& end) const
    {
        mActorsPaths->update(actor, path, agentBounds, start, end, mNavigator.getSettings());
    }

    void RenderingManager::removeActorPath(const MWWorld::ConstPtr& actor) const
    {
        mActorsPaths->remove(actor);
    }

    void RenderingManager::setNavMeshNumber(const std::size_t value)
    {
        mNavMeshNumber = value;
    }

    void RenderingManager::updateNavMesh()
    {
        if (!mNavMesh->isEnabled())
            return;

        const auto navMeshes = mNavigator.getNavMeshes();

        auto it = navMeshes.begin();
        for (std::size_t i = 0; it != navMeshes.end() && i < mNavMeshNumber; ++i)
            ++it;
        if (it == navMeshes.end())
        {
            mNavMesh->reset();
        }
        else
        {
            try
            {
                mNavMesh->update(it->second, mNavMeshNumber, mNavigator.getSettings());
            }
            catch (const std::exception& e)
            {
                Log(Debug::Error) << "NavMesh render update exception: " << e.what();
            }
        }
    }

    void RenderingManager::updateRecastMesh()
    {
        if (!mRecastMesh->isEnabled())
            return;

        mRecastMesh->update(mNavigator.getRecastMeshTiles(), mNavigator.getSettings());
    }

    void RenderingManager::setActiveGrid(const osg::Vec4i& grid)
    {
        mTerrain->setActiveGrid(grid);
    }
    bool RenderingManager::pagingEnableObject(int type, const MWWorld::ConstPtr& ptr, bool enabled)
    {
        if (!ptr.isInCell() || !ptr.getCell()->isExterior() || !mObjectPaging)
            return false;
        if (mObjectPaging->enableObject(type, ptr.getCellRef().getRefNum(), ptr.getCellRef().getPosition().asVec3(),
                osg::Vec2i(ptr.getCell()->getCell()->getGridX(), ptr.getCell()->getCell()->getGridY()), enabled))
        {
            mTerrain->rebuildViews();
            return true;
        }
        return false;
    }
    void RenderingManager::pagingBlacklistObject(int type, const MWWorld::ConstPtr& ptr)
    {
        if (!ptr.isInCell() || !ptr.getCell()->isExterior() || !mObjectPaging)
            return;
        const ESM::RefNum& refnum = ptr.getCellRef().getRefNum();
        if (!refnum.hasContentFile())
            return;
        if (mObjectPaging->blacklistObject(type, refnum, ptr.getCellRef().getPosition().asVec3(),
                osg::Vec2i(ptr.getCell()->getCell()->getGridX(), ptr.getCell()->getCell()->getGridY())))
            mTerrain->rebuildViews();
    }
    bool RenderingManager::pagingUnlockCache()
    {
        if (mObjectPaging && mObjectPaging->unlockCache())
        {
            mTerrain->rebuildViews();
            return true;
        }

        if (mGroundcoverPaging && mGroundcoverPaging->unlockCache())
        {
            mGroundcoverWorld->rebuildViews();
            return true;
        }

        return false;
    }
    void RenderingManager::getPagedRefnums(const osg::Vec4i& activeGrid, std::vector<ESM::RefNum>& out)
    {
        if (mObjectPaging)
            mObjectPaging->getPagedRefnums(activeGrid, out);
    }

    void RenderingManager::setNavMeshMode(NavMeshMode value)
    {
        mNavMesh->setMode(value);
    }
}
