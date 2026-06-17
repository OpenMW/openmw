#ifndef OPENMW_MWRENDER_PINGPONGCANVAS_H
#define OPENMW_MWRENDER_PINGPONGCANVAS_H

#include <array>
#include <optional>

#include <osg/FrameBufferObject>
#include <osg/Geometry>
#include <osg/Texture2D>

#include <components/fx/technique.hpp>

#include "luminancecalculator.hpp"

namespace Shader
{
    class ShaderManager;
}

namespace MWRender
{
    class PingPongCanvas : public osg::Geometry
    {
    public:
        PingPongCanvas(
            Shader::ShaderManager& shaderManager, const std::shared_ptr<LuminanceCalculator>& luminanceCalculator);

        void drawGeometry(osg::RenderInfo& renderInfo) const;

        void drawImplementation(osg::RenderInfo& renderInfo) const override;

        void dirty() { mDirty = true; }

        void setDirtyAttachments(const std::vector<Fx::Types::RenderTarget>& attachments)
        {
            mDirtyAttachments = attachments;
        }

        const Fx::DispatchArray& getPasses() { return mPasses; }

        void setPasses(Fx::DispatchArray&& passes);

        void setMask(bool underwater, bool exterior);

        void setTextureScene(osg::ref_ptr<osg::Texture> tex) { mTextureScene = tex; }

        void setTextureDepth(osg::ref_ptr<osg::Texture> tex) { mTextureDepth = tex; }

        void setTextureNormals(osg::ref_ptr<osg::Texture> tex) { mTextureNormals = tex; }

        void setTextureDistortion(osg::ref_ptr<osg::Texture> tex) { mTextureDistortion = tex; }

        void setCalculateAvgLum(bool enabled) { mAvgLum = enabled; }

        void setPostProcessing(bool enabled) { mPostprocessing = enabled; }

        const osg::ref_ptr<osg::Texture>& getSceneTexture(size_t frameId) const { return mTextureScene; }

    private:
        bool mAvgLum = false;
        bool mPostprocessing = false;

        Fx::DispatchArray mPasses;
        Fx::FlagsType mMask = 0;

        osg::ref_ptr<osg::Program> mFallbackProgram;
        osg::ref_ptr<osg::Program> mMultiviewResolveProgram;
        osg::ref_ptr<osg::StateSet> mFallbackStateSet;
        osg::ref_ptr<osg::StateSet> mMultiviewResolveStateSet;

        osg::ref_ptr<osg::Texture> mTextureScene;
        osg::ref_ptr<osg::Texture> mTextureDepth;
        osg::ref_ptr<osg::Texture> mTextureNormals;
        osg::ref_ptr<osg::Texture> mTextureDistortion;

        mutable bool mDirty = false;
        mutable std::vector<Fx::Types::RenderTarget> mDirtyAttachments;
        mutable osg::ref_ptr<osg::Viewport> mRenderViewport;
        mutable osg::ref_ptr<osg::FrameBufferObject> mMultiviewResolveFramebuffer;
        mutable osg::ref_ptr<osg::FrameBufferObject> mDestinationFBO;
        mutable std::array<osg::ref_ptr<osg::FrameBufferObject>, 3> mFbos;
        mutable std::shared_ptr<LuminanceCalculator> mLuminanceCalculator;
    };
}

#endif
