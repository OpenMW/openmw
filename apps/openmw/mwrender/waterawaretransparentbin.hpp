#ifndef OPENMW_MWRENDER_WATERAWARETRANSPARENTBIN_H
#define OPENMW_MWRENDER_WATERAWARETRANSPARENTBIN_H

#include <osgUtil/RenderBin>

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
        osg::ref_ptr<OpaqueColorBinCallback> mOpaqueColorResolve;
        osgUtil::RenderBin::RenderLeafList mUnderwaterLeaves;
        osgUtil::RenderBin::RenderLeafList mWaterLeaves;
        osgUtil::RenderBin::RenderLeafList mAboveWaterLeaves;
        bool mCameraUnderwater = false;
        const Water* mWater;
    };
}

#endif
