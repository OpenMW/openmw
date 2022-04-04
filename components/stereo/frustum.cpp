#include "stereomanager.hpp"
#include "multiview.hpp"

#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/Texture2DMultisample>
#include <osg/Texture2DArray>
#include <osg/DisplaySettings>

#include <osgUtil/CullVisitor>
#include <osgUtil/RenderStage>

#include <osgViewer/Renderer>
#include <osgViewer/Viewer>

#include <iostream>
#include <map>
#include <string>

#include <components/debug/debuglog.hpp>

#include <components/sceneutil/statesetupdater.hpp>
#include <components/sceneutil/visitor.hpp>
#include <components/sceneutil/util.hpp>
#include <components/sceneutil/depth.hpp>
#include <components/sceneutil/color.hpp>
#include <components/sceneutil/mwshadowtechnique.hpp>

#include <components/settings/settings.hpp>

#include <components/misc/stringops.hpp>
#include "frustum.hpp"

namespace Stereo
{
#ifdef OSG_HAS_MULTIVIEW
    struct MultiviewFrustumCallback final : public osg::CullSettings::InitialFrustumCallback
    {
        MultiviewFrustumCallback(StereoFrustumManager* sfm)
            : mSfm(sfm)
        {

        }

        void setInitialFrustum(osg::CullStack& cullStack, osg::Polytope& frustum) const override
        {
            auto cm = cullStack.getCullingMode();
            bool nearCulling = !!(cm & osg::CullSettings::NEAR_PLANE_CULLING);
            bool farCulling = !!(cm & osg::CullSettings::FAR_PLANE_CULLING);
            frustum.setToBoundingBox(mSfm->boundingBox(), nearCulling, farCulling);
        }

        StereoFrustumManager* mSfm;
    };
#endif

    struct ShadowFrustumCallback final : public SceneUtil::MWShadowTechnique::CustomFrustumCallback
    {
        ShadowFrustumCallback(StereoFrustumManager* parent) : mParent(parent)
        {
        }

        void operator()(osgUtil::CullVisitor& cv, osg::BoundingBoxd& customClipSpace, osgUtil::CullVisitor*& sharedFrustumHint) override
        {
            mParent->customFrustumCallback(cv, customClipSpace, sharedFrustumHint);
        }

        StereoFrustumManager* mParent;
    };

    void joinBoundingBoxes(const osg::Matrix& masterProjection, const osg::Matrix& slaveProjection, osg::BoundingBoxd& bb)
    {
        static const std::array<osg::Vec3d, 8> clipCorners = {{
            {-1.0, -1.0, -1.0},
            { 1.0, -1.0, -1.0},
            { 1.0, -1.0,  1.0},
            {-1.0, -1.0,  1.0},
            {-1.0,  1.0, -1.0},
            { 1.0,  1.0, -1.0},
            { 1.0,  1.0,  1.0},
            {-1.0,  1.0,  1.0}
        }};

        osg::Matrix slaveClipToView;
        slaveClipToView.invert(slaveProjection);

        for (const auto& clipCorner : clipCorners)
        {
            auto masterViewVertice = clipCorner * slaveClipToView;
            auto masterClipVertice = masterViewVertice * masterProjection;
            bb.expandBy(masterClipVertice);
        }
    }

    StereoFrustumManager::StereoFrustumManager(osg::Camera* camera)
        : mCamera(camera)
        , mShadowTechnique(nullptr)
        , mShadowFrustumCallback(nullptr)
    {
        if (Stereo::getMultiview())
        {
#ifdef OSG_HAS_MULTIVIEW
            mMultiviewFrustumCallback = new MultiviewFrustumCallback(this);
            mCamera->setInitialFrustumCallback(mMultiviewFrustumCallback);
#endif
        }

        if (Settings::Manager::getBool("shared shadow maps", "Stereo"))
        {
            mShadowFrustumCallback = new ShadowFrustumCallback(this);
            auto* renderer = static_cast<osgViewer::Renderer*>(mCamera->getRenderer());
            for (auto* sceneView : { renderer->getSceneView(0), renderer->getSceneView(1) })
            {
                mSharedFrustums[sceneView->getCullVisitorRight()] = sceneView->getCullVisitorLeft();
            }
        }
    }

    StereoFrustumManager::~StereoFrustumManager()
    {
        if (Stereo::getMultiview())
        {
#ifdef OSG_HAS_MULTIVIEW
            mCamera->setInitialFrustumCallback(nullptr);
#endif
        }

        if (mShadowTechnique)
            mShadowTechnique->setCustomFrustumCallback(nullptr);
    }

    void StereoFrustumManager::setShadowTechnique(
        SceneUtil::MWShadowTechnique* shadowTechnique)
    {
        if (mShadowTechnique)
            mShadowTechnique->setCustomFrustumCallback(nullptr);
        mShadowTechnique = shadowTechnique;
        if (mShadowTechnique)
            mShadowTechnique->setCustomFrustumCallback(mShadowFrustumCallback);
    }

    void StereoFrustumManager::customFrustumCallback(
        osgUtil::CullVisitor& cv, 
        osg::BoundingBoxd& customClipSpace,
        osgUtil::CullVisitor*& sharedFrustumHint)
    {
        auto it = mSharedFrustums.find(&cv);
        if (it != mSharedFrustums.end())
        {
            sharedFrustumHint = it->second;
        }

        customClipSpace = mBoundingBox;
    }

    void StereoFrustumManager::update(std::array<osg::Matrix, 2> projections)
    {
        mBoundingBox.init();
        for (auto& projection : projections)
            joinBoundingBoxes(mCamera->getProjectionMatrix(), projection, mBoundingBox);
    }
}
