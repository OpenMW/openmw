#include "renderingmanager.hpp"

#include <stdexcept>

#include <osg/io_utils>
#include <osg/Light>
#include <osg/LightModel>
#include <osg/Fog>
#include <osg/Group>
#include <osg/PositionAttitudeTransform>

#include <osgUtil/IncrementalCompileOperation>

#include <osgViewer/Viewer>

#include <components/resource/resourcesystem.hpp>

#include <components/settings/settings.hpp>

#include <components/sceneutil/util.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/statesetupdater.hpp>

#include <components/esm/loadcell.hpp>

#include "../mwbase/world.hpp"

#include "sky.hpp"
#include "effectmanager.hpp"
#include "npcanimation.hpp"
#include "vismask.hpp"
#include "pathgrid.hpp"

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
            stateset->setAttributeAndModes(fog, osg::StateAttribute::ON);
        }

        virtual void apply(osg::StateSet* stateset, osg::NodeVisitor*)
        {
            osg::LightModel* lightModel = static_cast<osg::LightModel*>(stateset->getAttribute(osg::StateAttribute::LIGHTMODEL));
            lightModel->setAmbientIntensity(mAmbientColor);
            osg::Fog* fog = static_cast<osg::Fog*>(stateset->getAttribute(osg::StateAttribute::FOG));
            fog->setColor(mFogColor);
            fog->setEnd(mFogEnd);
            fog->setMode(osg::Fog::LINEAR);
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

    RenderingManager::RenderingManager(osgViewer::Viewer &viewer, osg::ref_ptr<osg::Group> rootNode, Resource::ResourceSystem* resourceSystem)
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

        mViewer.setIncrementalCompileOperation(new osgUtil::IncrementalCompileOperation);

        mObjects->setIncrementalCompileOperation(mViewer.getIncrementalCompileOperation());

        mEffectManager.reset(new EffectManager(mRootNode, mResourceSystem));

        mViewer.setLightingMode(osgViewer::View::NO_LIGHT);

        osg::ref_ptr<osg::LightSource> source = new osg::LightSource;
        mSunLight = new osg::Light;
        source->setLight(mSunLight);
        mSunLight->setDiffuse(osg::Vec4f(0,0,0,1));
        mSunLight->setAmbient(osg::Vec4f(0,0,0,1));
        mSunLight->setConstantAttenuation(1.f);
        lightRoot->addChild(source);

        lightRoot->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
        lightRoot->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::ON);
        lightRoot->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);

        lightRoot->setNodeMask(Mask_Scene);

        mSky.reset(new SkyManager(lightRoot, resourceSystem->getSceneManager()));

        source->setStateSetModes(*mRootNode->getOrCreateStateSet(), osg::StateAttribute::ON);

        mStateUpdater = new StateUpdater;
        lightRoot->addUpdateCallback(mStateUpdater);

        osg::Camera::CullingMode cullingMode = osg::Camera::DEFAULT_CULLING|osg::Camera::FAR_PLANE_CULLING;

        // for consistent benchmarks against the ogre branch. remove later
        cullingMode &= ~(osg::CullStack::SMALL_FEATURE_CULLING);

        viewer.getCamera()->setCullingMode( cullingMode );

        mViewer.getCamera()->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
        mViewer.getCamera()->setCullingMode(cullingMode);

        mViewDistance = Settings::Manager::getFloat("viewing distance", "Viewing distance");

        double fovy, aspect, zNear, zFar;
        mViewer.getCamera()->getProjectionMatrixAsPerspective(fovy, aspect, zNear, zFar);
        fovy = 55.f;
        zNear = 5.f;
        zFar = mViewDistance;
        mViewer.getCamera()->setProjectionMatrixAsPerspective(fovy, aspect, zNear, zFar);
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
        mSunLight->setDiffuse(colour);
    }

    void RenderingManager::setSunDirection(const osg::Vec3f &direction)
    {
        mSunLight->setDirection(direction*-1);

        mSky->setSunDirection(direction*-1);
    }

    osg::Vec3f RenderingManager::getEyePos()
    {
        osg::Vec3d eye = mViewer.getCameraManipulator()->getMatrix().getTrans();
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
        mViewer.getCamera()->setClearColor(colour);

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
    }

    void RenderingManager::rotateObject(const MWWorld::Ptr &ptr, const osg::Quat& rot)
    {
        //if(ptr.getRefData().getHandle() == mCamera->getHandle() &&
        //   !mCamera->isVanityOrPreviewModeEnabled())
        //    mCamera->rotateCamera(-rot, false);

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
            mLightRoot->addChild(mPlayerNode);
        }

        player.getRefData().setBaseNode(mPlayerNode);

        //attachCameraTo(player);
    }

    void RenderingManager::renderPlayer(const MWWorld::Ptr &player)
    {
        mPlayerAnimation.reset(new NpcAnimation(player, player.getRefData().getBaseNode(), mResourceSystem, 0));

        //mCamera->setAnimation(mPlayerAnimation);
        //mWater->removeEmitter(ptr);
        //mWater->addEmitter(ptr);
    }

}
