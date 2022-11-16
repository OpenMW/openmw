#ifndef OPENMW_MWRENDER_TRANSPARENTPASS_H
#define OPENMW_MWRENDER_TRANSPARENTPASS_H

#include <array>
#include <memory>

#include <osg/StateSet>
#include <osg/ref_ptr>

#include <osgUtil/RenderBin>

#include <components/stereo/multiview.hpp>

namespace Shader
{
    class ShaderManager;
}

namespace osg
{
    class FrameBufferObject;
    class RenderInfo;
}

namespace osgUtil
{
    class RenderLeaf;
}

namespace MWRender
{
    class TransparentDepthBinCallback : public osgUtil::RenderBin::DrawCallback
    {
    public:
        TransparentDepthBinCallback(Shader::ShaderManager& shaderManager, bool postPass);

        void drawImplementation(
            osgUtil::RenderBin* bin, osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous) override;
        void dirtyFrame(int frameId);

        std::array<osg::ref_ptr<osg::FrameBufferObject>, 2> mFbo;
        std::array<osg::ref_ptr<osg::FrameBufferObject>, 2> mMsaaFbo;
        std::array<osg::ref_ptr<osg::FrameBufferObject>, 2> mOpaqueFbo;

        std::array<std::unique_ptr<Stereo::MultiviewFramebufferResolve>, 2> mMultiviewResolve;

    private:
        osg::ref_ptr<osg::StateSet> mStateSet;
        bool mPostPass;
    };

}

#endif
