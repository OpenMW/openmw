#include "renderingmanager.hpp"

#include <stdexcept>

#include <osg/io_utils>
#include <osg/Light>
#include <osg/LightModel>
#include <osg/Group>

#include <osgViewer/Viewer>

#include <components/sceneutil/util.hpp>

#include <components/sceneutil/lightmanager.hpp>

#include <components/esm/loadcell.hpp>

namespace MWRender
{

    RenderingManager::RenderingManager(osgViewer::Viewer &viewer, osg::ref_ptr<osg::Group> rootNode, Resource::ResourceSystem* resourceSystem)
        : mViewer(viewer)
        , mRootNode(rootNode)
        , mResourceSystem(resourceSystem)
    {
        osg::ref_ptr<SceneUtil::LightManager> lightRoot = new SceneUtil::LightManager;
        lightRoot->setStartLight(1);

        mRootNode->addChild(lightRoot);

        mObjects.reset(new Objects(mResourceSystem, lightRoot));

        mViewer.setLightingMode(osgViewer::View::NO_LIGHT);

        osg::ref_ptr<osg::LightSource> source = new osg::LightSource;
        mSunLight = new osg::Light;
        source->setLight(mSunLight);
        mSunLight->setDiffuse(osg::Vec4f(0,0,0,1));
        mSunLight->setAmbient(osg::Vec4f(0,0,0,1));
        mSunLight->setConstantAttenuation(1.f);
        source->setStateSetModes(*rootNode->getOrCreateStateSet(), osg::StateAttribute::ON);
        lightRoot->addChild(source);

        rootNode->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
        rootNode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::ON);
        rootNode->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);

        // for consistent benchmarks against the ogre branch. remove later
        osg::CullStack::CullingMode cullingMode = viewer.getCamera()->getCullingMode();
        cullingMode &= ~(osg::CullStack::SMALL_FEATURE_CULLING);
        viewer.getCamera()->setCullingMode( cullingMode );
    }

    MWRender::Objects& RenderingManager::getObjects()
    {
        return *mObjects.get();
    }

    MWRender::Actors& RenderingManager::getActors()
    {
        throw std::runtime_error("unimplemented");
    }

    Resource::ResourceSystem* RenderingManager::getResourceSystem()
    {
        return mResourceSystem;
    }

    void RenderingManager::configureAmbient(const ESM::Cell *cell)
    {
        osg::ref_ptr<osg::LightModel> lightmodel = new osg::LightModel;
        lightmodel->setAmbientIntensity(SceneUtil::colourFromRGB(cell->mAmbi.mAmbient));
        mRootNode->getOrCreateStateSet()->setAttributeAndModes(lightmodel, osg::StateAttribute::ON);

        mSunLight->setDiffuse(SceneUtil::colourFromRGB(cell->mAmbi.mSunlight));
        mSunLight->setDirection(osg::Vec3f(1.f,-1.f,-1.f));
    }

    osg::Vec3f RenderingManager::getEyePos()
    {
        osg::Vec3d eye;
        //mViewer.getCamera()->getViewMatrixAsLookAt(eye, center, up);
        eye = mViewer.getCameraManipulator()->getMatrix().getTrans();
        return eye;
    }

    void RenderingManager::removeCell(const MWWorld::CellStore *store)
    {
        mObjects->removeCell(store);
    }

}
