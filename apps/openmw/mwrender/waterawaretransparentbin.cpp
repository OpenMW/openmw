#include "waterawaretransparentbin.hpp"

#include <osg/BoundingSphere>
#include <osgUtil/RenderStage>

#include "opaqueblit.hpp"
#include "water.hpp"

namespace
{
    osg::Vec3d worldCenter(const osgUtil::RenderLeaf* leaf, const osg::Matrixd& inverseView)
    {
        const osg::Matrixd localToWorld = *leaf->_modelview * inverseView;
        return osg::Vec3d(leaf->_drawable->getBound().center()) * localToWorld;
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
    WaterAwareTransparentBin::WaterAwareTransparentBin(OpaqueColorBinCallback* opaqueColorResolve, const Water* water)
        : osgUtil::RenderBin(osgUtil::RenderBin::SORT_BACK_TO_FRONT)
        , mOpaqueColorResolve(opaqueColorResolve)
        , mWater(water)
    {
    }

    WaterAwareTransparentBin::WaterAwareTransparentBin(const WaterAwareTransparentBin& rhs, const osg::CopyOp& copyop)
        : osgUtil::RenderBin(rhs, copyop)
        , mOpaqueColorResolve(rhs.mOpaqueColorResolve)
        , mWater(rhs.mWater)
    {
    }

    void WaterAwareTransparentBin::sortImplementation()
    {
        osgUtil::RenderBin::sortBackToFront();

        mUnderwaterLeaves.clear();
        mWaterLeaves.clear();
        mAboveWaterLeaves.clear();

        const osg::Matrixd inverseView = osg::Matrixd::inverse(*getStage()->getInitialViewMatrix());
        const double waterHeight = mWater->getHeight();
        const osg::Vec3d cameraPosition = osg::Vec3d() * inverseView;

        mCameraUnderwater = cameraPosition.z() < waterHeight;

        for (osgUtil::RenderLeaf* leaf : _renderLeafList)
        {
            if (leaf->_drawable.get() == mWater->getDrawable())
                mWaterLeaves.push_back(leaf);
            else if (worldCenter(leaf, inverseView).z() < waterHeight)
                mUnderwaterLeaves.push_back(leaf);
            else
                mAboveWaterLeaves.push_back(leaf);
        }

        if (mWaterLeaves.empty())
        {
            mCameraUnderwater = false;
            mUnderwaterLeaves.clear();
            mAboveWaterLeaves = _renderLeafList;
        }
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
