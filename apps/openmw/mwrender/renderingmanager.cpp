#include "renderingmanager.hpp"

#include <stdexcept>
#include <limits>

#include <osg/Light>
#include <osg/LightModel>
#include <osg/Fog>
#include <osg/PolygonMode>
#include <osg/Group>
#include <osg/PositionAttitudeTransform>
#include <osg/UserDataContainer>
#include <osg/ComputeBoundsVisitor>

#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/IncrementalCompileOperation>

#include <osgViewer/Viewer>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/texturemanager.hpp>
#include <components/resource/scenemanager.hpp>

#include <components/settings/settings.hpp>

#include <components/sceneutil/util.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/statesetupdater.hpp>

#include <components/terrain/terraingrid.hpp>

#include <components/esm/loadcell.hpp>

#include "../mwworld/fallback.hpp"
#include "../mwworld/cellstore.hpp"

#include "sky.hpp"
#include "effectmanager.hpp"
#include "npcanimation.hpp"
#include "vismask.hpp"
#include "pathgrid.hpp"
#include "camera.hpp"
#include "water.hpp"
#include "terrainstorage.hpp"

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

    RenderingManager::RenderingManager(osgViewer::Viewer* viewer, osg::ref_ptr<osg::Group> rootNode, Resource::ResourceSystem* resourceSystem,
                                       const MWWorld::Fallback* fallback, const std::string& resourcePath)
        : mViewer(viewer)
        , mRootNode(rootNode)
        , mResourceSystem(resourceSystem)
        , mFogDepth(0.f)
        , mUnderwaterColor(fallback->getFallbackColour("Water_UnderwaterColor"))
        , mUnderwaterWeight(fallback->getFallbackFloat("Water_UnderwaterColorWeight"))
        , mUnderwaterFog(0.f)
        , mUnderwaterIndoorFog(fallback->getFallbackFloat("Water_UnderwaterIndoorFog"))
        , mNightEyeFactor(0.f)
    {
        resourceSystem->getSceneManager()->setParticleSystemMask(MWRender::Mask_ParticleSystem);

        osg::ref_ptr<SceneUtil::LightManager> lightRoot = new SceneUtil::LightManager;
        lightRoot->setLightingMask(Mask_Lighting);
        mLightRoot = lightRoot;
        lightRoot->setStartLight(1);

        mRootNode->addChild(lightRoot);

        mPathgrid.reset(new Pathgrid(mRootNode));

        mObjects.reset(new Objects(mResourceSystem, lightRoot));

        mViewer->setIncrementalCompileOperation(new osgUtil::IncrementalCompileOperation);

        mResourceSystem->getSceneManager()->setIncrementalCompileOperation(mViewer->getIncrementalCompileOperation());

        mEffectManager.reset(new EffectManager(lightRoot, mResourceSystem));

        mWater.reset(new Water(mRootNode, lightRoot, mResourceSystem, mViewer->getIncrementalCompileOperation(), fallback, resourcePath));

        mTerrain.reset(new Terrain::TerrainGrid(lightRoot, mResourceSystem, mViewer->getIncrementalCompileOperation(),
                                                new TerrainStorage(mResourceSystem->getVFS(), false), Mask_Terrain));

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
        lightRoot->addChild(source);

        lightRoot->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
        lightRoot->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::ON);
        lightRoot->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);

        lightRoot->setNodeMask(Mask_Scene);
        lightRoot->setName("Scene Root");

        mSky.reset(new SkyManager(lightRoot, resourceSystem->getSceneManager()));

        source->setStateSetModes(*mRootNode->getOrCreateStateSet(), osg::StateAttribute::ON);

        mStateUpdater = new StateUpdater;
        lightRoot->addUpdateCallback(mStateUpdater);

        osg::Camera::CullingMode cullingMode = osg::Camera::DEFAULT_CULLING|osg::Camera::FAR_PLANE_CULLING;

        if (!Settings::Manager::getBool("small feature culling", "Camera"))
            cullingMode &= ~(osg::CullStack::SMALL_FEATURE_CULLING);
        else
            cullingMode |= osg::CullStack::SMALL_FEATURE_CULLING;

        mViewer->getCamera()->setCullingMode( cullingMode );

        mViewer->getCamera()->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
        mViewer->getCamera()->setCullingMode(cullingMode);

        mViewer->getCamera()->setCullMask(~(Mask_UpdateVisitor|Mask_SimpleWater));

        mNearClip = Settings::Manager::getFloat("near clip", "Camera");
        mViewDistance = Settings::Manager::getFloat("viewing distance", "Camera");
        mFieldOfView = Settings::Manager::getFloat("field of view", "General");
        updateProjectionMatrix();
        mStateUpdater->setFogEnd(mViewDistance);

        mRootNode->getOrCreateStateSet()->addUniform(new osg::Uniform("near", mNearClip));
        mRootNode->getOrCreateStateSet()->addUniform(new osg::Uniform("far", mViewDistance));
    }

    RenderingManager::~RenderingManager()
    {
    }

    MWRender::Objects& RenderingManager::getObjects()
    {
        return *mObjects.get();
    }

    Resource::ResourceSystem* RenderingManager::getResourceSystem()
    {
        return mResourceSystem;
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

        mSunLight->setDiffuse(SceneUtil::colourFromRGB(cell->mAmbi.mSunlight));
        mSunLight->setDirection(osg::Vec3f(1.f,-1.f,-1.f));
    }

    void RenderingManager::setSunColour(const osg::Vec4f &colour)
    {
        // need to wrap this in a StateUpdater?
        mSunLight->setDiffuse(colour);
        mSunLight->setSpecular(colour);
    }

    void RenderingManager::setSunDirection(const osg::Vec3f &direction)
    {
        osg::Vec3 position = direction * -1;
        // need to wrap this in a StateUpdater?
        mSunLight->setPosition(osg::Vec4(position.x(), position.y(), position.z(), 0));

        mSky->setSunDirection(position);
    }

    osg::Vec3f RenderingManager::getEyePos()
    {
        osg::Vec3d eye = mViewer->getCameraManipulator()->getMatrix().getTrans();
        return eye;
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
        /*
        else //if (mode == Render_BoundingBoxes)
        {
            bool show = !mRendering.getScene()->getShowBoundingBoxes();
            mRendering.getScene()->showBoundingBoxes(show);
            return show;
        }
        */
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
        if (!paused)
        {
            mEffectManager->update(dt);
            mSky->update(dt);
        }

        mWater->update(dt);
        mCamera->update(dt, paused);

        osg::Vec3f focal, cameraPos;
        mCamera->getPosition(focal, cameraPos);
        if (mWater->isUnderwater(cameraPos))
        {
            setFogColor(mUnderwaterColor * mUnderwaterWeight + mFogColor * (1.f-mUnderwaterWeight));
            mStateUpdater->setFogStart(mViewDistance * (1 - mUnderwaterFog));
            mStateUpdater->setFogEnd(mViewDistance);
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
            mPlayerAnimation->updatePtr(ptr);

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


    class NoTraverseCallback : public osg::NodeCallback
    {
    public:
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
        }
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
        rttCamera->addChild(mLightRoot);
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
        rttCamera->setGraphicsContext(NULL);
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
        if (intersector->containsIntersections())
        {
            result.mHit = true;
            osgUtil::LineSegmentIntersector::Intersection intersection = intersector->getFirstIntersection();

            result.mHitPointWorld = intersection.getWorldIntersectPoint();
            result.mHitNormalWorld = intersection.getWorldIntersectNormal();

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

    osg::ref_ptr<osgUtil::IntersectionVisitor> createIntersectionVisitor(osgUtil::Intersector* intersector, bool ignorePlayer, bool ignoreActors)
    {
        osg::ref_ptr<osgUtil::IntersectionVisitor> intersectionVisitor( new osgUtil::IntersectionVisitor(intersector));
        int mask = intersectionVisitor->getTraversalMask();
        mask &= ~(Mask_RenderToTexture|Mask_Sky|Mask_Debug|Mask_Effect|Mask_Water|Mask_SimpleWater);
        if (ignorePlayer)
            mask &= ~(Mask_Player);
        if (ignoreActors)
            mask &= ~(Mask_Actor|Mask_Player);

        intersectionVisitor->setTraversalMask(mask);
        return intersectionVisitor;
    }

    RenderingManager::RayResult RenderingManager::castRay(const osg::Vec3f& origin, const osg::Vec3f& dest, bool ignorePlayer, bool ignoreActors)
    {
        osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector (new osgUtil::LineSegmentIntersector(osgUtil::LineSegmentIntersector::MODEL,
            origin, dest));
        intersector->setIntersectionLimit(osgUtil::LineSegmentIntersector::LIMIT_NEAREST);

        mRootNode->accept(*createIntersectionVisitor(intersector, ignorePlayer, ignoreActors));

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

        mViewer->getCamera()->accept(*createIntersectionVisitor(intersector, ignorePlayer, ignoreActors));

        return getIntersectionResult(intersector);
    }

    void RenderingManager::updatePtr(const MWWorld::Ptr &old, const MWWorld::Ptr &updated)
    {
        mObjects->updatePtr(old, updated);
    }

    void RenderingManager::spawnEffect(const std::string &model, const std::string &texture, const osg::Vec3f &worldPosition, float scale)
    {
        mEffectManager->addEffect(model, texture, worldPosition, scale);
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
    }

    MWRender::Animation* RenderingManager::getAnimation(const MWWorld::Ptr &ptr)
    {
        if (mPlayerAnimation.get() && ptr == mPlayerAnimation->getPtr())
            return mPlayerAnimation.get();

        return mObjects->getAnimation(ptr);
    }

    MWRender::Animation* RenderingManager::getPlayerAnimation()
    {
        return mPlayerAnimation.get();
    }

    void RenderingManager::setupPlayer(const MWWorld::Ptr &player)
    {
        if (!mPlayerNode)
        {
            mPlayerNode = new osg::PositionAttitudeTransform;
            mPlayerNode->setNodeMask(Mask_Player);
            mLightRoot->addChild(mPlayerNode);
        }

        mPlayerNode->setUserDataContainer(new osg::DefaultUserDataContainer);
        mPlayerNode->getUserDataContainer()->addUserObject(new PtrHolder(player));

        player.getRefData().setBaseNode(mPlayerNode);

        mWater->addEmitter(player);
    }

    void RenderingManager::renderPlayer(const MWWorld::Ptr &player)
    {
        mPlayerAnimation.reset(new NpcAnimation(player, player.getRefData().getBaseNode(), mResourceSystem, 0));

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

    void RenderingManager::updateProjectionMatrix()
    {
        double aspect = mViewer->getCamera()->getViewport()->aspectRatio();
        mViewer->getCamera()->setProjectionMatrixAsPerspective(mFieldOfView, aspect, mNearClip, mViewDistance);
    }

    void RenderingManager::updateTextureFiltering()
    {
        osg::Texture::FilterMode min = osg::Texture::LINEAR_MIPMAP_NEAREST;
        osg::Texture::FilterMode mag = osg::Texture::LINEAR;

        if (Settings::Manager::getString("texture filtering", "General") == "trilinear")
            min = osg::Texture::LINEAR_MIPMAP_LINEAR;

        int maxAnisotropy = Settings::Manager::getInt("anisotropy", "General");

        mViewer->stopThreading();
        mResourceSystem->getTextureManager()->setFilterSettings(min, mag, maxAnisotropy);
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

    void RenderingManager::processChangedSettings(const Settings::CategorySettingVector &changed)
    {
        for (Settings::CategorySettingVector::const_iterator it = changed.begin(); it != changed.end(); ++it)
        {
            if (it->first == "General" && it->second == "field of view")
            {
                mFieldOfView = Settings::Manager::getFloat("field of view", "General");
                updateProjectionMatrix();
            }
            else if (it->first == "Camera" && it->second == "viewing distance")
            {
                mViewDistance = Settings::Manager::getFloat("viewing distance", "Camera");
                mStateUpdater->setFogEnd(mViewDistance);
                updateProjectionMatrix();
            }
            else if (it->first == "General" && (it->second == "texture filtering" || it->second == "anisotropy"))
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

}
