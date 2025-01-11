#ifndef OPENMW_MWRENDER_TRANSPARENTPASS_H
#define OPENMW_MWRENDER_TRANSPARENTPASS_H

#include <array>
#include <memory>

#include <osg/FrameBufferObject>
#include <osg/StateSet>

#include <osgUtil/RenderBin>

#include <components/sceneutil/depth.hpp>

namespace Shader
{
    class ShaderManager;
}

namespace Stereo
{
    class MultiviewFramebufferResolve;
}

namespace MWRender
{
    class TransparentDepthBinCallback : public osgUtil::RenderBin::DrawCallback
    {
    public:
        TransparentDepthBinCallback(Shader::ShaderManager& shaderManager, bool postPass);

        void drawImplementation(
            osgUtil::RenderBin* bin, osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous) override;

        std::array<osg::ref_ptr<osg::FrameBufferObject>, 2> mFbo;
        std::array<osg::ref_ptr<osg::FrameBufferObject>, 2> mMsaaFbo;
        std::array<osg::ref_ptr<osg::FrameBufferObject>, 2> mOpaqueFbo;

        std::array<std::unique_ptr<Stereo::MultiviewFramebufferResolve>, 2> mMultiviewResolve;

        void setNormalsMode(int mode) { mNormalsMode = mode; }

        void setNormalsEnabled(bool enable) { mNormals = enable; };

        void setPostPass(bool enable);

    private:
        osg::ref_ptr<osg::StateSet> mStateSet;
        osg::ref_ptr<osg::Depth> mDepth;
        bool mPostPass;
        bool mNormals;
        int mNormalsMode;
    };

}

#endif
