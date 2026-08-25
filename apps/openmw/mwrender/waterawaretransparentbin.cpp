#include "waterawaretransparentbin.hpp"

#include <osg/BoundingBox>
#include <osg/ClipPlane>
#include <osg/Plane>
#include <osg/State>
#include <osg/StateSet>
#include <osgUtil/RenderStage>
#include <osgUtil/StateGraph>

#include <components/misc/constants.hpp>

#include "opaqueblit.hpp"
#include "water.hpp"

namespace
{
    osg::BoundingBox worldBound(const osgUtil::RenderLeaf* leaf, const osg::Matrixd& inverseView)
    {
        const osg::Matrixd localToWorld = *leaf->_modelview * inverseView;
        const osg::BoundingBox& localBound = leaf->_drawable->getBoundingBox();

        osg::BoundingBox bound;
        for (unsigned int i = 0; i < 8; ++i)
            bound.expandBy(localBound.corner(i) * localToWorld);

        return bound;
    }
}

namespace MWRender
{
    WaterAwareTransparentBin::WaterAwareTransparentBin(OpaqueColorBinCallback* opaqueColorResolve, const Water* water)
        : osgUtil::RenderBin(osgUtil::RenderBin::SORT_BACK_TO_FRONT)
        , mOpaqueColorResolve(opaqueColorResolve)
        , mClipStateSet(new osg::StateSet)
        , mWater(water)
    {
    }

    WaterAwareTransparentBin::WaterAwareTransparentBin(const WaterAwareTransparentBin& rhs, const osg::CopyOp& copyop)
        : osgUtil::RenderBin(rhs, copyop)
        , mOpaqueColorResolve(rhs.mOpaqueColorResolve)
        , mClipStateSet(new osg::StateSet)
        , mWater(rhs.mWater)
    {
    }

    bool WaterAwareTransparentBin::isSceneCamera() const
    {
        const osg::Camera* camera = getStage()->getCamera();

        return camera && camera->getName() == Constants::SceneCamera;
    }

    void WaterAwareTransparentBin::sortImplementation()
    {
        osgUtil::RenderBin::sortBackToFront();

        if (!isSceneCamera())
            return;

        mUnderwaterLeaves.clear();
        mWaterLeaves.clear();
        mAboveWaterLeaves.clear();
        mStraddlingLeaves.clear();

        const osg::Matrixd inverseView = osg::Matrixd::inverse(*getStage()->getInitialViewMatrix());
        const double waterHeight = mWater->getHeight();
        const osg::Vec3d cameraPosition = osg::Vec3d() * inverseView;

        mCameraUnderwater = cameraPosition.z() < waterHeight;

        for (osgUtil::RenderLeaf* leaf : _renderLeafList)
        {
            if (!leaf || !leaf->_drawable || !leaf->_modelview)
                continue;

            if (leaf->_drawable == mWater->getDrawable())
                mWaterLeaves.push_back(leaf);
            else
            {
                const osg::BoundingBox bound = worldBound(leaf, inverseView);

                if (bound.zMax() < waterHeight)
                    mUnderwaterLeaves.push_back(leaf);
                else if (bound.zMin() > waterHeight)
                    mAboveWaterLeaves.push_back(leaf);
                else
                    mStraddlingLeaves.push_back(leaf);
            }
        }

        if (mWaterLeaves.empty())
        {
            mCameraUnderwater = false;
            mUnderwaterLeaves.clear();
            mStraddlingLeaves.clear();
            mAboveWaterLeaves = _renderLeafList;
        }
    }

    void WaterAwareTransparentBin::drawImplementation(osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous)
    {
        // Fallback to standard draw order for anything other then the scene camera and when there is no water plane
        if (!isSceneCamera() || mWaterLeaves.empty())
        {
            osgUtil::RenderBin::drawImplementation(renderInfo, previous);
            return;
        }

        const RenderLeafList& farSideLeaves = mCameraUnderwater ? mAboveWaterLeaves : mUnderwaterLeaves;
        const RenderLeafList& nearSideLeaves = mCameraUnderwater ? mUnderwaterLeaves : mAboveWaterLeaves;

        const osg::Matrixd inverseView = osg::Matrixd::inverse(*getStage()->getInitialViewMatrix());
        const double waterHeight = mWater->getHeight();

        // Does a good job at hiding blending artifacts at the water plane intersection, though it's not perfect
        constexpr double clipOffset = 4.0;
        const double clipHeight = waterHeight + (mCameraUnderwater ? clipOffset : -clipOffset);

        const double direction = mCameraUnderwater ? -1.0 : 1.0;

        osg::Plane nearSidePlane(0.0, 0.0, direction, -direction * clipHeight);
        osg::Plane farSidePlane(0.0, 0.0, -direction, direction * clipHeight);

        drawLeavesWithClippedStraddlers(
            farSideLeaves, mStraddlingLeaves, renderInfo, previous, farSidePlane, inverseView, false);

        if (!farSideLeaves.empty() || !mStraddlingLeaves.empty())
            mOpaqueColorResolve->resolve(this, renderInfo);

        for (osgUtil::RenderLeaf* leaf : mWaterLeaves)
        {
            leaf->render(renderInfo, previous);
            previous = leaf;
        }

        drawLeavesWithClippedStraddlers(
            nearSideLeaves, mStraddlingLeaves, renderInfo, previous, nearSidePlane, inverseView, true);
    }

    void WaterAwareTransparentBin::drawLeavesWithClippedStraddlers(const osgUtil::RenderBin::RenderLeafList& leaves,
        const osgUtil::RenderBin::RenderLeafList& straddlingLeaves, osg::RenderInfo& renderInfo,
        osgUtil::RenderLeaf*& previous, const osg::Plane& worldPlane, const osg::Matrixd& inverseView,
        bool decrementStraddlingDynamicObjects)
    {
        if (leaves.empty() && straddlingLeaves.empty())
            return;

        osg::State& state = *renderInfo.getState();

        osg::Plane viewPlane = worldPlane;
        viewPlane.transformProvidingInverse(inverseView);
        mClipStateSet->setAttributeAndModes(new osg::ClipPlane(1, viewPlane),
            osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE | osg::StateAttribute::PROTECTED);

        if (previous)
            osgUtil::StateGraph::moveToRootStateGraph(state, previous->_parent->_parent);

        state.applyModelViewMatrix(osg::Matrixd::identity());
        state.pushStateSet(mClipStateSet);
        state.apply();

        previous = nullptr;

        std::size_t leafIndex = 0;
        std::size_t straddlingIndex = 0;

        while (leafIndex < leaves.size() || straddlingIndex < straddlingLeaves.size())
        {
            const bool drawStraddling = straddlingIndex < straddlingLeaves.size()
                && (leafIndex == leaves.size()
                    || straddlingLeaves[straddlingIndex]->_depth >= leaves[leafIndex]->_depth);

            osgUtil::RenderLeaf* leaf;
            if (drawStraddling)
                leaf = straddlingLeaves[straddlingIndex++];
            else
                leaf = leaves[leafIndex++];

            const bool dynamic = leaf->_dynamic;
            if (drawStraddling && !decrementStraddlingDynamicObjects)
                leaf->_dynamic = false;

            leaf->render(renderInfo, previous);

            leaf->_dynamic = dynamic;
            previous = leaf;
        }

        osgUtil::StateGraph::moveToRootStateGraph(state, previous->_parent->_parent);
        state.popStateSet();
        state.apply();

        osgUtil::StateGraph::moveStateGraph(state, nullptr, previous->_parent->_parent);
        state.apply(previous->_parent->getStateSet());
    }
}
