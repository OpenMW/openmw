#include "luminancecalculator.hpp"

#include <components/misc/mathutil.hpp>
#include <components/settings/values.hpp>
#include <components/shader/shadermanager.hpp>

#include "pingpongcanvas.hpp"

namespace MWRender
{
    LuminanceCalculator::LuminanceCalculator(Shader::ShaderManager& shaderManager)
    {
        Shader::ShaderManager::DefineMap defines = {
            { "hdrExposureTime", std::to_string(Settings::postProcessing().mAutoExposureSpeed) },
        };

        auto vertex = shaderManager.getShader("fullscreen_tri.vert", {});
        auto luminanceFragment = shaderManager.getShader("luminance/luminance.frag", defines);
        auto resolveFragment = shaderManager.getShader("luminance/resolve.frag", defines);

        mResolveProgram = shaderManager.getProgram(vertex, std::move(resolveFragment));
        mLuminanceProgram = shaderManager.getProgram(std::move(vertex), std::move(luminanceFragment));

        for (auto& buffer : mBuffers)
        {
            buffer.mipmappedSceneLuminanceTex = new osg::Texture2D;
            buffer.mipmappedSceneLuminanceTex->setInternalFormat(GL_R16F);
            buffer.mipmappedSceneLuminanceTex->setSourceFormat(GL_RED);
            buffer.mipmappedSceneLuminanceTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            buffer.mipmappedSceneLuminanceTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            buffer.mipmappedSceneLuminanceTex->setFilter(
                osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR_MIPMAP_NEAREST);
            buffer.mipmappedSceneLuminanceTex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
            buffer.mipmappedSceneLuminanceTex->setTextureSize(mWidth, mHeight);

            buffer.luminanceTex = new osg::Texture2D;
            buffer.luminanceTex->setInternalFormat(GL_R16F);
            buffer.luminanceTex->setSourceFormat(GL_RED);
            buffer.luminanceTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            buffer.luminanceTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            buffer.luminanceTex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::NEAREST);
            buffer.luminanceTex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::NEAREST);
            buffer.luminanceTex->setTextureSize(1, 1);

            buffer.luminanceProxyTex = new osg::Texture2D(*buffer.luminanceTex);
            buffer.luminanceProxyTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            buffer.luminanceProxyTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

            buffer.resolveFbo = new osg::FrameBufferObject;
            buffer.resolveFbo->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0,
                osg::FrameBufferAttachment(buffer.luminanceTex));

            buffer.luminanceProxyFbo = new osg::FrameBufferObject;
            buffer.luminanceProxyFbo->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0,
                osg::FrameBufferAttachment(buffer.luminanceProxyTex));

            buffer.sceneLumSS = new osg::StateSet;
            buffer.sceneLumSS->setAttributeAndModes(mLuminanceProgram);
            buffer.sceneLumSS->addUniform(new osg::Uniform("sceneTex", 0));
            buffer.sceneLumSS->addUniform(new osg::Uniform("scaling", mScale));

            buffer.resolveSS = new osg::StateSet;
            buffer.resolveSS->setAttributeAndModes(mResolveProgram);
            buffer.resolveSS->setTextureAttributeAndModes(0, buffer.luminanceProxyTex);
            buffer.resolveSS->addUniform(new osg::Uniform("luminanceSceneTex", 0));
            buffer.resolveSS->addUniform(new osg::Uniform("prevLuminanceSceneTex", 1));
        }

        mBuffers[0].resolveSS->setTextureAttributeAndModes(1, mBuffers[1].luminanceTex);
        mBuffers[1].resolveSS->setTextureAttributeAndModes(1, mBuffers[0].luminanceTex);
    }

    void LuminanceCalculator::compile()
    {
        int mipmapLevels = osg::Image::computeNumberOfMipmapLevels(mWidth, mHeight);

        for (auto& buffer : mBuffers)
        {
            buffer.mipmappedSceneLuminanceTex->setTextureSize(mWidth, mHeight);
            buffer.mipmappedSceneLuminanceTex->setNumMipmapLevels(mipmapLevels);
            buffer.mipmappedSceneLuminanceTex->dirtyTextureObject();

            buffer.resolveSceneLumFbo = new osg::FrameBufferObject;
            buffer.resolveSceneLumFbo->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0,
                osg::FrameBufferAttachment(buffer.mipmappedSceneLuminanceTex, mipmapLevels - 1));

            buffer.sceneLumFbo = new osg::FrameBufferObject;
            buffer.sceneLumFbo->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0,
                osg::FrameBufferAttachment(buffer.mipmappedSceneLuminanceTex));
        }

        mCompiled = true;
    }

    void LuminanceCalculator::draw(const PingPongCanvas& canvas, osg::RenderInfo& renderInfo, osg::State& state,
        osg::GLExtensions* ext, size_t frameId)
    {
        if (!mEnabled)
            return;

        bool dirty = !mCompiled;

        if (dirty)
            compile();

        auto& buffer = mBuffers[frameId];
        buffer.sceneLumFbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
        buffer.sceneLumSS->setTextureAttributeAndModes(0, canvas.getSceneTexture(frameId));
        buffer.sceneLumSS->getUniform("scaling")->set(mScale);

        state.apply(buffer.sceneLumSS);
        canvas.drawGeometry(renderInfo);

        state.applyTextureAttribute(0, buffer.mipmappedSceneLuminanceTex);
        ext->glGenerateMipmap(GL_TEXTURE_2D);

        buffer.resolveSceneLumFbo->apply(state, osg::FrameBufferObject::READ_FRAMEBUFFER);
        buffer.luminanceProxyFbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
        ext->glBlitFramebuffer(0, 0, 1, 1, 0, 0, 1, 1, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        if (mIsBlank)
        {
            // Use current frame data for previous frame to warm up calculations and prevent popin
            mBuffers[(frameId + 1) % 2].resolveFbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
            ext->glBlitFramebuffer(0, 0, 1, 1, 0, 0, 1, 1, GL_COLOR_BUFFER_BIT, GL_NEAREST);

            buffer.luminanceProxyFbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
            mIsBlank = false;
        }

        buffer.resolveFbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
        state.apply(buffer.resolveSS);
        canvas.drawGeometry(renderInfo);

        ext->glBindFramebuffer(
            GL_FRAMEBUFFER_EXT, state.getGraphicsContext() ? state.getGraphicsContext()->getDefaultFboId() : 0);
    }

    osg::ref_ptr<osg::Texture2D> LuminanceCalculator::getLuminanceTexture(size_t frameId) const
    {
        return mBuffers[frameId].luminanceTex;
    }

    void LuminanceCalculator::dirty(int w, int h)
    {
        constexpr int minSize = 64;

        mWidth = std::max(minSize, Misc::nextPowerOfTwo(w) / 2);
        mHeight = std::max(minSize, Misc::nextPowerOfTwo(h) / 2);

        mScale = osg::Vec2f(w / static_cast<float>(mWidth), h / static_cast<float>(mHeight));

        mCompiled = false;
    }
}
