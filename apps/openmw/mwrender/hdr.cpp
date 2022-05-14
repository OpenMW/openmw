#include "hdr.hpp"

#include <components/settings/settings.hpp>
#include <components/shader/shadermanager.hpp>

#include "pingpongcanvas.hpp"

namespace MWRender
{
    HDRDriver::HDRDriver(Shader::ShaderManager& shaderManager)
        : mCompiled(false)
        , mEnabled(false)
        , mWidth(1)
        , mHeight(1)
    {
        const float hdrExposureTime = std::clamp(Settings::Manager::getFloat("hdr exposure time", "Post Processing"), 0.f, 1.f);

        constexpr float minLog = -9.0;
        constexpr float maxLog = 4.0;
        constexpr float logLumRange = (maxLog - minLog);
        constexpr float invLogLumRange = 1.0 / logLumRange;
        constexpr float epsilon = 0.004;

        Shader::ShaderManager::DefineMap defines = {
            {"minLog", std::to_string(minLog)},
            {"maxLog", std::to_string(maxLog)},
            {"logLumRange", std::to_string(logLumRange)},
            {"invLogLumRange", std::to_string(invLogLumRange)},
            {"hdrExposureTime", std::to_string(hdrExposureTime)},
            {"epsilon", std::to_string(epsilon)},
        };

        auto vertex = shaderManager.getShader("fullscreen_tri_vertex.glsl", {}, osg::Shader::VERTEX);
        auto hdrLuminance = shaderManager.getShader("hdr_luminance_fragment.glsl", defines, osg::Shader::FRAGMENT);
        auto hdr = shaderManager.getShader("hdr_fragment.glsl", defines, osg::Shader::FRAGMENT);

        mProgram = shaderManager.getProgram(vertex, hdr);
        mLuminanceProgram = shaderManager.getProgram(vertex, hdrLuminance);
    }

    void HDRDriver::compile()
    {
        int mipmapLevels = osg::Image::computeNumberOfMipmapLevels(mWidth, mHeight);

        for (auto& buffer : mBuffers)
        {
            buffer.texture = new osg::Texture2D;
            buffer.texture->setInternalFormat(GL_R16F);
            buffer.texture->setSourceFormat(GL_RED);
            buffer.texture->setSourceType(GL_FLOAT);
            buffer.texture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR_MIPMAP_NEAREST);
            buffer.texture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
            buffer.texture->setTextureSize(mWidth, mHeight);
            buffer.texture->setNumMipmapLevels(mipmapLevels);

            buffer.finalTexture = new osg::Texture2D;
            buffer.finalTexture->setInternalFormat(GL_R16F);
            buffer.finalTexture->setSourceFormat(GL_RED);
            buffer.finalTexture->setSourceType(GL_FLOAT);
            buffer.finalTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::NEAREST);
            buffer.finalTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::NEAREST);
            buffer.finalTexture->setTextureSize(1, 1);

            buffer.finalFbo = new osg::FrameBufferObject;
            buffer.finalFbo->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0, osg::FrameBufferAttachment(buffer.finalTexture));

            buffer.fullscreenFbo = new osg::FrameBufferObject;
            buffer.fullscreenFbo->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0, osg::FrameBufferAttachment(buffer.texture));

            buffer.mipmapFbo = new osg::FrameBufferObject;
            buffer.mipmapFbo->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0, osg::FrameBufferAttachment(buffer.texture, mipmapLevels - 1));

            buffer.fullscreenStateset = new osg::StateSet;
            buffer.fullscreenStateset->setAttributeAndModes(mLuminanceProgram);
            buffer.fullscreenStateset->addUniform(new osg::Uniform("sceneTex", 0));

            buffer.mipmapStateset = new osg::StateSet;
            buffer.mipmapStateset->setAttributeAndModes(mProgram);
            buffer.mipmapStateset->setTextureAttributeAndModes(0, buffer.texture);
            buffer.mipmapStateset->addUniform(new osg::Uniform("luminanceSceneTex", 0));
            buffer.mipmapStateset->addUniform(new osg::Uniform("prevLuminanceSceneTex", 1));
        }

        mBuffers[0].mipmapStateset->setTextureAttributeAndModes(1, mBuffers[1].finalTexture);
        mBuffers[1].mipmapStateset->setTextureAttributeAndModes(1, mBuffers[0].finalTexture);

        mCompiled = true;
    }

    void HDRDriver::draw(const PingPongCanvas& canvas, osg::RenderInfo& renderInfo, osg::State& state, osg::GLExtensions* ext, size_t frameId)
    {
        if (!mEnabled)
            return;

        if (!mCompiled)
            compile();

        auto& hdrBuffer = mBuffers[frameId];
        hdrBuffer.fullscreenFbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
        hdrBuffer.fullscreenStateset->setTextureAttributeAndModes(0, canvas.getSceneTexture(frameId));

        state.apply(hdrBuffer.fullscreenStateset);
        canvas.drawGeometry(renderInfo);

        state.applyTextureAttribute(0, hdrBuffer.texture);
        ext->glGenerateMipmap(GL_TEXTURE_2D);

        hdrBuffer.mipmapFbo->apply(state, osg::FrameBufferObject::READ_FRAMEBUFFER);
        hdrBuffer.finalFbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);

        ext->glBlitFramebuffer(0, 0, 1, 1, 0, 0, 1, 1, GL_COLOR_BUFFER_BIT, GL_LINEAR);

        state.apply(hdrBuffer.mipmapStateset);
        canvas.drawGeometry(renderInfo);

        ext->glBindFramebuffer(GL_FRAMEBUFFER_EXT, state.getGraphicsContext() ? state.getGraphicsContext()->getDefaultFboId() : 0);
    }

    osg::ref_ptr<osg::Texture2D> HDRDriver::getLuminanceTexture(size_t frameId) const
    {
        return mBuffers[frameId].finalTexture;
    }
}