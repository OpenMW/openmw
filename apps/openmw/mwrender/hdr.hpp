#ifndef OPENMW_MWRENDER_HDR_H
#define OPENMW_MWRENDER_HDR_H

#include <array>

#include <osg/FrameBufferObject>
#include <osg/Texture2D>
#include <osg/Program>

namespace Shader
{
    class ShaderManager;
}

namespace MWRender
{
    class PingPongCanvas;

    class HDRDriver
    {

    public:

        HDRDriver() = default;

        HDRDriver(Shader::ShaderManager& shaderManager);

        void draw(const PingPongCanvas& canvas, osg::RenderInfo& renderInfo, osg::State& state, osg::GLExtensions* ext, size_t frameId);

        bool isEnabled() const { return mEnabled; }

        void enable() { mEnabled = true; }
        void disable() { mEnabled = false; }

        void dirty(int w, int h)
        {
            mWidth = w;
            mHeight = h;
            mCompiled = false;
        }

        osg::ref_ptr<osg::Texture2D> getLuminanceTexture(size_t frameId) const;

    private:

        void compile();

        struct HDRContainer
        {
            osg::ref_ptr<osg::FrameBufferObject> fullscreenFbo;
            osg::ref_ptr<osg::FrameBufferObject> mipmapFbo;
            osg::ref_ptr<osg::FrameBufferObject> finalFbo;
            osg::ref_ptr<osg::Texture2D> texture;
            osg::ref_ptr<osg::Texture2D> finalTexture;
            osg::ref_ptr<osg::StateSet> fullscreenStateset;
            osg::ref_ptr<osg::StateSet> mipmapStateset;
        };

        std::array<HDRContainer, 2> mBuffers;
        osg::ref_ptr<osg::Program> mLuminanceProgram;
        osg::ref_ptr<osg::Program> mProgram;

        bool mCompiled;
        bool mEnabled;

        int mWidth;
        int mHeight;
    };
}

#endif
