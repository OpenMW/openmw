#include "renderingmanager.hpp"

#include <limits>
#include <cstdlib>

#include <osg/Light>
#include <osg/LightModel>
#include <osg/Fog>
#include <osg/Material>
#include <osg/PolygonMode>
#include <osg/Group>
#include <osg/UserDataContainer>
#include <osg/ComputeBoundsVisitor>
#include <osg/Depth>
#include <osg/ClipControl>

#include <osgUtil/LineSegmentIntersector>

#include <osgViewer/Viewer>

#include <components/nifosg/nifloader.hpp>

#include <components/debug/debuglog.hpp>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/imagemanager.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/resource/keyframemanager.hpp>

#include <components/shader/removedalphafunc.hpp>
#include <components/shader/shadermanager.hpp>

#include <components/settings/settings.hpp>

#include <components/sceneutil/util.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/statesetupdater.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/workqueue.hpp>
#include <components/sceneutil/writescene.hpp>
#include <components/sceneutil/shadow.hpp>

#include <components/terrain/terraingrid.hpp>
#include <components/terrain/quadtreeworld.hpp>

#include <components/esm/loadcell.hpp>

#include <components/detournavigator/navigator.hpp>

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwgui/loadingscreen.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "sky.hpp"
#include "effectmanager.hpp"
#include "npcanimation.hpp"
#include "vismask.hpp"
#include "pathgrid.hpp"
#include "camera.hpp"
#include "viewovershoulder.hpp"
#include "water.hpp"
#include "terrainstorage.hpp"
#include "navmesh.hpp"
#include "actorspaths.hpp"
#include "recastmesh.hpp"
#include "fogmanager.hpp"
#include "objectpaging.hpp"
#include "screenshotmanager.hpp"
#include "groundcover.hpp"
#include "postprocessor.hpp"

namespace MWRender
{
    class SharedUniformStateUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        SharedUniformStateUpdater(bool usePlayerUniforms)
            : mLinearFac(0.f)
            , mNear(0.f)
            , mFar(0.f)
            , mUsePlayerUniforms(usePlayerUniforms)
            , mWindSpeed(0.f)
        {
        }

        void setDefaults(osg::StateSet *stateset) override
        {
            stateset->addUniform(new osg::Uniform("projectionMatrix", osg::Matrixf{}));
            stateset->addUniform(new osg::Uniform("linearFac", 0.f));
            stateset->addUniform(new osg::Uniform("near", 0.f));
            stateset->addUniform(new osg::Uniform("far", 0.f));
            if (mUsePlayerUniforms)
            {
                stateset->addUniform(new osg::Uniform("windSpeed", 0.0f));
                stateset->addUniform(new osg::Uniform("playerPos", osg::Vec3f(0.f, 0.f, 0.f)));
            }
        }

        void apply(osg::StateSet* stateset, osg::NodeVisitor* nv) override
        {
            auto* uProjectionMatrix = stateset->getUniform("projectionMatrix");
            if (uProjectionMatrix)
                uProjectionMatrix->set(mProjectionMatrix);

            auto* uLinearFac = stateset->getUniform("linearFac");
            if (uLinearFac)
                uLinearFac->set(mLinearFac);

            auto* uNear = stateset->getUniform("near");
            if (uNear)
                uNear->set(mNear);

            auto* uFar = stateset->getUniform("far");
            if (uFar)
                uFar->set(mFar);

            if (mUsePlayerUniforms)
            {
                auto* windSpeed = stateset->getUniform("windSpeed");
                if (windSpeed)
                    windSpeed->set(mWindSpeed);

                auto* playerPos = stateset->getUniform("playerPos");
                if (playerPos)
                    playerPos->set(mPlayerPos);
            }
        }

        void setProjectionMatrix(const osg::Matrixf& projectionMatrix)
        {
            mProjectionMatrix = projectionMatrix;
        }

        void setLinearFac(float linearFac)
        {
            mLinearFac = linearFac;
        }

        void setNear(float near)
        {
            mNear = near;
        }

        void setFar(float far)
        {
            mFar = far;
        }

        void setWindSpeed(float windSpeed)
        {
            mWindSpeed = windSpeed;
        }

        void setPlayerPos(osg::Vec3f playerPos)
        {
            mPlayerPos = playerPos;
        }


    private:
        osg::Matrixf mProjectionMatrix;
        float mLinearFac;
        float mNear;
        float mFar;
        bool mUsePlayerUniforms;
        float mWindSpeed;
        osg::Vec3f mPlayerPos;
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

        void setDefaults(osg::StateSet *stateset) override
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
            osg::LightModel* lightModel = static_cast<osg::LightModel*>(stateset->getAttribute(osg::StateAttribute::LIGHTMODEL));
            lightModel->setAmbientIntensity(mAmbientColor);
            osg::Fog* fog = static_cast<osg::Fog*>(stateset->getAttribute(osg::StateAttribute::FOG));
            fog->setColor(mFogColor);
            fog->setStart(mFogStart);
            fog->setEnd(mFogEnd);
        }

        void setAmbientColor(const osg::Vec4f& col)
        {
            mAmbientColor = col;
        }

        void setFogColor(const osg::Vec4f& col)
        {
            mFogColor = col;
        }

        void setFogStart(float start)
        {
            mFogStart = start;
        }

        void setFogEnd(float end)
        {
            mFogEnd = end;
        }

        void setWireframe(bool wireframe)
        {
            if (mWireframe != wireframe)
            {
                mWireframe = wireframe;
                reset();
            }
        }

        bool getWireframe() const
        {
            return mWireframe;
        }

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
                                       const std::string& resourcePath, DetourNavigator::Navigator& navigator)
        : mViewer(viewer)
        , mRootNode(rootNode)
        , mResourceSystem(resourceSystem)
        , mWorkQueue(workQueue)
        , mNavigator(navigator)
        , mMinimumAmbientLuminance(0.f)
        , mNightEyeFactor(0.f)
        // TODO: Near clip should not need to be bounded like this, but too small values break OSG shadow calculations CPU-side.
        // See issue: #6072
        , mNearClip(std::max(0.005f, Settings::Manager::getFloat("near clip", "Camera")))
        , mViewDistance(Settings::Manager::getFloat("viewing distance", "Camera"))
        , mFieldOfViewOverridden(false)
        , mFieldOfViewOverride(0.f)
        , mFieldOfView(std::min(std::max(1.f, Settings::Manager::getFloat("field of view", "Camera")), 179.f))
    {
        bool reverseZ = SceneUtil::getReverseZ();

        if (reverseZ)
            Log(Debug::Info) << "Using reverse-z depth buffer";
        else
            Log(Debug::Info) << "Using standard depth buffer";

        auto lightingMethod = SceneUtil::LightManager::getLightingMethodFromString(Settings::Manager::getString("lighting method", "Shaders"));

        resourceSystem->getSceneManager()->setParticleSystemMask(MWRender::Mask_ParticleSystem);
        // Shadows and radial fog have problems with fixed-function mode
        bool forceShaders = Settings::Manager::getBool("radial fog", "Shaders")
                            || Settings::Manager::getBool("force shaders", "Shaders")
                            || Settings::Manager::getBool("enable shadows", "Shadows")
                            || lightingMethod != SceneUtil::LightingMethod::FFP
                            || reverseZ;
        resourceSystem->getSceneManager()->setForceShaders(forceShaders);
        // FIXME: calling dummy method because terrain needs to know whether lighting is clamped
        resourceSystem->getSceneManager()->setClampLighting(Settings::Manager::getBool("clamp lighting", "Shaders"));
        resourceSystem->getSceneManager()->setAutoUseNormalMaps(Settings::Manager::getBool("auto use object normal maps", "Shaders"));
        resourceSystem->getSceneManager()->setNormalMapPattern(Settings::Manager::getString("normal map pattern", "Shaders"));
        resourceSystem->getSceneManager()->setNormalHeightMapPattern(Settings::Manager::getString("normal height map pattern", "Shaders"));
        resourceSystem->getSceneManager()->setAutoUseSpecularMaps(Settings::Manager::getBool("auto use object specular maps", "Shaders"));
        resourceSystem->getSceneManager()->setSpecularMapPattern(Settings::Manager::getString("specular map pattern", "Shaders"));
        resourceSystem->getSceneManager()->setApplyLightingToEnvMaps(Settings::Manager::getBool("apply lighting to environment maps", "Shaders"));
        resourceSystem->getSceneManager()->setConvertAlphaTestToAlphaToCoverage(Settings::Manager::getBool("antialias alpha test", "Shaders") && Settings::Manager::getInt("antialiasing", "Video") > 1);

        // Let LightManager choose which backend to use based on our hint. For methods besides legacy lighting, this depends on support for various OpenGL extensions.
        osg::ref_ptr<SceneUtil::LightManager> sceneRoot = new SceneUtil::LightManager(lightingMethod == SceneUtil::LightingMethod::FFP);
        resourceSystem->getSceneManager()->setLightingMethod(sceneRoot->getLightingMethod());
        resourceSystem->getSceneManager()->setSupportedLightingMethods(sceneRoot->getSupportedLightingMethods());
        mMinimumAmbientLuminance = std::clamp(Settings::Manager::getFloat("minimum interior brightness", "Shaders"), 0.f, 1.f);

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
            shadowCastingTraversalMask |= (Mask_Object|Mask_Static);
        if (Settings::Manager::getBool("terrain shadows", "Shadows"))
            shadowCastingTraversalMask |= Mask_Terrain;

        mShadowManager.reset(new SceneUtil::ShadowManager(sceneRoot, mRootNode, shadowCastingTraversalMask, indoorShadowCastingTraversalMask, Mask_Terrain|Mask_Object|Mask_Static, mResourceSystem->getSceneManager()->getShaderManager()));

        Shader::ShaderManager::DefineMap shadowDefines = mShadowManager->getShadowDefines();
        Shader::ShaderManager::DefineMap lightDefines = sceneRoot->getLightDefines();
        Shader::ShaderManager::DefineMap globalDefines = mResourceSystem->getSceneManager()->getShaderManager().getGlobalDefines();

        for (auto itr = shadowDefines.begin(); itr != shadowDefines.end(); itr++)
            globalDefines[itr->first] = itr->second;

        globalDefines["forcePPL"] = Settings::Manager::getBool("force per pixel lighting", "Shaders") ? "1" : "0";
        globalDefines["clamp"] = Settings::Manager::getBool("clamp lighting", "Shaders") ? "1" : "0";
        globalDefines["preLightEnv"] = Settings::Manager::getBool("apply lighting to environment maps", "Shaders") ? "1" : "0";
        globalDefines["radialFog"] = Settings::Manager::getBool("radial fog", "Shaders") ? "1" : "0";
        globalDefines["useGPUShader4"] = "0";

        for (auto itr = lightDefines.begin(); itr != lightDefines.end(); itr++)
            globalDefines[itr->first] = itr->second;

        // Refactor this at some point - most shaders don't care about these defines
        float groundcoverDistance = std::max(0.f, Settings::Manager::getFloat("rendering distance", "Groundcover"));
        globalDefines["groundcoverFadeStart"] = std::to_string(groundcoverDistance * 0.9f);
        globalDefines["groundcoverFadeEnd"] = std::to_string(groundcoverDistance);
        globalDefines["groundcoverStompMode"] = std::to_string(std::clamp(Settings::Manager::getInt("stomp mode", "Groundcover"), 0, 2));
        globalDefines["groundcoverStompIntensity"] = std::to_string(std::clamp(Settings::Manager::getInt("stomp intensity", "Groundcover"), 0, 2));

        globalDefines["reverseZ"] = reverseZ ? "1" : "0";

        // It is unnecessary to stop/start the viewer as no frames are being rendered yet.
        mResourceSystem->getSceneManager()->getShaderManager().setGlobalDefines(globalDefines);

        mNavMesh.reset(new NavMesh(mRootNode, Settings::Manager::getBool("enable nav mesh render", "Navigator")));
        mActorsPaths.reset(new ActorsPaths(mRootNode, Settings::Manager::getBool("enable agents paths render", "Navigator")));
        mRecastMesh.reset(new RecastMesh(mRootNode, Settings::Manager::getBool("enable recast mesh render", "Navigator")));
        mPathgrid.reset(new Pathgrid(mRootNode));

        mObjects.reset(new Objects(mResourceSystem, sceneRoot));

        if (getenv("OPENMW_DONT_PRECOMPILE") == nullptr)
        {
            mViewer->setIncrementalCompileOperation(new osgUtil::IncrementalCompileOperation);
            mViewer->getIncrementalCompileOperation()->setTargetFrameRate(Settings::Manager::getFloat("target framerate", "Cells"));
        }

        mResourceSystem->getSceneManager()->setIncrementalCompileOperation(mViewer->getIncrementalCompileOperation());

        mEffectManager.reset(new EffectManager(sceneRoot, mResourceSystem));

        const std::string normalMapPattern = Settings::Manager::getString("normal map pattern", "Shaders");
        const std::string heightMapPattern = Settings::Manager::getString("normal height map pattern", "Shaders");
        const std::string specularMapPattern = Settings::Manager::getString("terrain specular map pattern", "Shaders");
        const bool useTerrainNormalMaps = Settings::Manager::getBool("auto use terrain normal maps", "Shaders");
        const bool useTerrainSpecularMaps = Settings::Manager::getBool("auto use terrain specular maps", "Shaders");

        mTerrainStorage.reset(new TerrainStorage(mResourceSystem, normalMapPattern, heightMapPattern, useTerrainNormalMaps, specularMapPattern, useTerrainSpecularMaps));
        const float lodFactor = Settings::Manager::getFloat("lod factor", "Terrain");

        bool groundcover = Settings::Manager::getBool("enabled", "Groundcover");
        bool distantTerrain = Settings::Manager::getBool("distant terrain", "Terrain");
        if (distantTerrain || groundcover)
        {
            const int compMapResolution = Settings::Manager::getInt("composite map resolution", "Terrain");
            int compMapPower = Settings::Manager::getInt("composite map level", "Terrain");
            compMapPower = std::max(-3, compMapPower);
            float compMapLevel = pow(2, compMapPower);
            const int vertexLodMod = Settings::Manager::getInt("vertex lod mod", "Terrain");
            float maxCompGeometrySize = Settings::Manager::getFloat("max composite geometry size", "Terrain");
            maxCompGeometrySize = std::max(maxCompGeometrySize, 1.f);
            bool debugChunks = Settings::Manager::getBool("debug chunks", "Terrain");
            mTerrain.reset(new Terrain::QuadTreeWorld(
                sceneRoot, mRootNode, mResourceSystem, mTerrainStorage.get(), Mask_Terrain, Mask_PreCompile, Mask_Debug,
                compMapResolution, compMapLevel, lodFactor, vertexLodMod, maxCompGeometrySize, debugChunks));
            if (Settings::Manager::getBool("object paging", "Terrain"))
            {
                mObjectPaging.reset(new ObjectPaging(mResourceSystem->getSceneManager()));
                static_cast<Terrain::QuadTreeWorld*>(mTerrain.get())->addChunkManager(mObjectPaging.get());
                mResourceSystem->addResourceManager(mObjectPaging.get());
            }
        }
        else
            mTerrain.reset(new Terrain::TerrainGrid(sceneRoot, mRootNode, mResourceSystem, mTerrainStorage.get(), Mask_Terrain, Mask_PreCompile, Mask_Debug));

        mTerrain->setTargetFrameRate(Settings::Manager::getFloat("target framerate", "Cells"));

        if (groundcover)
        {
            float density = Settings::Manager::getFloat("density", "Groundcover");
            density = std::clamp(density, 0.f, 1.f);

            mGroundcover.reset(new Groundcover(mResourceSystem->getSceneManager(), density));
            static_cast<Terrain::QuadTreeWorld*>(mTerrain.get())->addChunkManager(mGroundcover.get());
            mResourceSystem->addResourceManager(mGroundcover.get());

            mGroundcover->setViewDistance(groundcoverDistance);
        }

        mStateUpdater = new StateUpdater;
        sceneRoot->addUpdateCallback(mStateUpdater);

        mSharedUniformStateUpdater = new SharedUniformStateUpdater(groundcover);
        rootNode->addUpdateCallback(mSharedUniformStateUpdater);

        mPostProcessor = new PostProcessor(*this, viewer, mRootNode);
        resourceSystem->getSceneManager()->setDepthFormat(mPostProcessor->getDepthFormat());

        if (reverseZ && !SceneUtil::isFloatingPointDepthFormat(mPostProcessor->getDepthFormat()))
            Log(Debug::Warning) << "Floating point depth format not in use but reverse-z buffer is enabled, consider disabling it.";

        // water goes after terrain for correct waterculling order
        mWater.reset(new Water(sceneRoot->getParent(0), sceneRoot, mResourceSystem, mViewer->getIncrementalCompileOperation(), resourcePath));

        mCamera.reset(new Camera(mViewer->getCamera()));
        if (Settings::Manager::getBool("view over shoulder", "Camera"))
            mViewOverShoulderController.reset(new ViewOverShoulderController(mCamera.get()));

        mScreenshotManager.reset(new ScreenshotManager(viewer, mRootNode, sceneRoot, mResourceSystem, mWater.get()));

        mViewer->setLightingMode(osgViewer::View::NO_LIGHT);

        osg::ref_ptr<osg::LightSource> source = new osg::LightSource;
        source->setNodeMask(Mask_Lighting);
        mSunLight = new osg::Light;
        source->setLight(mSunLight);
        mSunLight->setDiffuse(osg::Vec4f(0,0,0,1));
        mSunLight->setAmbient(osg::Vec4f(0,0,0,1));
        mSunLight->setSpecular(osg::Vec4f(0,0,0,0));
        mSunLight->setConstantAttenuation(1.f);
        sceneRoot->setSunlight(mSunLight);
        sceneRoot->addChild(source);

        sceneRoot->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
        sceneRoot->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::ON);
        sceneRoot->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
        osg::ref_ptr<osg::Material> defaultMat (new osg::Material);
        defaultMat->setColorMode(osg::Material::OFF);
        defaultMat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,1));
        defaultMat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,1));
        defaultMat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 0.f));
        sceneRoot->getOrCreateStateSet()->setAttribute(defaultMat);
        sceneRoot->getOrCreateStateSet()->addUniform(new osg::Uniform("emissiveMult", 1.f));

        mFog.reset(new FogManager());

        mSky.reset(new SkyManager(sceneRoot, resourceSystem->getSceneManager()));
        mSky->setCamera(mViewer->getCamera());

        source->setStateSetModes(*mRootNode->getOrCreateStateSet(), osg::StateAttribute::ON);

        osg::Camera::CullingMode cullingMode = osg::Camera::DEFAULT_CULLING|osg::Camera::FAR_PLANE_CULLING;

        if (!Settings::Manager::getBool("small feature culling", "Camera"))
            cullingMode &= ~(osg::CullStack::SMALL_FEATURE_CULLING);
        else
        {
            mViewer->getCamera()->setSmallFeatureCullingPixelSize(Settings::Manager::getFloat("small feature culling pixel size", "Camera"));
            cullingMode |= osg::CullStack::SMALL_FEATURE_CULLING;
        }

        mViewer->getCamera()->setCullingMode( cullingMode );

        mViewer->getCamera()->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
        mViewer->getCamera()->setCullingMode(cullingMode);

        mViewer->getCamera()->setCullMask(~(Mask_UpdateVisitor|Mask_SimpleWater));
        NifOsg::Loader::setHiddenNodeMask(Mask_UpdateVisitor);
        NifOsg::Loader::setIntersectionDisabledNodeMask(Mask_Effect);
        Nif::NIFFile::setLoadUnsupportedFiles(Settings::Manager::getBool("load unsupported nif files", "Models"));

        float firstPersonFov = Settings::Manager::getFloat("first person field of view", "Camera");
        mFirstPersonFieldOfView = std::min(std::max(1.f, firstPersonFov), 179.f);
        mStateUpdater->setFogEnd(mViewDistance);

        mRootNode->getOrCreateStateSet()->addUniform(new osg::Uniform("simpleWater", false));

        // Hopefully, anything genuinely requiring the default alpha func of GL_ALWAYS explicitly sets it
        mRootNode->getOrCreateStateSet()->setAttribute(Shader::RemovedAlphaFunc::getInstance(GL_ALWAYS));
        // The transparent renderbin sets alpha testing on because that was faster on old GPUs. It's now slower and breaks things.
        mRootNode->getOrCreateStateSet()->setMode(GL_ALPHA_TEST, osg::StateAttribute::OFF);

        if (reverseZ)
        {
            osg::ref_ptr<osg::ClipControl> clipcontrol = new osg::ClipControl(osg::ClipControl::LOWER_LEFT, osg::ClipControl::ZERO_TO_ONE);
            mRootNode->getOrCreateStateSet()->setAttributeAndModes(SceneUtil::createDepth(), osg::StateAttribute::ON);
            mRootNode->getOrCreateStateSet()->setAttributeAndModes(clipcontrol, osg::StateAttribute::ON);
        }

        SceneUtil::setCameraClearDepth(mViewer->getCamera());

        updateProjectionMatrix();
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
        osg::ref_ptr<PreloadCommonAssetsWorkItem> workItem (new PreloadCommonAssetsWorkItem(mResourceSystem));
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

    osg::Group* RenderingManager::getLightRoot()
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

    void RenderingManager::setAmbientColour(const osg::Vec4f &colour)
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

    void RenderingManager::configureAmbient(const ESM::Cell *cell)
    {
        bool needsAdjusting = false;
        if (mResourceSystem->getSceneManager()->getLightingMethod() != SceneUtil::LightingMethod::FFP)
            needsAdjusting = !cell->isExterior() && !(cell->mData.mFlags & ESM::Cell::QuasiEx);

        auto ambient = SceneUtil::colourFromRGB(cell->mAmbi.mAmbient);

        if (needsAdjusting)
        {
            constexpr float pR = 0.2126;
            constexpr float pG = 0.7152;
            constexpr float pB = 0.0722;

            // we already work in linear RGB so no conversions are needed for the luminosity function
            float relativeLuminance = pR*ambient.r() + pG*ambient.g() + pB*ambient.b();
            if (relativeLuminance < mMinimumAmbientLuminance)
            {
                // brighten ambient so it reaches the minimum threshold but no more, we want to mess with content data as least we can
                float targetBrightnessIncreaseFactor = mMinimumAmbientLuminance / relativeLuminance;
                if (ambient.r() == 0.f && ambient.g() == 0.f && ambient.b() == 0.f)
                    ambient = osg::Vec4(mMinimumAmbientLuminance, mMinimumAmbientLuminance, mMinimumAmbientLuminance, ambient.a());
                else
                    ambient *= targetBrightnessIncreaseFactor;
            }
        }

        setAmbientColour(ambient);

        osg::Vec4f diffuse = SceneUtil::colourFromRGB(cell->mAmbi.mSunlight);
        mSunLight->setDiffuse(diffuse);
        mSunLight->setSpecular(diffuse);
        mSunLight->setPosition(osg::Vec4f(-0.15f, 0.15f, 1.f, 0.f));
    }

    void RenderingManager::setSunColour(const osg::Vec4f& diffuse, const osg::Vec4f& specular)
    {
        // need to wrap this in a StateUpdater?
        mSunLight->setDiffuse(diffuse);
        mSunLight->setSpecular(specular);
    }

    void RenderingManager::setSunDirection(const osg::Vec3f &direction)
    {
        osg::Vec3 position = direction * -1;
        // need to wrap this in a StateUpdater?
        mSunLight->setPosition(osg::Vec4(position.x(), position.y(), position.z(), 0));

        mSky->setSunDirection(position);
    }

    void RenderingManager::addCell(const MWWorld::CellStore *store)
    {
        mPathgrid->addCell(store);

        mWater->changeCell(store);

        if (store->getCell()->isExterior())
        {
            mTerrain->loadCell(store->getCell()->getGridX(), store->getCell()->getGridY());
        }
    }
    void RenderingManager::removeCell(const MWWorld::CellStore *store)
    {
        mPathgrid->removeCell(store);
        mActorsPaths->removeCell(store);
        mObjects->removeCell(store);

        if (store->getCell()->isExterior())
        {
            mTerrain->unloadCell(store->getCell()->getGridX(), store->getCell()->getGridY());
        }

        mWater->removeCell(store);
    }

    void RenderingManager::enableTerrain(bool enable)
    {
        if (!enable)
            mWater->setCullCallback(nullptr);
        mTerrain->enable(enable);
    }

    void RenderingManager::setSkyEnabled(bool enabled)
    {
        mSky->setEnabled(enabled);
        if (enabled)
            mShadowManager->enableOutdoorMode();
        else
            mShadowManager->enableIndoorMode();
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
            unsigned int mask = mViewer->getCamera()->getCullMask();
            bool enabled = !(mask&sToggleWorldMask);
            if (enabled)
                mask |= sToggleWorldMask;
            else
                mask &= ~sToggleWorldMask;
            mWater->showWorld(enabled);
            mViewer->getCamera()->setCullMask(mask);
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

    void RenderingManager::configureFog(const ESM::Cell *cell)
    {
        mFog->configure(mViewDistance, cell);
    }

    void RenderingManager::configureFog(float fogDepth, float underwaterFog, float dlFactor, float dlOffset, const osg::Vec4f &color)
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

        float rainIntensity = mSky->getPrecipitationAlpha();
        mWater->setRainIntensity(rainIntensity);

        if (!paused)
        {
            mEffectManager->update(dt);
            mSky->update(dt);
            mWater->update(dt);

            const MWWorld::Ptr& player = mPlayerAnimation->getPtr();
            osg::Vec3f playerPos(player.getRefData().getPosition().asVec3());

            float windSpeed = mSky->getBaseWindSpeed();
            mSharedUniformStateUpdater->setWindSpeed(windSpeed);
            mSharedUniformStateUpdater->setPlayerPos(playerPos);
        }

        updateNavMesh();
        updateRecastMesh();

        if (mViewOverShoulderController)
            mViewOverShoulderController->update();
        mCamera->update(dt, paused);

        osg::Vec3d focal, cameraPos;
        mCamera->getPosition(focal, cameraPos);
        mCurrentCameraPos = cameraPos;

        bool isUnderwater = mWater->isUnderwater(cameraPos);
        mStateUpdater->setFogStart(mFog->getFogStart(isUnderwater));
        mStateUpdater->setFogEnd(mFog->getFogEnd(isUnderwater));
        setFogColor(mFog->getFogColor(isUnderwater));
    }

    void RenderingManager::updatePlayerPtr(const MWWorld::Ptr &ptr)
    {
        if(mPlayerAnimation.get())
        {
            setupPlayer(ptr);
            mPlayerAnimation->updatePtr(ptr);
        }
        mCamera->attachTo(ptr);
    }

    void RenderingManager::removePlayer(const MWWorld::Ptr &player)
    {
        mWater->removeEmitter(player);
    }

    void RenderingManager::rotateObject(const MWWorld::Ptr &ptr, const osg::Quat& rot)
    {
        if(ptr == mCamera->getTrackingPtr() &&
           !mCamera->isVanityOrPreviewModeEnabled())
        {
            mCamera->rotateCameraToTrackingPtr();
        }

        ptr.getRefData().getBaseNode()->setAttitude(rot);
    }

    void RenderingManager::moveObject(const MWWorld::Ptr &ptr, const osg::Vec3f &pos)
    {
        ptr.getRefData().getBaseNode()->setPosition(pos);
    }

    void RenderingManager::scaleObject(const MWWorld::Ptr &ptr, const osg::Vec3f &scale)
    {
        ptr.getRefData().getBaseNode()->setScale(scale);

        if (ptr == mCamera->getTrackingPtr()) // update height of camera
            mCamera->processViewChange();
    }

    void RenderingManager::removeObject(const MWWorld::Ptr &ptr)
    {
        mActorsPaths->remove(ptr);
        mObjects->removeObject(ptr);
        mWater->removeEmitter(ptr);
    }

    void RenderingManager::setWaterEnabled(bool enabled)
    {
        mWater->setEnabled(enabled);
        mSky->setWaterEnabled(enabled);
    }

    void RenderingManager::setWaterHeight(float height)
    {
        mWater->setCullCallback(mTerrain->getHeightCullCallback(height, Mask_Water));
        mWater->setHeight(height);
        mSky->setWaterHeight(height);
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

        unsigned int maskBackup = mPlayerAnimation->getObjectRoot()->getNodeMask();

        if (mCamera->isFirstPerson())
            mPlayerAnimation->getObjectRoot()->setNodeMask(0);

        mScreenshotManager->screenshot360(image);

        mPlayerAnimation->getObjectRoot()->setNodeMask(maskBackup);

        return true;
    }

    osg::Vec4f RenderingManager::getScreenBounds(const osg::BoundingBox &worldbb)
    {
        if (!worldbb.valid()) return osg::Vec4f();
        osg::Matrix viewProj = mViewer->getCamera()->getViewMatrix() * mViewer->getCamera()->getProjectionMatrix();
        float min_x = 1.0f, max_x = 0.0f, min_y = 1.0f, max_y = 0.0f;
        for (int i=0; i<8; ++i)
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

    RenderingManager::RayResult getIntersectionResult (osgUtil::LineSegmentIntersector* intersector)
    {
        RenderingManager::RayResult result;
        result.mHit = false;
        result.mHitRefnum.unset();
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
            for (osg::NodePath::const_iterator it = intersection.nodePath.begin(); it != intersection.nodePath.end(); ++it)
            {
                osg::UserDataContainer* userDataContainer = (*it)->getUserDataContainer();
                if (!userDataContainer)
                    continue;
                for (unsigned int i=0; i<userDataContainer->getNumUserObjects(); ++i)
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
            for (unsigned int i=0; i<refnumMarkers.size(); ++i)
            {
                unsigned int intersectionIndex = intersection.indexList.empty() ? 0 : intersection.indexList[0];
                if (!refnumMarkers[i]->mNumVertices || (intersectionIndex >= vertexCounter && intersectionIndex < vertexCounter + refnumMarkers[i]->mNumVertices))
                {
                    result.mHitRefnum = refnumMarkers[i]->mRefnum;
                    break;
                }
                vertexCounter += refnumMarkers[i]->mNumVertices;
            }
        }

        return result;

    }

    osg::ref_ptr<osgUtil::IntersectionVisitor> RenderingManager::getIntersectionVisitor(osgUtil::Intersector *intersector, bool ignorePlayer, bool ignoreActors)
    {
        if (!mIntersectionVisitor)
            mIntersectionVisitor = new osgUtil::IntersectionVisitor;

        mIntersectionVisitor->setTraversalNumber(mViewer->getFrameStamp()->getFrameNumber());
        mIntersectionVisitor->setFrameStamp(mViewer->getFrameStamp());
        mIntersectionVisitor->setIntersector(intersector);

        unsigned int mask = ~0u;
        mask &= ~(Mask_RenderToTexture|Mask_Sky|Mask_Debug|Mask_Effect|Mask_Water|Mask_SimpleWater|Mask_Groundcover);
        if (ignorePlayer)
            mask &= ~(Mask_Player);
        if (ignoreActors)
            mask &= ~(Mask_Actor|Mask_Player);

        mIntersectionVisitor->setTraversalMask(mask);
        return mIntersectionVisitor;
    }

    RenderingManager::RayResult RenderingManager::castRay(const osg::Vec3f& origin, const osg::Vec3f& dest, bool ignorePlayer, bool ignoreActors)
    {
        osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector (new osgUtil::LineSegmentIntersector(osgUtil::LineSegmentIntersector::MODEL,
            origin, dest));
        intersector->setIntersectionLimit(osgUtil::LineSegmentIntersector::LIMIT_NEAREST);

        mRootNode->accept(*getIntersectionVisitor(intersector, ignorePlayer, ignoreActors));

        return getIntersectionResult(intersector);
    }

    RenderingManager::RayResult RenderingManager::castCameraToViewportRay(const float nX, const float nY, float maxDistance, bool ignorePlayer, bool ignoreActors)
    {
        osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector (new osgUtil::LineSegmentIntersector(osgUtil::LineSegmentIntersector::PROJECTION,
                                                                                                       nX * 2.f - 1.f, nY * (-2.f) + 1.f));

        osg::Vec3d dist (0.f, 0.f, -maxDistance);

        dist = dist * mViewer->getCamera()->getProjectionMatrix();

        osg::Vec3d end = intersector->getEnd();
        end.z() = dist.z();
        intersector->setEnd(end);
        intersector->setIntersectionLimit(osgUtil::LineSegmentIntersector::LIMIT_NEAREST);

        mViewer->getCamera()->accept(*getIntersectionVisitor(intersector, ignorePlayer, ignoreActors));

        return getIntersectionResult(intersector);
    }

    void RenderingManager::updatePtr(const MWWorld::Ptr &old, const MWWorld::Ptr &updated)
    {
        mObjects->updatePtr(old, updated);
        mActorsPaths->updatePtr(old, updated);
    }

    void RenderingManager::spawnEffect(const std::string &model, const std::string &texture, const osg::Vec3f &worldPosition, float scale, bool isMagicVFX)
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
    }

    MWRender::Animation* RenderingManager::getAnimation(const MWWorld::Ptr &ptr)
    {
        if (mPlayerAnimation.get() && ptr == mPlayerAnimation->getPtr())
            return mPlayerAnimation.get();

        return mObjects->getAnimation(ptr);
    }

    const MWRender::Animation* RenderingManager::getAnimation(const MWWorld::ConstPtr &ptr) const
    {
        if (mPlayerAnimation.get() && ptr == mPlayerAnimation->getPtr())
            return mPlayerAnimation.get();

        return mObjects->getAnimation(ptr);
    }

    void RenderingManager::setupPlayer(const MWWorld::Ptr &player)
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

    void RenderingManager::renderPlayer(const MWWorld::Ptr &player)
    {
        mPlayerAnimation = new NpcAnimation(player, player.getRefData().getBaseNode(), mResourceSystem, 0, NpcAnimation::VM_Normal,
                                                mFirstPersonFieldOfView);

        mCamera->setAnimation(mPlayerAnimation.get());
        mCamera->attachTo(player);
    }

    void RenderingManager::rebuildPtr(const MWWorld::Ptr &ptr)
    {
        NpcAnimation *anim = nullptr;
        if(ptr == mPlayerAnimation->getPtr())
            anim = mPlayerAnimation.get();
        else
            anim = dynamic_cast<NpcAnimation*>(mObjects->getAnimation(ptr));
        if(anim)
        {
            anim->rebuild();
            if(mCamera->getTrackingPtr() == ptr)
            {
                mCamera->attachTo(ptr);
                mCamera->setAnimation(anim);
            }
        }
    }

    void RenderingManager::addWaterRippleEmitter(const MWWorld::Ptr &ptr)
    {
        mWater->addEmitter(ptr);
    }

    void RenderingManager::removeWaterRippleEmitter(const MWWorld::Ptr &ptr)
    {
        mWater->removeEmitter(ptr);
    }

    void RenderingManager::emitWaterRipple(const osg::Vec3f &pos)
    {
        mWater->emitRipple(pos);
    }

    void RenderingManager::updateProjectionMatrix()
    {
        double aspect = mViewer->getCamera()->getViewport()->aspectRatio();
        float fov = mFieldOfView;
        if (mFieldOfViewOverridden)
            fov = mFieldOfViewOverride;

        mViewer->getCamera()->setProjectionMatrixAsPerspective(fov, aspect, mNearClip, mViewDistance);

        if (SceneUtil::getReverseZ())
        {
            mSharedUniformStateUpdater->setLinearFac(-mNearClip / (mViewDistance - mNearClip) - 1.f);
            mSharedUniformStateUpdater->setProjectionMatrix(SceneUtil::getReversedZProjectionMatrixAsPerspective(fov, aspect, mNearClip, mViewDistance));
        }
        else
            mSharedUniformStateUpdater->setProjectionMatrix(mViewer->getCamera()->getProjectionMatrix());

        mSharedUniformStateUpdater->setNear(mNearClip);
        mSharedUniformStateUpdater->setFar(mViewDistance);

        // Since our fog is not radial yet, we should take FOV in account, otherwise terrain near viewing distance may disappear.
        // Limit FOV here just for sure, otherwise viewing distance can be too high.
        fov = std::min(mFieldOfView, 140.f);
        float distanceMult = std::cos(osg::DegreesToRadians(fov)/2.f);
        mTerrain->setViewDistance(mViewDistance * (distanceMult ? 1.f/distanceMult : 1.f));
    }

    void RenderingManager::updateTextureFiltering()
    {
        mViewer->stopThreading();

        mResourceSystem->getSceneManager()->setFilterSettings(
            Settings::Manager::getString("texture mag filter", "General"),
            Settings::Manager::getString("texture min filter", "General"),
            Settings::Manager::getString("texture mipmap", "General"),
            Settings::Manager::getInt("anisotropy", "General")
        );

        mTerrain->updateTextureFiltering();

        mViewer->startThreading();
    }

    void RenderingManager::updateAmbient()
    {
        osg::Vec4f color = mAmbientColor;

        if (mNightEyeFactor > 0.f)
            color += osg::Vec4f(0.7, 0.7, 0.7, 0.0) * mNightEyeFactor;

        mStateUpdater->setAmbientColor(color);
    }

    void RenderingManager::setFogColor(const osg::Vec4f &color)
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

    void RenderingManager::processChangedSettings(const Settings::CategorySettingVector &changed)
    {
        for (Settings::CategorySettingVector::const_iterator it = changed.begin(); it != changed.end(); ++it)
        {
            if (it->first == "Camera" && it->second == "field of view")
            {
                mFieldOfView = Settings::Manager::getFloat("field of view", "Camera");
                updateProjectionMatrix();
            }
            else if (it->first == "Camera" && it->second == "viewing distance")
            {
                mViewDistance = Settings::Manager::getFloat("viewing distance", "Camera");
                if(!Settings::Manager::getBool("use distant fog", "Fog"))
                    mStateUpdater->setFogEnd(mViewDistance);
                updateProjectionMatrix();
            }
            else if (it->first == "General" && (it->second == "texture filter" ||
                                                it->second == "texture mipmap" ||
                                                it->second == "anisotropy"))
            {
                updateTextureFiltering();
            }
            else if (it->first == "Water")
            {
                mWater->processChangedSettings(changed);
            }
            else if (it->first == "Shaders" && it->second == "minimum interior brightness")
            {
                mMinimumAmbientLuminance = std::clamp(Settings::Manager::getFloat("minimum interior brightness", "Shaders"), 0.f, 1.f);
                if (MWMechanics::getPlayer().isInCell())
                    configureAmbient(MWMechanics::getPlayer().getCell()->getCell());
            }
            else if (it->first == "Shaders" && (it->second == "light bounds multiplier" ||
                                                it->second == "maximum light distance" ||
                                                it->second == "light fade start" ||
                                                it->second == "max lights"))
            {
                auto* lightManager = static_cast<SceneUtil::LightManager*>(getLightRoot());
                lightManager->processChangedSettings(changed);

                if (it->second == "max lights" && !lightManager->usingFFP())
                {
                    mViewer->stopThreading();

                    lightManager->updateMaxLights();

                    auto defines = mResourceSystem->getSceneManager()->getShaderManager().getGlobalDefines();
                    for (const auto& [name, key] : lightManager->getLightDefines())
                        defines[name] = key;
                    mResourceSystem->getSceneManager()->getShaderManager().setGlobalDefines(defines);

                    mStateUpdater->reset();

                    mViewer->startThreading();
                }
            }
        }
    }

    float RenderingManager::getNearClipDistance() const
    {
        return mNearClip;
    }

    float RenderingManager::getTerrainHeightAt(const osg::Vec3f &pos)
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

    osg::Vec3f RenderingManager::getHalfExtents(const MWWorld::ConstPtr& object) const
    {
        osg::Vec3f halfExtents(0, 0, 0);
        std::string modelName = object.getClass().getModel(object);
        if (modelName.empty())
            return halfExtents;

        osg::ref_ptr<const osg::Node> node = mResourceSystem->getSceneManager()->getTemplate(modelName);
        osg::ComputeBoundsVisitor computeBoundsVisitor;
        computeBoundsVisitor.setTraversalMask(~(MWRender::Mask_ParticleSystem|MWRender::Mask_Effect));
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
    void RenderingManager::exportSceneGraph(const MWWorld::Ptr &ptr, const std::string &filename, const std::string &format)
    {
        osg::Node* node = mViewer->getSceneData();
        if (!ptr.isEmpty())
            node = ptr.getRefData().getBaseNode();

        SceneUtil::writeScene(node, filename, format);
    }

    LandManager *RenderingManager::getLandManager() const
    {
        return mTerrainStorage->getLandManager();
    }

    void RenderingManager::updateActorPath(const MWWorld::ConstPtr& actor, const std::deque<osg::Vec3f>& path,
            const osg::Vec3f& halfExtents, const osg::Vec3f& start, const osg::Vec3f& end) const
    {
        mActorsPaths->update(actor, path, halfExtents, start, end, mNavigator.getSettings());
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
                const auto locked = it->second->lockConst();
                mNavMesh->update(locked->getImpl(), mNavMeshNumber, locked->getGeneration(),
                                 locked->getNavMeshRevision(), mNavigator.getSettings());
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

    void RenderingManager::setActiveGrid(const osg::Vec4i &grid)
    {
        mTerrain->setActiveGrid(grid);
    }
    bool RenderingManager::pagingEnableObject(int type, const MWWorld::ConstPtr& ptr, bool enabled)
    {
        if (!ptr.isInCell() || !ptr.getCell()->isExterior() || !mObjectPaging)
            return false;
        if (mObjectPaging->enableObject(type, ptr.getCellRef().getRefNum(), ptr.getCellRef().getPosition().asVec3(), osg::Vec2i(ptr.getCell()->getCell()->getGridX(), ptr.getCell()->getCell()->getGridY()), enabled))
        {
            mTerrain->rebuildViews();
            return true;
        }
        return false;
    }
    void RenderingManager::pagingBlacklistObject(int type, const MWWorld::ConstPtr &ptr)
    {
        if (!ptr.isInCell() || !ptr.getCell()->isExterior() || !mObjectPaging)
            return;
        const ESM::RefNum & refnum = ptr.getCellRef().getRefNum();
        if (!refnum.hasContentFile()) return;
        if (mObjectPaging->blacklistObject(type, refnum, ptr.getCellRef().getPosition().asVec3(), osg::Vec2i(ptr.getCell()->getCell()->getGridX(), ptr.getCell()->getCell()->getGridY())))
            mTerrain->rebuildViews();
    }
    bool RenderingManager::pagingUnlockCache()
    {
        if (mObjectPaging && mObjectPaging->unlockCache())
        {
            mTerrain->rebuildViews();
            return true;
        }
        return false;
    }
    void RenderingManager::getPagedRefnums(const osg::Vec4i &activeGrid, std::set<ESM::RefNum> &out)
    {
        if (mObjectPaging)
            mObjectPaging->getPagedRefnums(activeGrid, out);
    }
}
