#ifndef OPENMW_MWRENDER_OPAQUEBLIT_H
#define OPENMW_MWRENDER_OPAQUEBLIT_H

#include <array>
#include <memory>

#include <osg/FrameBufferObject>
#include <osgUtil/RenderBin>

namespace Stereo
{
    class MultiviewFramebufferResolve;
}

namespace MWRender
{
    class OpaqueColorBinCallback : public osgUtil::RenderBin::DrawCallback
    {
    public:
        void drawImplementation(osgUtil::RenderBin*, osg::RenderInfo&, osgUtil::RenderLeaf*&) override;
        void resolve(osgUtil::RenderBin*, osg::RenderInfo&);

        osg::ref_ptr<osg::FrameBufferObject> mFbo[2];
        osg::ref_ptr<osg::FrameBufferObject> mMsaaFbo[2];
        osg::ref_ptr<osg::FrameBufferObject> mOpaqueFbo[2];
        std::array<std::unique_ptr<Stereo::MultiviewFramebufferResolve>, 2> mMultiviewResolve;
    };
}

#endif
