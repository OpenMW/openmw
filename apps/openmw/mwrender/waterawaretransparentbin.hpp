#ifndef OPENMW_MWRENDER_WATERAWARETRANSPARENTBIN_H
#define OPENMW_MWRENDER_WATERAWARETRANSPARENTBIN_H

#include <osgUtil/RenderBin>

namespace osg
{
    class StateSet;
}

namespace MWRender
{
    class OpaqueColorBinCallback;
    class Water;

    class WaterAwareTransparentBin : public osgUtil::RenderBin
    {
    public:
        WaterAwareTransparentBin(OpaqueColorBinCallback* opaqueColorResolve, const Water* water);
        WaterAwareTransparentBin(const WaterAwareTransparentBin& rhs, const osg::CopyOp& copyop);

        osg::Object* cloneType() const override { return new WaterAwareTransparentBin(mOpaqueColorResolve, mWater); }
        osg::Object* clone(const osg::CopyOp& copyop) const override
        {
            return new WaterAwareTransparentBin(*this, copyop);
        }

        void sortImplementation() override;
        void drawImplementation(osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous) override;

    private:
        void drawClippedLeaf(osgUtil::RenderLeaf* leaf, osg::RenderInfo& renderInfo, osgUtil::RenderLeaf* previous,
            const osg::Plane& worldPlane, const osg::Matrixd& inverseView, bool decrementDynamicObjectCount);

        void drawLeavesWithClippedStraddlers(const osgUtil::RenderBin::RenderLeafList& leaves,
            const osgUtil::RenderBin::RenderLeafList& straddlingLeaves, osg::RenderInfo& renderInfo,
            osgUtil::RenderLeaf*& previous, const osg::Plane& worldPlane, const osg::Matrixd& inverseView,
            bool decrementStraddlingDynamicObjects);

        bool isSceneCamera() const;

        osg::ref_ptr<OpaqueColorBinCallback> mOpaqueColorResolve;
        osgUtil::RenderBin::RenderLeafList mUnderwaterLeaves;
        osgUtil::RenderBin::RenderLeafList mWaterLeaves;
        osgUtil::RenderBin::RenderLeafList mAboveWaterLeaves;
        osgUtil::RenderBin::RenderLeafList mStraddlingLeaves;
        osg::ref_ptr<osg::StateSet> mClipStateSet;
        bool mCameraUnderwater = false;
        const Water* mWater;
    };
}

#endif
