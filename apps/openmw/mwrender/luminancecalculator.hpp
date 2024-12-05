#ifndef OPENMW_MWRENDER_LUMINANCECALCULATOR_H
#define OPENMW_MWRENDER_LUMINANCECALCULATOR_H

#include <array>

#include <osg/FrameBufferObject>
#include <osg/Program>
#include <osg/Texture2D>

namespace Shader
{
    class ShaderManager;
}

namespace MWRender
{
    class PingPongCanvas;

    class LuminanceCalculator
    {

    public:
        LuminanceCalculator() = default;

        LuminanceCalculator(Shader::ShaderManager& shaderManager);

        void draw(const PingPongCanvas& canvas, osg::RenderInfo& renderInfo, osg::State& state, osg::GLExtensions* ext,
            size_t frameId);

        bool isEnabled() const { return mEnabled; }

        void enable() { mEnabled = true; }
        void disable() { mEnabled = false; }

        void dirty(int w, int h);

        osg::ref_ptr<osg::Texture2D> getLuminanceTexture(size_t frameId) const;

    private:
        void compile();

        struct Container
        {
            osg::ref_ptr<osg::FrameBufferObject> sceneLumFbo;
            osg::ref_ptr<osg::FrameBufferObject> resolveSceneLumFbo;
            osg::ref_ptr<osg::FrameBufferObject> resolveFbo;
            osg::ref_ptr<osg::FrameBufferObject> luminanceProxyFbo;
            osg::ref_ptr<osg::Texture2D> mipmappedSceneLuminanceTex;
            osg::ref_ptr<osg::Texture2D> luminanceTex;
            osg::ref_ptr<osg::Texture2D> luminanceProxyTex;
            osg::ref_ptr<osg::StateSet> sceneLumSS;
            osg::ref_ptr<osg::StateSet> resolveSS;
        };

        std::array<Container, 2> mBuffers;
        osg::ref_ptr<osg::Program> mLuminanceProgram;
        osg::ref_ptr<osg::Program> mResolveProgram;

        bool mCompiled = false;
        bool mEnabled = false;
        bool mIsBlank = true;

        int mWidth = 1;
        int mHeight = 1;
        osg::Vec2f mScale = osg::Vec2f(1, 1);
    };
}

#endif
