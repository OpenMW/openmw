#include "waterawaretransparentbin.hpp"

#include <algorithm>

#include <osg/BoundingSphere>
#include <osg/StateSet>
#include <osgUtil/RenderStage>

#include "opaqueblit.hpp"

namespace
{
    osg::Vec3d worldCenter(const osgUtil::RenderLeaf* leaf, const osg::Matrixd& inverseView)
    {
        const osg::Matrixd localToWorld = *leaf->_modelview * inverseView;
        return osg::Vec3d(leaf->_drawable->getBound().center()) * localToWorld;
    }

    void sortLeavesBackToFront(osgUtil::RenderBin::RenderLeafList& leaves)
    {
        std::stable_sort(leaves.begin(), leaves.end(),
            [](const auto* left, const auto* right) { return left->_depth > right->_depth; });
    }

    void drawLeaves(
        const osgUtil::RenderBin::RenderLeafList& leaves, osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous)
    {
        for (osgUtil::RenderLeaf* leaf : leaves)
        {
            leaf->render(renderInfo, previous);
            previous = leaf;
        }
    }
}

namespace MWRender
{
    bool isWaterSurface(const osgUtil::RenderLeaf& leaf)
    {
        for (const osgUtil::StateGraph* stateGraph = leaf._parent; stateGraph; stateGraph = stateGraph->_parent)
        {
            const osg::StateSet* stateSet = stateGraph->getStateSet();
            if (stateSet && stateSet->getUniform("waterSurface"))
                return true;
        }
        return false;
    }

    WaterAwareTransparentBin::WaterAwareTransparentBin(OpaqueColorBinCallback* opaqueColorResolve)
        : osgUtil::RenderBin(osgUtil::RenderBin::SORT_BACK_TO_FRONT)
        , mOpaqueColorResolve(opaqueColorResolve)
    {
    }

    WaterAwareTransparentBin::WaterAwareTransparentBin(const WaterAwareTransparentBin& rhs, const osg::CopyOp& copyop)
        : osgUtil::RenderBin(rhs, copyop)
        , mOpaqueColorResolve(rhs.mOpaqueColorResolve)
    {
    }

    void WaterAwareTransparentBin::sortImplementation()
    {
        osgUtil::RenderBin::sortBackToFront();

        mUnderwaterLeaves.clear();
        mWaterLeaves.clear();
        mAboveWaterLeaves.clear();

        const osgUtil::RenderLeaf* waterLeaf = nullptr;
        for (const auto& leaf : _renderLeafList)
        {
            if (isWaterSurface(*leaf))
            {
                waterLeaf = leaf;
                break;
            }
        }

        if (!waterLeaf)
        {
            mCameraUnderwater = false;
            mAboveWaterLeaves = _renderLeafList;
            sortLeavesBackToFront(mAboveWaterLeaves);
            return;
        }

        const osg::Matrixd inverseView = osg::Matrixd::inverse(*getStage()->getInitialViewMatrix());
        const double waterHeight = worldCenter(waterLeaf, inverseView).z();
        const osg::Vec3d cameraPosition = osg::Vec3d() * inverseView;
        mCameraUnderwater = cameraPosition.z() < waterHeight;
        for (const auto& leaf : _renderLeafList)
        {
            if (isWaterSurface(*leaf))
                mWaterLeaves.push_back(leaf);
            else if (worldCenter(leaf, inverseView).z() < waterHeight)
                mUnderwaterLeaves.push_back(leaf);
            else
                mAboveWaterLeaves.push_back(leaf);
        }

        sortLeavesBackToFront(mUnderwaterLeaves);
        sortLeavesBackToFront(mWaterLeaves);
        sortLeavesBackToFront(mAboveWaterLeaves);
    }

    void WaterAwareTransparentBin::drawImplementation(osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous)
    {
        const RenderLeafList& farSideLeaves = mCameraUnderwater ? mAboveWaterLeaves : mUnderwaterLeaves;
        const RenderLeafList& nearSideLeaves = mCameraUnderwater ? mUnderwaterLeaves : mAboveWaterLeaves;

        drawLeaves(farSideLeaves, renderInfo, previous);

        if (!mWaterLeaves.empty() && !farSideLeaves.empty())
            mOpaqueColorResolve->resolve(this, renderInfo);

        drawLeaves(mWaterLeaves, renderInfo, previous);
        drawLeaves(nearSideLeaves, renderInfo, previous);
    }
}
