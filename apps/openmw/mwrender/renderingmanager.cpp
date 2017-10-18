#include "renderingmanager.hpp"

#include <stdexcept>
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

#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/IncrementalCompileOperation>

#include <osgViewer/Viewer>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/imagemanager.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/resource/keyframemanager.hpp>

#include <components/settings/settings.hpp>

#include <components/sceneutil/util.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/statesetupdater.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/workqueue.hpp>
#include <components/sceneutil/unrefqueue.hpp>
#include <components/sceneutil/writescene.hpp>

#include <components/terrain/terraingrid.hpp>
#include <components/terrain/quadtreeworld.hpp>

#include <components/esm/loadcell.hpp>
#include <components/fallback/fallback.hpp>

#include "../mwworld/cellstore.hpp"

#include "sky.hpp"
#include "effectmanager.hpp"
#include "npcanimation.hpp"
#include "vismask.hpp"
#include "pathgrid.hpp"
#include "camera.hpp"
#include "water.hpp"
#include "terrainstorage.hpp"
#include "util.hpp"

namespace MWRender
{

    class StateUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        StateUpdater()
            : mFogStart(0.f)
            , mFogEnd(0.f)
            , mWireframe(false)
        {
        }

        virtual void setDefaults(osg::StateSet *stateset)
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

        virtual void apply(osg::StateSet* stateset, osg::NodeVisitor*)
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

        virtual void doWork()
        {
            try
            {
                for (std::vector<std::string>::const_iterator it = mModels.begin(); it != mModels.end(); ++it)
                    mResourceSystem->getSceneManager()->cacheInstance(*it);
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

    RenderingManager::RenderingManager(osgViewer::Viewer* viewer, osg::ref_ptr<osg::Group> rootNode, Resource::ResourceSystem* resourceSystem, SceneUtil::WorkQueue* workQueue,
                                       const Fallback::Map* fallback, const std::string& resourcePath)
        : mViewer(viewer)
        , mRootNode(rootNode)
        , mResourceSystem(resourceSystem)
        , mWorkQueue(workQueue)
        , mUnrefQueue(new SceneUtil::UnrefQueue)
        , mFogDepth(0.f)
        , mUnderwaterColor(fallback->getFallbackColour("Water_UnderwaterColor"))
        , mUnderwaterWeight(fallback->getFallbackFloat("Water_UnderwaterColorWeight"))
        , mUnderwaterFog(0.f)
        , mUnderwaterIndoorFog(fallback->getFallbackFloat("Water_UnderwaterIndoorFog"))
        , mNightEyeFactor(0.f)
        , mFieldOfViewOverride(0.f)
        , mFieldOfViewOverridden(false)
    {
        resourceSystem->getSceneManager()->setParticleSystemMask(MWRender::Mask_ParticleSystem);
        resourceSystem->getSceneManager()->setShaderPath(resourcePath + "/shaders");
        resourceSystem->getSceneManager()->setForceShaders(Settings::Manager::getBool("force shaders", "Shaders"));
        resourceSystem->getSceneManager()->setClampLighting(Settings::Manager::getBool("clamp lighting", "Shaders"));
        resourceSystem->getSceneManager()->setForcePerPixelLighting(Settings::Manager::getBool("force per pixel lighting", "Shaders"));
        resourceSystem->getSceneManager()->setAutoUseNormalMaps(Settings::Manager::getBool("auto use object normal maps", "Shaders"));
        resourceSystem->getSceneManager()->setNormalMapPattern(Settings::Manager::getString("normal map pattern", "Shaders"));
        resourceSystem->getSceneManager()->setNormalHeightMapPattern(Settings::Manager::getString("normal height map pattern", "Shaders"));
        resourceSystem->getSceneManager()->setAutoUseSpecularMaps(Settings::Manager::getBool("auto use object specular maps", "Shaders"));
        resourceSystem->getSceneManager()->setSpecularMapPattern(Settings::Manager::getString("specular map pattern", "Shaders"));

        osg::ref_ptr<SceneUtil::LightManager> sceneRoot = new SceneUtil::LightManager;
        sceneRoot->setLightingMask(Mask_Lighting);
        mSceneRoot = sceneRoot;
        sceneRoot->setStartLight(1);

        mRootNode->addChild(sceneRoot);

        mPathgrid.reset(new Pathgrid(mRootNode));

        mObjects.reset(new Objects(mResourceSystem, sceneRoot, mUnrefQueue.get()));

        if (getenv("OPENMW_DONT_PRECOMPILE") == NULL)
            mViewer->setIncrementalCompileOperation(new osgUtil::IncrementalCompileOperation);

        mResourceSystem->getSceneManager()->setIncrementalCompileOperation(mViewer->getIncrementalCompileOperation());

        mEffectManager.reset(new EffectManager(sceneRoot, mResourceSystem));

        mWater.reset(new Water(mRootNode, sceneRoot, mResourceSystem, mViewer->getIncrementalCompileOperation(), fallback, resourcePath));

        const bool distantTerrain = Settings::Manager::getBool("distant terrain", "Terrain");
        mTerrainStorage = new TerrainStorage(mResourceSystem, Settings::Manager::getString("normal map pattern", "Shaders"), Settings::Manager::getString("normal height map pattern", "Shaders"),
                                            Settings::Manager::getBool("auto use terrain normal maps", "Shaders"), Settings::Manager::getString("terrain specular map pattern", "Shaders"),
                                             Settings::Manager::getBool("auto use terrain specular maps", "Shaders"));

        if (distantTerrain)
            mTerrain.reset(new Terrain::QuadTreeWorld(sceneRoot, mRootNode, mResourceSystem, mTerrainStorage, Mask_Terrain, Mask_PreCompile));
        else
            mTerrain.reset(new Terrain::TerrainGrid(sceneRoot, mRootNode, mResourceSystem, mTerrainStorage, Mask_Terrain, Mask_PreCompile));
        mTerrain->setDefaultViewer(mViewer->getCamera());

        mCamera.reset(new Camera(mViewer->getCamera()));

        mViewer->setLightingMode(osgViewer::View::NO_LIGHT);

        osg::ref_ptr<osg::LightSource> source = new osg::LightSource;
        source->setNodeMask(Mask_Lighting);
        mSunLight = new osg::Light;
        source->setLight(mSunLight);
        mSunLight->setDiffuse(osg::Vec4f(0,0,0,1));
        mSunLight->setAmbient(osg::Vec4f(0,0,0,1));
        mSunLight->setSpecular(osg::Vec4f(0,0,0,0));
        mSunLight->setConstantAttenuation(1.f);
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

        sceneRoot->setNodeMask(Mask_Scene);
        sceneRoot->setName("Scene Root");

        mSky.reset(new SkyManager(sceneRoot, resourceSystem->getSceneManager()));
        mSky->setCamera(mViewer->getCamera());
        mSky->setRainIntensityUniform(mWater->getRainIntensityUniform());

        source->setStateSetModes(*mRootNode->getOrCreateStateSet(), osg::StateAttribute::ON);

        mStateUpdater = new StateUpdater;
        sceneRoot->addUpdateCallback(mStateUpdater);

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

        mNearClip = Settings::Manager::getFloat("near clip", "Camera");
        mViewDistance = Settings::Manager::getFloat("viewing distance", "Camera");
        mFieldOfView = Settings::Manager::getFloat("field of view", "Camera");
        mFirstPersonFieldOfView = Settings::Manager::getFloat("first person field of view", "Camera");
        mStateUpdater->setFogEnd(mViewDistance);

        mRootNode->getOrCreateStateSet()->addUniform(new osg::Uniform("near", mNearClip));
        mRootNode->getOrCreateStateSet()->addUniform(new osg::Uniform("far", mViewDistance));

        mUniformNear = mRootNode->getOrCreateStateSet()->getUniform("near");
        mUniformFar = mRootNode->getOrCreateStateSet()->getUniform("far");
        updateProjectionMatrix();
    }

    RenderingManager::~RenderingManager()
    {
        // let background loading thread finish before we delete anything else
        mWorkQueue = NULL;
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

    SceneUtil::UnrefQueue* RenderingManager::getUnrefQueue()
    {
        return mUnrefQueue.get();
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

        const char* basemodels[] = {"xbase_anim", "xbase_anim.1st", "xbase_anim_female", "xbase_animkna"};
        for (size_t i=0; i<sizeof(basemodels)/sizeof(basemodels[0]); ++i)
        {
            workItem->mModels.push_back(std::string("meshes/") + basemodels[i] + ".nif");
            workItem->mKeyframes.push_back(std::string("meshes/") + basemodels[i] + ".kf");
        }

        workItem->mTextures.push_back("textures/_land_default.dds");

        mWorkQueue->addWorkItem(workItem);
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
        setAmbientColour(SceneUtil::colourFromRGB(cell->mAmbi.mAmbient));

        osg::Vec4f diffuse = SceneUtil::colourFromRGB(cell->mAmbi.mSunlight);
        mSunLight->setDiffuse(diffuse);
        mSunLight->setSpecular(diffuse);
        mSunLight->setDirection(osg::Vec3f(1.f,-1.f,-1.f));
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
            mTerrain->loadCell(store->getCell()->getGridX(), store->getCell()->getGridY());
    }
    void RenderingManager::removeCell(const MWWorld::CellStore *store)
    {
        mPathgrid->removeCell(store);
        mObjects->removeCell(store);

        if (store->getCell()->isExterior())
            mTerrain->unloadCell(store->getCell()->getGridX(), store->getCell()->getGridY());

        mWater->removeCell(store);
    }

    void RenderingManager::enableTerrain(bool enable)
    {
        mTerrain->enable(enable);
    }

    void RenderingManager::setSkyEnabled(bool enabled)
    {
        mSky->setEnabled(enabled);
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
            int mask = mViewer->getCamera()->getCullMask();
            bool enabled = mask&Mask_Scene;
            enabled = !enabled;
            if (enabled)
                mask |= Mask_Scene;
            else
                mask &= ~Mask_Scene;
            mViewer->getCamera()->setCullMask(mask);
            return enabled;
        }
        return false;
    }

    void RenderingManager::configureFog(const ESM::Cell *cell)
    {
        osg::Vec4f color = SceneUtil::colourFromRGB(cell->mAmbi.mFog);

        configureFog (cell->mAmbi.mFogDensity, mUnderwaterIndoorFog, color);
    }

    void RenderingManager::configureFog(float fogDepth, float underwaterFog, const osg::Vec4f &color)
    {
        mFogDepth = fogDepth;
        mFogColor = color;
        mUnderwaterFog = underwaterFog;
    }

    SkyManager* RenderingManager::getSkyManager()
    {
        return mSky.get();
    }

    void RenderingManager::update(float dt, bool paused)
    {
        reportStats();

        mUnrefQueue->flush(mWorkQueue.get());

        if (!paused)
        {
            mEffectManager->update(dt);
            mSky->update(dt);
            mWater->update(dt);
        }

        mCamera->update(dt, paused);

        osg::Vec3f focal, cameraPos;
        mCamera->getPosition(focal, cameraPos);
        mCurrentCameraPos = cameraPos;
        if (mWater->isUnderwater(cameraPos))
        {
            float viewDistance = mViewDistance;
            viewDistance = std::min(viewDistance, 6666.f);
            setFogColor(mUnderwaterColor * mUnderwaterWeight + mFogColor * (1.f-mUnderwaterWeight));
            mStateUpdater->setFogStart(viewDistance * (1 - mUnderwaterFog));
            mStateUpdater->setFogEnd(viewDistance);
        }
        else
        {
            setFogColor(mFogColor);

            if (mFogDepth == 0.f)
            {
                mStateUpdater->setFogStart(0.f);
                mStateUpdater->setFogEnd(std::numeric_limits<float>::max());
            }
            else
            {
                mStateUpdater->setFogStart(mViewDistance * (1 - mFogDepth));
                mStateUpdater->setFogEnd(mViewDistance);
            }
        }
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
            mCamera->rotateCamera(-ptr.getRefData().getPosition().rot[0], -ptr.getRefData().getPosition().rot[2], false);
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
        mWater->setHeight(height);
        mSky->setWaterHeight(height);
    }

    class NotifyDrawCompletedCallback : public osg::Camera::DrawCallback
    {
    public:
        NotifyDrawCompletedCallback()
            : mDone(false)
        {
        }

        virtual void operator () (osg::RenderInfo& renderInfo) const
        {
            mMutex.lock();
            mDone = true;
            mMutex.unlock();
            mCondition.signal();
        }

        void waitTillDone()
        {
            mMutex.lock();
            if (mDone)
                return;
            mCondition.wait(&mMutex);
            mMutex.unlock();
        }

        mutable OpenThreads::Condition mCondition;
        mutable OpenThreads::Mutex mMutex;
        mutable bool mDone;
    };

    void RenderingManager::screenshot(osg::Image *image, int w, int h)
    {
        osg::ref_ptr<osg::Camera> rttCamera (new osg::Camera);
        rttCamera->setNodeMask(Mask_RenderToTexture);
        rttCamera->attach(osg::Camera::COLOR_BUFFER, image);
        rttCamera->setRenderOrder(osg::Camera::PRE_RENDER);
        rttCamera->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
        rttCamera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT, osg::Camera::PIXEL_BUFFER_RTT);
        rttCamera->setProjectionMatrixAsPerspective(mFieldOfView, w/float(h), mNearClip, mViewDistance);
        rttCamera->setViewMatrix(mViewer->getCamera()->getViewMatrix());
        rttCamera->setViewport(0, 0, w, h);

        osg::ref_ptr<osg::Texture2D> texture (new osg::Texture2D);
        texture->setInternalFormat(GL_RGB);
        texture->setTextureSize(w, h);
        texture->setResizeNonPowerOfTwoHint(false);
        texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        rttCamera->attach(osg::Camera::COLOR_BUFFER, texture);

        image->setDataType(GL_UNSIGNED_BYTE);
        image->setPixelFormat(texture->getInternalFormat());

        rttCamera->setUpdateCallback(new NoTraverseCallback);
        rttCamera->addChild(mSceneRoot);
        rttCamera->setCullMask(mViewer->getCamera()->getCullMask() & (~Mask_GUI));

        mRootNode->addChild(rttCamera);

        // The draw needs to complete before we can copy back our image.
        osg::ref_ptr<NotifyDrawCompletedCallback> callback (new NotifyDrawCompletedCallback);
        rttCamera->setFinalDrawCallback(callback);

        // at the time this function is called we are in the middle of a frame,
        // so out of order calls are necessary to get a correct frameNumber for the next frame.
        // refer to the advance() and frame() order in Engine::go()
        mViewer->eventTraversal();
        mViewer->updateTraversal();
        mViewer->renderingTraversals();

        callback->waitTillDone();

        // now that we've "used up" the current frame, get a fresh framenumber for the next frame() following after the screenshot is completed
        mViewer->advance(mViewer->getFrameStamp()->getSimulationTime());

        rttCamera->removeChildren(0, rttCamera->getNumChildren());
        mRootNode->removeChild(rttCamera);
    }

    osg::Vec4f RenderingManager::getScreenBounds(const MWWorld::Ptr& ptr)
    {
        if (!ptr.getRefData().getBaseNode())
            return osg::Vec4f();

        osg::ComputeBoundsVisitor computeBoundsVisitor;
        computeBoundsVisitor.setTraversalMask(~(Mask_ParticleSystem|Mask_Effect));
        ptr.getRefData().getBaseNode()->accept(computeBoundsVisitor);

        osg::Matrix viewProj = mViewer->getCamera()->getViewMatrix() * mViewer->getCamera()->getProjectionMatrix();
        float min_x = 1.0f, max_x = 0.0f, min_y = 1.0f, max_y = 0.0f;
        for (int i=0; i<8; ++i)
        {
            osg::Vec3f corner = computeBoundsVisitor.getBoundingBox().corner(i);
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
        result.mRatio = 0;
        if (intersector->containsIntersections())
        {
            result.mHit = true;
            osgUtil::LineSegmentIntersector::Intersection intersection = intersector->getFirstIntersection();

            result.mHitPointWorld = intersection.getWorldIntersectPoint();
            result.mHitNormalWorld = intersection.getWorldIntersectNormal();
            result.mRatio = intersection.ratio;

            PtrHolder* ptrHolder = NULL;
            for (osg::NodePath::const_iterator it = intersection.nodePath.begin(); it != intersection.nodePath.end(); ++it)
            {
                osg::UserDataContainer* userDataContainer = (*it)->getUserDataContainer();
                if (!userDataContainer)
                    continue;
                for (unsigned int i=0; i<userDataContainer->getNumUserObjects(); ++i)
                {
                    if (PtrHolder* p = dynamic_cast<PtrHolder*>(userDataContainer->getUserObject(i)))
                        ptrHolder = p;
                }
            }

            if (ptrHolder)
                result.mHitObject = ptrHolder->mPtr;
        }

        return result;

    }

    osg::ref_ptr<osgUtil::IntersectionVisitor> RenderingManager::getIntersectionVisitor(osgUtil::Intersector *intersector, bool ignorePlayer, bool ignoreActors)
    {
        if (!mIntersectionVisitor)
            mIntersectionVisitor = new osgUtil::IntersectionVisitor;

        mIntersectionVisitor->setTraversalNumber(mViewer->getFrameStamp()->getFrameNumber());
        mIntersectionVisitor->setIntersector(intersector);

        int mask = ~0;
        mask &= ~(Mask_RenderToTexture|Mask_Sky|Mask_Debug|Mask_Effect|Mask_Water|Mask_SimpleWater);
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
    }

    void RenderingManager::spawnEffect(const std::string &model, const std::string &texture, const osg::Vec3f &worldPosition, float scale, bool isMagicVFX)
    {
        mEffectManager->addEffect(model, texture, worldPosition, scale, isMagicVFX);
    }

    void RenderingManager::notifyWorldSpaceChanged()
    {
        mEffectManager->clear();
    }

    void RenderingManager::clear()
    {
        mSky->setMoonColour(false);

        notifyWorldSpaceChanged();
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
        NpcAnimation *anim = NULL;
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

        mUniformNear->set(mNearClip);
        mUniformFar->set(mViewDistance);
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
            stats->setAttribute(frameNumber, "UnrefQueue", mUnrefQueue->getNumItems());

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
                mStateUpdater->setFogEnd(mViewDistance);
                updateProjectionMatrix();
            }
            else if (it->first == "General" && (it->second == "texture filter" ||
                                                it->second == "texture mipmap" ||
                                                it->second == "anisotropy"))
                updateTextureFiltering();
            else if (it->first == "Water")
                mWater->processChangedSettings(changed);
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

    bool RenderingManager::vanityRotateCamera(const float *rot)
    {
        if(!mCamera->isVanityOrPreviewModeEnabled())
            return false;

        mCamera->rotateCamera(rot[0], rot[2], true);
        return true;
    }

    void RenderingManager::setCameraDistance(float dist, bool adjust, bool override)
    {
        if(!mCamera->isVanityOrPreviewModeEnabled() && !mCamera->isFirstPerson())
        {
            if(mCamera->isNearest() && dist > 0.f)
                mCamera->toggleViewMode();
            else
                mCamera->setCameraDistance(-dist / 120.f * 10, adjust, override);
        }
        else if(mCamera->isFirstPerson() && dist < 0.f)
        {
            mCamera->toggleViewMode();
            mCamera->setCameraDistance(0.f, false, override);
        }
    }

    void RenderingManager::resetCamera()
    {
        mCamera->reset();
    }

    float RenderingManager::getCameraDistance() const
    {
        return mCamera->getCameraDistance();
    }

    Camera* RenderingManager::getCamera()
    {
        return mCamera.get();
    }

    const osg::Vec3f &RenderingManager::getCameraPosition() const
    {
        return mCurrentCameraPos;
    }

    void RenderingManager::togglePOV()
    {
        mCamera->toggleViewMode();
    }

    void RenderingManager::togglePreviewMode(bool enable)
    {
        mCamera->togglePreviewMode(enable);
    }

    bool RenderingManager::toggleVanityMode(bool enable)
    {
        return mCamera->toggleVanityMode(enable);
    }

    void RenderingManager::allowVanityMode(bool allow)
    {
        mCamera->allowVanityMode(allow);
    }

    void RenderingManager::togglePlayerLooking(bool enable)
    {
        mCamera->togglePlayerLooking(enable);
    }

    void RenderingManager::changeVanityModeScale(float factor)
    {
        if(mCamera->isVanityOrPreviewModeEnabled())
            mCamera->setCameraDistance(-factor/120.f*10, true, true);
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


}
