#include "luminancecalculator.hpp"

#include <components/settings/settings.hpp>
#include <components/shader/shadermanager.hpp>

#include "pingpongcanvas.hpp"

namespace MWRender
{
    LuminanceCalculator::LuminanceCalculator(Shader::ShaderManager& shaderManager)
    {
        const float hdrExposureTime = std::max(Settings::Manager::getFloat("auto exposure speed", "Post Processing"), 0.0001f);

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
        auto luminanceFragment = shaderManager.getShader("hdr_luminance_fragment.glsl", defines, osg::Shader::FRAGMENT);
        auto resolveFragment = shaderManager.getShader("hdr_resolve_fragment.glsl", defines, osg::Shader::FRAGMENT);

        mResolveProgram = shaderManager.getProgram(vertex, resolveFragment);
        mLuminanceProgram = shaderManager.getProgram(vertex, luminanceFragment);
    }

    void LuminanceCalculator::compile()
    {
        int mipmapLevels = osg::Image::computeNumberOfMipmapLevels(mWidth, mHeight);

        for (auto& buffer : mBuffers)
        {
            buffer.mipmappedSceneLuminanceTex = new osg::Texture2D;
            buffer.mipmappedSceneLuminanceTex->setInternalFormat(GL_R16F);
            buffer.mipmappedSceneLuminanceTex->setSourceFormat(GL_RED);
            buffer.mipmappedSceneLuminanceTex->setSourceType(GL_FLOAT);
            buffer.mipmappedSceneLuminanceTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            buffer.mipmappedSceneLuminanceTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            buffer.mipmappedSceneLuminanceTex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR_MIPMAP_NEAREST);
            buffer.mipmappedSceneLuminanceTex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
            buffer.mipmappedSceneLuminanceTex->setTextureSize(mWidth, mHeight);
            buffer.mipmappedSceneLuminanceTex->setNumMipmapLevels(mipmapLevels);

            buffer.luminanceTex = new osg::Texture2D;
            buffer.luminanceTex->setInternalFormat(GL_R16F);
            buffer.luminanceTex->setSourceFormat(GL_RED);
            buffer.luminanceTex->setSourceType(GL_FLOAT);
            buffer.luminanceTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            buffer.luminanceTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            buffer.luminanceTex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::NEAREST);
            buffer.luminanceTex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::NEAREST);
            buffer.luminanceTex->setTextureSize(1, 1);

            buffer.luminanceProxyTex = new osg::Texture2D(*buffer.luminanceTex);
            buffer.luminanceProxyTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            buffer.luminanceProxyTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

            buffer.resolveFbo = new osg::FrameBufferObject;
            buffer.resolveFbo->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0, osg::FrameBufferAttachment(buffer.luminanceTex));

            buffer.luminanceProxyFbo = new osg::FrameBufferObject;
            buffer.luminanceProxyFbo->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0, osg::FrameBufferAttachment(buffer.luminanceProxyTex));

            buffer.resolveSceneLumFbo = new osg::FrameBufferObject;
            buffer.resolveSceneLumFbo->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0, osg::FrameBufferAttachment(buffer.mipmappedSceneLuminanceTex, mipmapLevels - 1));

            buffer.sceneLumFbo = new osg::FrameBufferObject;
            buffer.sceneLumFbo->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0, osg::FrameBufferAttachment(buffer.mipmappedSceneLuminanceTex));

            buffer.sceneLumSS = new osg::StateSet;
            buffer.sceneLumSS->setAttributeAndModes(mLuminanceProgram);
            buffer.sceneLumSS->addUniform(new osg::Uniform("sceneTex", 0));

            buffer.resolveSS = new osg::StateSet;
            buffer.resolveSS->setAttributeAndModes(mResolveProgram);
            buffer.resolveSS->setTextureAttributeAndModes(0, buffer.luminanceProxyTex);
            buffer.resolveSS->addUniform(new osg::Uniform("luminanceSceneTex", 0));
            buffer.resolveSS->addUniform(new osg::Uniform("prevLuminanceSceneTex", 1));
        }

        mBuffers[0].resolveSS->setTextureAttributeAndModes(1, mBuffers[1].luminanceTex);
        mBuffers[1].resolveSS->setTextureAttributeAndModes(1, mBuffers[0].luminanceTex);

        mCompiled = true;
    }

    void LuminanceCalculator::draw(const PingPongCanvas& canvas, osg::RenderInfo& renderInfo, osg::State& state, osg::GLExtensions* ext, size_t frameId)
    {
        if (!mEnabled)
            return;

        bool dirty = !mCompiled;

        if (dirty)
            compile();

        auto& buffer = mBuffers[frameId];
        buffer.sceneLumFbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
        buffer.sceneLumSS->setTextureAttributeAndModes(0, canvas.getSceneTexture(frameId));

        state.apply(buffer.sceneLumSS);
        canvas.drawGeometry(renderInfo);

        state.applyTextureAttribute(0, buffer.mipmappedSceneLuminanceTex);
        ext->glGenerateMipmap(GL_TEXTURE_2D);

        buffer.resolveSceneLumFbo->apply(state, osg::FrameBufferObject::READ_FRAMEBUFFER);
        buffer.luminanceProxyFbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
        ext->glBlitFramebuffer(0, 0, 1, 1, 0, 0, 1, 1, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        if (dirty) {
            // Use current frame data for previous frame to warm up calculations and prevent popin
            mBuffers[(frameId + 1) % 2].resolveFbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
            ext->glBlitFramebuffer(0, 0, 1, 1, 0, 0, 1, 1, GL_COLOR_BUFFER_BIT, GL_NEAREST);

            buffer.luminanceProxyFbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
        }

        buffer.resolveFbo->apply(state, osg::FrameBufferObject::DRAW_FRAMEBUFFER);
        state.apply(buffer.resolveSS);
        canvas.drawGeometry(renderInfo);

        ext->glBindFramebuffer(GL_FRAMEBUFFER_EXT, state.getGraphicsContext() ? state.getGraphicsContext()->getDefaultFboId() : 0);
    }

    osg::ref_ptr<osg::Texture2D> LuminanceCalculator::getLuminanceTexture(size_t frameId) const
    {
        return mBuffers[frameId].luminanceTex;
    }
}