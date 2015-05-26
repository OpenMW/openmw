#include "renderingmanager.hpp"

#include <stdexcept>

#include <osg/Light>
#include <osg/LightModel>
#include <osg/Fog>
#include <osg/Group>
#include <osg/PositionAttitudeTransform>
#include <osg/UserDataContainer>
#include <osg/ComputeBoundsVisitor>

#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/IncrementalCompileOperation>

#include <osgViewer/Viewer>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/texturemanager.hpp>

#include <components/settings/settings.hpp>

#include <components/sceneutil/util.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/statesetupdater.hpp>

#include <components/esm/loadcell.hpp>

#include "sky.hpp"
#include "effectmanager.hpp"
#include "npcanimation.hpp"
#include "vismask.hpp"
#include "pathgrid.hpp"
#include "camera.hpp"

namespace MWRender
{

    class StateUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        StateUpdater()
            : mFogEnd(0.f)
        {
        }

        virtual void setDefaults(osg::StateSet *stateset)
        {
            osg::LightModel* lightModel = new osg::LightModel;
            stateset->setAttribute(lightModel, osg::StateAttribute::ON);
            osg::Fog* fog = new osg::Fog;
            fog->setStart(1);
            fog->setMode(osg::Fog::LINEAR);
            stateset->setAttributeAndModes(fog, osg::StateAttribute::ON);
        }

        virtual void apply(osg::StateSet* stateset, osg::NodeVisitor*)
        {
            osg::LightModel* lightModel = static_cast<osg::LightModel*>(stateset->getAttribute(osg::StateAttribute::LIGHTMODEL));
            lightModel->setAmbientIntensity(mAmbientColor);
            osg::Fog* fog = static_cast<osg::Fog*>(stateset->getAttribute(osg::StateAttribute::FOG));
            fog->setColor(mFogColor);
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

        void setFogEnd(float end)
        {
            mFogEnd = end;
        }

    private:
        osg::Vec4f mAmbientColor;
        osg::Vec4f mFogColor;
        float mFogEnd;
    };

    RenderingManager::RenderingManager(osgViewer::Viewer* viewer, osg::ref_ptr<osg::Group> rootNode, Resource::ResourceSystem* resourceSystem)
        : mViewer(viewer)
        , mRootNode(rootNode)
        , mResourceSystem(resourceSystem)
    {
        osg::ref_ptr<SceneUtil::LightManager> lightRoot = new SceneUtil::LightManager;
        mLightRoot = lightRoot;
        lightRoot->setStartLight(1);

        mRootNode->addChild(lightRoot);

        mPathgrid.reset(new Pathgrid(mRootNode));

        mObjects.reset(new Objects(mResourceSystem, lightRoot));

        mViewer->setIncrementalCompileOperation(new osgUtil::IncrementalCompileOperation);

        mObjects->setIncrementalCompileOperation(mViewer->getIncrementalCompileOperation());

        mEffectManager.reset(new EffectManager(mRootNode, mResourceSystem));

        mCamera.reset(new Camera(mViewer->getCamera()));

        mViewer->setLightingMode(osgViewer::View::NO_LIGHT);

        osg::ref_ptr<osg::LightSource> source = new osg::LightSource;
        source->setNodeMask(SceneUtil::Mask_Lit);
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

        mViewer->getCamera()->setCullMask(~(Mask_UpdateVisitor));

        mNearClip = Settings::Manager::getFloat("near clip", "Camera");
        mViewDistance = Settings::Manager::getFloat("viewing distance", "Camera");
        mFieldOfView = Settings::Manager::getFloat("field of view", "General");
        updateProjectionMatrix();
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

    void RenderingManager::setAmbientColour(const osg::Vec4f &colour)
    {
        mStateUpdater->setAmbientColor(colour);
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
    }

    void RenderingManager::removeCell(const MWWorld::CellStore *store)
    {
        mPathgrid->removeCell(store);
        mObjects->removeCell(store);
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
            return false;
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

        configureFog (cell->mAmbi.mFogDensity, color);
    }

    void RenderingManager::configureFog(float /* fogDepth */, const osg::Vec4f &colour)
    {
        mViewer->getCamera()->setClearColor(colour);

        mStateUpdater->setFogColor(colour);
        mStateUpdater->setFogEnd(mViewDistance);
    }

    SkyManager* RenderingManager::getSkyManager()
    {
        return mSky.get();
    }

    void RenderingManager::update(float dt, bool paused)
    {
        mEffectManager->update(dt);
        mSky->update(dt);
        mCamera->update(dt, paused);
    }

    void RenderingManager::updatePlayerPtr(const MWWorld::Ptr &ptr)
    {
        if(mPlayerAnimation.get())
            mPlayerAnimation->updatePtr(ptr);

        mCamera->attachTo(ptr);
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
    }

    void RenderingManager::removeObject(const MWWorld::Ptr &ptr)
    {
        mObjects->removeObject(ptr);
    }

    osg::Vec4f RenderingManager::getScreenBounds(const MWWorld::Ptr& ptr)
    {
        if (!ptr.getRefData().getBaseNode())
            return osg::Vec4f();

        osg::ComputeBoundsVisitor computeBoundsVisitor;
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

    MWWorld::Ptr RenderingManager::getFacedObject(const float nX, const float nY, float maxDistance, bool ignorePlayer)
    {
        osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector (new osgUtil::LineSegmentIntersector(osgUtil::LineSegmentIntersector::PROJECTION,
                                                                                                       nX * 2.f - 1.f, nY * (-2.f) + 1.f));

        osg::Vec3d dist (0.f, 0.f, -maxDistance);

        dist = dist * mViewer->getCamera()->getProjectionMatrix();

        osg::Vec3d end = intersector->getEnd();
        end.z() = dist.z();
        intersector->setEnd(end);
        intersector->setIntersectionLimit(osgUtil::LineSegmentIntersector::LIMIT_NEAREST);

        osgUtil::IntersectionVisitor intersectionVisitor(intersector);
        int mask = intersectionVisitor.getTraversalMask();
        mask &= ~(Mask_RenderToTexture|Mask_Sky|Mask_Debug|Mask_Effect);
        if (ignorePlayer)
            mask &= ~(Mask_Player);

        intersectionVisitor.setTraversalMask(mask);

        mViewer->getCamera()->accept(intersectionVisitor);

        if (intersector->containsIntersections())
        {
            osgUtil::LineSegmentIntersector::Intersection intersection = intersector->getFirstIntersection();

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
                return ptrHolder->mPtr;
        }

        return MWWorld::Ptr();
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
        //mWater->clearRipples();
    }

    void RenderingManager::clear()
    {
        //mLocalMap->clear();
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
    }

    void RenderingManager::renderPlayer(const MWWorld::Ptr &player)
    {
        mPlayerAnimation.reset(new NpcAnimation(player, player.getRefData().getBaseNode(), mResourceSystem, 0));

        mCamera->setAnimation(mPlayerAnimation.get());
        mCamera->attachTo(player);
        //mWater->removeEmitter(ptr);
        //mWater->addEmitter(ptr);
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
        }
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
