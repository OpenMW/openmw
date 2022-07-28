#include "postprocessor.hpp"

#include <algorithm>
#include <SDL_opengl_glext.h>

#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Texture2DMultisample>
#include <osg/Texture3D>
#include <osg/Texture2DArray>

#include <components/settings/settings.hpp>
#include <components/sceneutil/depth.hpp>
#include <components/sceneutil/color.hpp>
#include <components/sceneutil/nodecallback.hpp>
#include <components/sceneutil/util.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/shader/shadermanager.hpp>
#include <components/misc/stringops.hpp>
#include <components/vfs/manager.hpp>
#include <components/stereo/multiview.hpp>
#include <components/stereo/stereomanager.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwgui/postprocessorhud.hpp"

#include "transparentpass.hpp"
#include "pingpongcull.hpp"
#include "renderingmanager.hpp"
#include "vismask.hpp"
#include "sky.hpp"

namespace
{
    struct ResizedCallback : osg::GraphicsContext::ResizedCallback
    {
        ResizedCallback(MWRender::PostProcessor* postProcessor)
            : mPostProcessor(postProcessor)
        { }

        void resizedImplementation(osg::GraphicsContext* gc, int x, int y, int width, int height) override
        {
            gc->resizedImplementation(x, y, width, height);

            mPostProcessor->setRenderTargetSize(width, height);
            mPostProcessor->resize();
        }

        MWRender::PostProcessor* mPostProcessor;
    };

    class HUDCullCallback : public SceneUtil::NodeCallback<HUDCullCallback, osg::Camera*, osgUtil::CullVisitor*>
    {
    public:
        void operator()(osg::Camera* camera, osgUtil::CullVisitor* cv)
        {
            osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet;
            auto& sm = Stereo::Manager::instance();
            auto* fullViewport = camera->getViewport();
            if (sm.getEye(cv) == Stereo::Eye::Left)
                stateset->setAttributeAndModes(new osg::Viewport(0, 0, fullViewport->width() / 2, fullViewport->height()));
            if (sm.getEye(cv) == Stereo::Eye::Right)
                stateset->setAttributeAndModes(new osg::Viewport(fullViewport->width() / 2, 0, fullViewport->width() / 2, fullViewport->height()));

            cv->pushStateSet(stateset);
            traverse(camera, cv);
            cv->popStateSet();
        }
    };

    enum class Usage
    {
        RENDER_BUFFER,
        TEXTURE,
    };

    static osg::FrameBufferAttachment createFrameBufferAttachmentFromTemplate(Usage usage, int width, int height, osg::Texture* template_, int samples)
    {
        if (usage == Usage::RENDER_BUFFER && !Stereo::getMultiview())
        {
            osg::ref_ptr<osg::RenderBuffer> attachment = new osg::RenderBuffer(width, height, template_->getInternalFormat(), samples);
            return osg::FrameBufferAttachment(attachment);
        }

        auto texture = Stereo::createMultiviewCompatibleTexture(width, height, samples);
        texture->setSourceFormat(template_->getSourceFormat());
        texture->setSourceType(template_->getSourceType());
        texture->setInternalFormat(template_->getInternalFormat());
        texture->setFilter(osg::Texture2D::MIN_FILTER, template_->getFilter(osg::Texture2D::MIN_FILTER));
        texture->setFilter(osg::Texture2D::MAG_FILTER, template_->getFilter(osg::Texture2D::MAG_FILTER));
        texture->setWrap(osg::Texture::WRAP_S, template_->getWrap(osg::Texture2D::WRAP_S));
        texture->setWrap(osg::Texture::WRAP_T, template_->getWrap(osg::Texture2D::WRAP_T));

        return Stereo::createMultiviewCompatibleAttachment(texture);
    }
}

namespace MWRender
{
    PostProcessor::PostProcessor(RenderingManager& rendering, osgViewer::Viewer* viewer, osg::Group* rootNode, const VFS::Manager* vfs)
        : osg::Group()
        , mRootNode(rootNode)
        , mSamples(Settings::Manager::getInt("antialiasing", "Video"))
        , mDirty(false)
        , mDirtyFrameId(0)
        , mRendering(rendering)
        , mViewer(viewer)
        , mVFS(vfs)
        , mReload(false)
        , mEnabled(false)
        , mUsePostProcessing(false)
        , mSoftParticles(false)
        , mDisableDepthPasses(false)
        , mLastFrameNumber(0)
        , mLastSimulationTime(0.f)
        , mExteriorFlag(false)
        , mUnderwater(false)
        , mHDR(false)
        , mNormals(false)
        , mPrevNormals(false)
        , mNormalsSupported(false)
        , mPassLights(false)
        , mPrevPassLights(false)
        , mMainTemplate(new osg::Texture2D)
    {
        mSoftParticles = Settings::Manager::getBool("soft particles", "Shaders");
        mUsePostProcessing = Settings::Manager::getBool("enabled", "Post Processing");

        osg::GraphicsContext* gc = viewer->getCamera()->getGraphicsContext();
        osg::GLExtensions* ext = gc->getState()->get<osg::GLExtensions>();

        mWidth = gc->getTraits()->width;
        mHeight = gc->getTraits()->height;

        if (!ext->glDisablei && ext->glDisableIndexedEXT)
            ext->glDisablei = ext->glDisableIndexedEXT;

#ifdef ANDROID
        ext->glDisablei = nullptr;
#endif

        if (ext->glDisablei)
            mNormalsSupported = true;
        else
            Log(Debug::Error) << "'glDisablei' unsupported, pass normals will not be available to shaders.";

        if (mSoftParticles)
        {
            for (int i = 0; i < 2; ++i)
            {
                if (Stereo::getMultiview())
                    mTextures[i][Tex_OpaqueDepth] = new osg::Texture2DArray;
                else
                    mTextures[i][Tex_OpaqueDepth] = new osg::Texture2D;
            }
        }

        mGLSLVersion = ext->glslLanguageVersion * 100;
        mUBO = ext->isUniformBufferObjectSupported && mGLSLVersion >= 330;
        mStateUpdater = new fx::StateUpdater(mUBO);

        if (!Stereo::getStereo() && !SceneUtil::AutoDepth::isReversed() && !mSoftParticles && !mUsePostProcessing)
            return;

        enable(mUsePostProcessing);
    }

    PostProcessor::~PostProcessor()
    {
        if (auto* bin = osgUtil::RenderBin::getRenderBinPrototype("DepthSortedBin"))
            bin->setDrawCallback(nullptr);
    }

    void PostProcessor::resize()
    {
        mHUDCamera->resize(mWidth, mHeight);
        mViewer->getCamera()->resize(mWidth, mHeight);
        if (Stereo::getStereo())
            Stereo::Manager::instance().screenResolutionChanged();

        auto width = renderWidth();
        auto height = renderHeight();
        for (auto& technique : mTechniques)
        {
            for (auto& [name, rt] : technique->getRenderTargetsMap())
            {
                const auto [w, h] = rt.mSize.get(width, height);
                rt.mTarget->setTextureSize(w, h);
            }
        }

        size_t frameId = frame() % 2;

        createTexturesAndCamera(frameId);
        createObjectsForFrame(frameId);

        mRendering.updateProjectionMatrix();
        mRendering.setScreenRes(width, height);

        dirtyTechniques();

        mPingPongCanvas->dirty(frameId);

        mDirty = true;
        mDirtyFrameId = !frameId;

    }

    void PostProcessor::populateTechniqueFiles()
    {
        for (const auto& name : mVFS->getRecursiveDirectoryIterator(fx::Technique::sSubdir))
        {
            std::filesystem::path path = name;
            std::string fileExt = Misc::StringUtils::lowerCase(path.extension().string());
            if (!path.parent_path().has_parent_path() && fileExt == fx::Technique::sExt)
            {
                auto absolutePath = std::filesystem::path(mVFS->getAbsoluteFileName(name));
                mTechniqueFileMap[absolutePath.stem().string()] = absolutePath;
            }
        }
    }

    void PostProcessor::enable(bool usePostProcessing)
    {
        mReload = true;
        mEnabled = true;
        bool postPass = Settings::Manager::getBool("transparent postpass", "Post Processing");
        mUsePostProcessing = usePostProcessing;

        mDisableDepthPasses = !mSoftParticles && !postPass;

#ifdef ANDROID
        mDisableDepthPasses = true;
#endif

        if (!mDisableDepthPasses)
        {
            mTransparentDepthPostPass = new TransparentDepthBinCallback(mRendering.getResourceSystem()->getSceneManager()->getShaderManager(), postPass);
            osgUtil::RenderBin::getRenderBinPrototype("DepthSortedBin")->setDrawCallback(mTransparentDepthPostPass);
        }

        if (mUsePostProcessing && mTechniqueFileMap.empty())
        {
            populateTechniqueFiles();
        }

        mMainTemplate->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        mMainTemplate->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        mMainTemplate->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        mMainTemplate->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        mMainTemplate->setInternalFormat(GL_RGBA);
        mMainTemplate->setSourceType(GL_UNSIGNED_BYTE);
        mMainTemplate->setSourceFormat(GL_RGBA);

        createTexturesAndCamera(frame() % 2);

        removeChild(mHUDCamera);
        removeChild(mRootNode);

        addChild(mHUDCamera);
        addChild(mRootNode);

        mViewer->setSceneData(this);
        mViewer->getCamera()->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        mViewer->getCamera()->getGraphicsContext()->setResizedCallback(new ResizedCallback(this));
        mViewer->getCamera()->setUserData(this);

        setCullCallback(mStateUpdater);
        mHUDCamera->setCullCallback(new HUDCullCallback);
    }

    void PostProcessor::disable()
    {
        if (!mSoftParticles)
            osgUtil::RenderBin::getRenderBinPrototype("DepthSortedBin")->setDrawCallback(nullptr);

        if (!SceneUtil::AutoDepth::isReversed() && !mSoftParticles)
        {
            removeChild(mHUDCamera);
            setCullCallback(nullptr);

            mViewer->getCamera()->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER);
            mViewer->getCamera()->getGraphicsContext()->setResizedCallback(nullptr);
            mViewer->getCamera()->setUserData(nullptr);

            mEnabled = false;
        }

        mUsePostProcessing = false;
        mRendering.getSkyManager()->setSunglare(true);
    }

    void PostProcessor::traverse(osg::NodeVisitor& nv)
    {
        if (!mEnabled)
        {
            osg::Group::traverse(nv);
            return;
        }

        size_t frameId = nv.getTraversalNumber() % 2;

        if (nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
            cull(frameId, static_cast<osgUtil::CullVisitor*>(&nv));
        else if (nv.getVisitorType() == osg::NodeVisitor::UPDATE_VISITOR)
            update(frameId);

        osg::Group::traverse(nv);
    }

    void PostProcessor::cull(size_t frameId, osgUtil::CullVisitor* cv)
    {
        const auto& fbo = getFbo(FBO_Intercept, frameId);
        if (fbo)
        {
            osgUtil::RenderStage* rs = cv->getRenderStage();
            if (rs && rs->getMultisampleResolveFramebufferObject())
                rs->setMultisampleResolveFramebufferObject(fbo);
        }

        mPingPongCanvas->setPostProcessing(frameId, mUsePostProcessing);
        mPingPongCanvas->setNormalsTexture(frameId, mNormals ? getTexture(Tex_Normal, frameId) : nullptr);
        mPingPongCanvas->setMask(frameId, mUnderwater, mExteriorFlag);
        mPingPongCanvas->setHDR(frameId, getHDR());

        mPingPongCanvas->setSceneTexture(frameId, getTexture(Tex_Scene, frameId));
            if (mDisableDepthPasses)
                mPingPongCanvas->setDepthTexture(frameId, getTexture(Tex_Depth, frameId));
            else
                mPingPongCanvas->setDepthTexture(frameId, getTexture(Tex_OpaqueDepth, frameId));

            mPingPongCanvas->setLDRSceneTexture(frameId, getTexture(Tex_Scene_LDR, frameId));

        if (mTransparentDepthPostPass)
        {
            mTransparentDepthPostPass->mFbo[frameId] = mFbos[frameId][FBO_Primary];
            mTransparentDepthPostPass->mMsaaFbo[frameId] = mFbos[frameId][FBO_Multisample];
            mTransparentDepthPostPass->mOpaqueFbo[frameId] = mFbos[frameId][FBO_OpaqueDepth];
            mTransparentDepthPostPass->dirtyFrame(frameId);
        }

        size_t frame = cv->getTraversalNumber();

        mStateUpdater->setResolution(osg::Vec2f(cv->getViewport()->width(), cv->getViewport()->height()));

        // per-frame data
        if (frame != mLastFrameNumber)
        {
            mLastFrameNumber = frame;
            auto stamp = cv->getFrameStamp();

            mStateUpdater->setSimulationTime(static_cast<float>(stamp->getSimulationTime()));
            mStateUpdater->setDeltaSimulationTime(static_cast<float>(stamp->getSimulationTime() - mLastSimulationTime));
            mLastSimulationTime = stamp->getSimulationTime();

            for (const auto& dispatchNode : mPingPongCanvas->getCurrentFrameData(frame))
            {
                for (auto& uniform : dispatchNode.mHandle->getUniformMap())
                {
                    if (uniform->getType().has_value() && !uniform->mSamplerType)
                        if (auto* u = dispatchNode.mRootStateSet->getUniform(uniform->mName))
                            uniform->setUniform(u);
                }
            }
        }
    }

    void PostProcessor::updateLiveReload()
    {
        static const bool liveReload = Settings::Manager::getBool("live reload", "Post Processing");
        if (!liveReload)
            return;

        for (auto& technique : mTechniques)
        {
            if (technique->getStatus() == fx::Technique::Status::File_Not_exists)
                continue;

            const auto lastWriteTime = std::filesystem::last_write_time(mTechniqueFileMap[technique->getName()]);
            const bool isDirty = technique->setLastModificationTime(lastWriteTime);

            if (!isDirty)
                continue;

            if (technique->compile())
                Log(Debug::Info) << "Reloaded technique : " << mTechniqueFileMap[technique->getName()].string();

            mReload = technique->isValid();
        }
    }

    void PostProcessor::reloadIfRequired()
    {
        if (!mReload)
            return;

        mReload = false;

        if (!mTechniques.empty())
            reloadMainPass(*mTechniques[0]);

        reloadTechniques();

        if (!mUsePostProcessing)
            resize();
    }

    void PostProcessor::update(size_t frameId)
    {
        while (!mQueuedTemplates.empty())
        {
            mTemplates.push_back(std::move(mQueuedTemplates.back()));

            mQueuedTemplates.pop_back();
        }

        updateLiveReload();

        reloadIfRequired();

        if (mDirty && mDirtyFrameId == frameId)
        {
            createTexturesAndCamera(frameId);
            createObjectsForFrame(frameId);
            mDirty = false;

            mPingPongCanvas->setCurrentFrameData(frameId, fx::DispatchArray(mTemplateData));
        }

        if ((mNormalsSupported && mNormals != mPrevNormals) || (mPassLights != mPrevPassLights))
        {
            mPrevNormals = mNormals;
            mPrevPassLights = mPassLights;

            mViewer->stopThreading();

            auto& shaderManager = MWBase::Environment::get().getResourceSystem()->getSceneManager()->getShaderManager();
            auto defines = shaderManager.getGlobalDefines();
            defines["disableNormals"] = mNormals ? "0" : "1";
            shaderManager.setGlobalDefines(defines);

            mRendering.getLightRoot()->setCollectPPLights(mPassLights);
            mStateUpdater->bindPointLights(mPassLights ? mRendering.getLightRoot()->getPPLightsBuffer() : nullptr);
            mStateUpdater->reset();

            mViewer->startThreading();

            createTexturesAndCamera(frameId);
            createObjectsForFrame(frameId);

            mDirty = true;
            mDirtyFrameId = !frameId;
        }
    }

    void PostProcessor::createObjectsForFrame(size_t frameId)
    {
        auto& fbos = mFbos[frameId];
        auto& textures = mTextures[frameId];
        auto width = renderWidth();
        auto height = renderHeight();

        for (auto& tex : textures)
        {
            if (!tex)
                continue;

            Stereo::setMultiviewCompatibleTextureSize(tex, width, height);
            tex->dirtyTextureObject();
        }

        fbos[FBO_Primary] = new osg::FrameBufferObject;
        fbos[FBO_Primary]->setAttachment(osg::Camera::COLOR_BUFFER0, Stereo::createMultiviewCompatibleAttachment(textures[Tex_Scene]));
        if (mNormals && mNormalsSupported)
            fbos[FBO_Primary]->setAttachment(osg::Camera::COLOR_BUFFER1, Stereo::createMultiviewCompatibleAttachment(textures[Tex_Normal]));
        fbos[FBO_Primary]->setAttachment(osg::Camera::PACKED_DEPTH_STENCIL_BUFFER, Stereo::createMultiviewCompatibleAttachment(textures[Tex_Depth]));

        fbos[FBO_FirstPerson] = new osg::FrameBufferObject;

        auto fpDepthRb = createFrameBufferAttachmentFromTemplate(Usage::RENDER_BUFFER, width, height, textures[Tex_Depth], mSamples);
        fbos[FBO_FirstPerson]->setAttachment(osg::FrameBufferObject::BufferComponent::PACKED_DEPTH_STENCIL_BUFFER, osg::FrameBufferAttachment(fpDepthRb));

        // When MSAA is enabled we must first render to a render buffer, then
        // blit the result to the FBO which is either passed to the main frame
        // buffer for display or used as the entry point for a post process chain.
        if (mSamples > 1)
        {
            fbos[FBO_Multisample] = new osg::FrameBufferObject;
            auto colorRB = createFrameBufferAttachmentFromTemplate(Usage::RENDER_BUFFER, width, height, textures[Tex_Scene], mSamples);
            if (mNormals && mNormalsSupported)
            {
                auto normalRB = createFrameBufferAttachmentFromTemplate(Usage::RENDER_BUFFER, width, height, textures[Tex_Normal], mSamples);
                fbos[FBO_Multisample]->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER1, normalRB);
                fbos[FBO_FirstPerson]->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER1, normalRB);
            }
            auto depthRB = createFrameBufferAttachmentFromTemplate(Usage::RENDER_BUFFER, width, height, textures[Tex_Depth], mSamples);
            fbos[FBO_Multisample]->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0, colorRB);
            fbos[FBO_Multisample]->setAttachment(osg::FrameBufferObject::BufferComponent::PACKED_DEPTH_STENCIL_BUFFER, depthRB);
            fbos[FBO_FirstPerson]->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0, colorRB);

            fbos[FBO_Intercept] = new osg::FrameBufferObject;
            fbos[FBO_Intercept]->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0, Stereo::createMultiviewCompatibleAttachment(textures[Tex_Scene]));
            fbos[FBO_Intercept]->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER1, Stereo::createMultiviewCompatibleAttachment(textures[Tex_Normal]));
        }
        else
        {
            fbos[FBO_FirstPerson]->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0, Stereo::createMultiviewCompatibleAttachment(textures[Tex_Scene]));
            if (mNormals && mNormalsSupported)
                fbos[FBO_FirstPerson]->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER1, Stereo::createMultiviewCompatibleAttachment(textures[Tex_Normal]));
        }

        if (textures[Tex_OpaqueDepth])
        {
            fbos[FBO_OpaqueDepth] = new osg::FrameBufferObject;
            fbos[FBO_OpaqueDepth]->setAttachment(osg::FrameBufferObject::BufferComponent::PACKED_DEPTH_STENCIL_BUFFER, Stereo::createMultiviewCompatibleAttachment(textures[Tex_OpaqueDepth]));
        }

#ifdef __APPLE__
        if (textures[Tex_OpaqueDepth])
            fbos[FBO_OpaqueDepth]->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER, osg::FrameBufferAttachment(new osg::RenderBuffer(textures[Tex_OpaqueDepth]->getTextureWidth(), textures[Tex_OpaqueDepth]->getTextureHeight(), textures[Tex_Scene]->getInternalFormat())));
#endif
    }

    void PostProcessor::dirtyTechniques()
    {
        if (!isEnabled())
            return;

        size_t frameId = frame() % 2;

        mDirty = true;
        mDirtyFrameId = !frameId;

        mTemplateData = {};

        bool sunglare = true;
        mHDR = false;
        mNormals = false;
        mPassLights = false;

        for (const auto& technique : mTechniques)
        {
            if (!technique->isValid())
                continue;

            if (technique->getGLSLVersion() > mGLSLVersion)
            {
                Log(Debug::Warning) << "Technique " << technique->getName() << " requires GLSL version " << technique->getGLSLVersion() << " which is unsupported by your hardware.";
                continue;
            }

            fx::DispatchNode node;

            node.mFlags = technique->getFlags();

            if (technique->getHDR())
                mHDR = true;

            if (technique->getNormals())
                mNormals = true;

            if (technique->getLights())
                mPassLights = true;

            if (node.mFlags & fx::Technique::Flag_Disable_SunGlare)
                sunglare = false;

            // required default samplers available to every shader pass
            node.mRootStateSet->addUniform(new osg::Uniform("omw_SamplerLastShader", Unit_LastShader));
            node.mRootStateSet->addUniform(new osg::Uniform("omw_SamplerLastPass", Unit_LastPass));
            node.mRootStateSet->addUniform(new osg::Uniform("omw_SamplerDepth", Unit_Depth));

            if (mNormals)
                node.mRootStateSet->addUniform(new osg::Uniform("omw_SamplerNormals", Unit_Normals));

            if (technique->getHDR())
                node.mRootStateSet->addUniform(new osg::Uniform("omw_EyeAdaptation", Unit_EyeAdaptation));

            int texUnit = Unit_NextFree;

            // user-defined samplers
            for (const osg::Texture* texture : technique->getTextures())
            {
                if (const auto* tex1D = dynamic_cast<const osg::Texture1D*>(texture))
                    node.mRootStateSet->setTextureAttribute(texUnit, new osg::Texture1D(*tex1D));
                else if (const auto* tex2D = dynamic_cast<const osg::Texture2D*>(texture))
                    node.mRootStateSet->setTextureAttribute(texUnit, new osg::Texture2D(*tex2D));
                else if (const auto* tex3D = dynamic_cast<const osg::Texture3D*>(texture))
                    node.mRootStateSet->setTextureAttribute(texUnit, new osg::Texture3D(*tex3D));

                node.mRootStateSet->addUniform(new osg::Uniform(texture->getName().c_str(), texUnit++));
            }

            // user-defined uniforms
            for (auto& uniform : technique->getUniformMap())
            {
                if (uniform->mSamplerType) continue;

                if (auto type = uniform->getType())
                    uniform->setUniform(node.mRootStateSet->getOrCreateUniform(uniform->mName.c_str(), *type, uniform->getNumElements()));
            }

            std::unordered_map<osg::Texture2D*, osg::Texture2D*> renderTargetCache;

            for (const auto& pass : technique->getPasses())
            {
                int subTexUnit = texUnit;
                fx::DispatchNode::SubPass subPass;

                pass->prepareStateSet(subPass.mStateSet, technique->getName());

                node.mHandle = technique;

                if (!pass->getTarget().empty())
                {
                    const auto& rt = technique->getRenderTargetsMap()[pass->getTarget()];

                    const auto [w, h] = rt.mSize.get(renderWidth(), renderHeight());

                    subPass.mRenderTexture = new osg::Texture2D(*rt.mTarget);
                    renderTargetCache[rt.mTarget] = subPass.mRenderTexture;
                    subPass.mRenderTexture->setTextureSize(w, h);
                    subPass.mRenderTexture->setName(std::string(pass->getTarget()));

                    if (rt.mMipMap)
                        subPass.mRenderTexture->setNumMipmapLevels(osg::Image::computeNumberOfMipmapLevels(w, h));

                    subPass.mRenderTarget = new osg::FrameBufferObject;
                    subPass.mRenderTarget->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0, osg::FrameBufferAttachment(subPass.mRenderTexture));
                    subPass.mStateSet->setAttributeAndModes(new osg::Viewport(0, 0, w, h));
                }

                for (const auto& whitelist : pass->getRenderTargets())
                {
                    auto it = technique->getRenderTargetsMap().find(whitelist);
                    if (it != technique->getRenderTargetsMap().end() && renderTargetCache[it->second.mTarget])
                    {
                        subPass.mStateSet->setTextureAttribute(subTexUnit, renderTargetCache[it->second.mTarget]);
                        subPass.mStateSet->addUniform(new osg::Uniform(std::string(it->first).c_str(), subTexUnit++));
                    }
                }

                node.mPasses.emplace_back(std::move(subPass));
            }

            node.compile();

            mTemplateData.emplace_back(std::move(node));
        }

        mPingPongCanvas->setCurrentFrameData(frameId, fx::DispatchArray(mTemplateData));

        if (auto hud = MWBase::Environment::get().getWindowManager()->getPostProcessorHud())
            hud->updateTechniques();

        mRendering.getSkyManager()->setSunglare(sunglare);
    }

    PostProcessor::Status PostProcessor::enableTechnique(std::shared_ptr<fx::Technique> technique, std::optional<int> location)
    {
        if (!isEnabled())
        {
            Log(Debug::Warning) << "PostProcessing disabled, cannot load technique '" << technique->getName() << "'";
            return Status_Error;
        }

        if (!technique || technique->getLocked() || (location.has_value() && location.value() <= 0))
            return Status_Error;

        disableTechnique(technique, false);

        int pos = std::min<int>(location.value_or(mTechniques.size()), mTechniques.size());

        mTechniques.insert(mTechniques.begin() + pos, technique);
        dirtyTechniques();

        return Status_Toggled;
    }

    PostProcessor::Status PostProcessor::disableTechnique(std::shared_ptr<fx::Technique> technique, bool dirty)
    {
        if (technique->getLocked())
            return Status_Error;

        auto it = std::find(mTechniques.begin(), mTechniques.end(), technique);
        if (it == std::end(mTechniques))
            return Status_Unchanged;

        mTechniques.erase(it);
        if (dirty)
            dirtyTechniques();

        return Status_Toggled;
    }

    bool PostProcessor::isTechniqueEnabled(const std::shared_ptr<fx::Technique>& technique) const
    {
        if (auto it = std::find(mTechniques.begin(), mTechniques.end(), technique); it == mTechniques.end())
            return false;

        return technique->isValid();
    }

    void PostProcessor::createTexturesAndCamera(size_t frameId)
    {
        auto& textures = mTextures[frameId];

        auto width = renderWidth();
        auto height = renderHeight();

        for (auto& texture : textures)
        {
            if (!texture)
            {
                if (Stereo::getMultiview())
                    texture = new osg::Texture2DArray;
                else
                    texture = new osg::Texture2D;
            }
            Stereo::setMultiviewCompatibleTextureSize(texture, width, height);
            texture->setSourceFormat(GL_RGBA);
            texture->setSourceType(GL_UNSIGNED_BYTE);
            texture->setInternalFormat(GL_RGBA);
            texture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture::LINEAR);
            texture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture::LINEAR);
            texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            texture->setResizeNonPowerOfTwoHint(false);
        }

        textures[Tex_Normal]->setSourceFormat(GL_RGB);
        textures[Tex_Normal]->setInternalFormat(GL_RGB);

        if (mMainTemplate)
        {
            textures[Tex_Scene]->setSourceFormat(mMainTemplate->getSourceFormat());
            textures[Tex_Scene]->setSourceType(mMainTemplate->getSourceType());
            textures[Tex_Scene]->setInternalFormat(mMainTemplate->getInternalFormat());
            textures[Tex_Scene]->setFilter(osg::Texture2D::MIN_FILTER, mMainTemplate->getFilter(osg::Texture2D::MIN_FILTER));
            textures[Tex_Scene]->setFilter(osg::Texture2D::MAG_FILTER, mMainTemplate->getFilter(osg::Texture2D::MAG_FILTER));
            textures[Tex_Scene]->setWrap(osg::Texture::WRAP_S, mMainTemplate->getWrap(osg::Texture2D::WRAP_S));
            textures[Tex_Scene]->setWrap(osg::Texture::WRAP_T, mMainTemplate->getWrap(osg::Texture2D::WRAP_T));
        }

        auto setupDepth = [] (osg::Texture* tex) {
            tex->setSourceFormat(GL_DEPTH_STENCIL_EXT);
            tex->setSourceType(SceneUtil::AutoDepth::depthSourceType());
            tex->setInternalFormat(SceneUtil::AutoDepth::depthInternalFormat());
        };

        setupDepth(textures[Tex_Depth]);

        if (mDisableDepthPasses)
        {
            textures[Tex_OpaqueDepth] = nullptr;
        }
        else
        {
            setupDepth(textures[Tex_OpaqueDepth]);
            textures[Tex_OpaqueDepth]->setName("opaqueTexMap");
        }

        if (mHUDCamera)
            return;

        mHUDCamera = new osg::Camera;
        mHUDCamera->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
        mHUDCamera->setRenderOrder(osg::Camera::POST_RENDER);
        mHUDCamera->setClearColor(osg::Vec4(0.45, 0.45, 0.14, 1.0));
        mHUDCamera->setClearMask(0);
        mHUDCamera->setProjectionMatrix(osg::Matrix::ortho2D(0, 1, 0, 1));
        mHUDCamera->setAllowEventFocus(false);
        mHUDCamera->setViewport(0, 0, mWidth, mHeight);

        mViewer->getCamera()->removeCullCallback(mPingPongCull);
        mPingPongCull = new PingPongCull(this);
        mViewer->getCamera()->addCullCallback(mPingPongCull);

        mPingPongCanvas = new PingPongCanvas(mRendering.getResourceSystem()->getSceneManager()->getShaderManager());
        mHUDCamera->addChild(mPingPongCanvas);
        mHUDCamera->setNodeMask(Mask_RenderToTexture);

        mHUDCamera->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        mHUDCamera->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    }

    std::shared_ptr<fx::Technique> PostProcessor::loadTechnique(const std::string& name, bool loadNextFrame)
    {
        if (!isEnabled())
        {
            Log(Debug::Warning) << "PostProcessing disabled, cannot load technique '" << name << "'";
            return nullptr;
        }

        for (const auto& technique : mTemplates)
            if (Misc::StringUtils::ciEqual(technique->getName(), name))
                return technique;

        for (const auto& technique : mQueuedTemplates)
            if (Misc::StringUtils::ciEqual(technique->getName(), name))
                return technique;

        auto technique = std::make_shared<fx::Technique>(*mVFS, *mRendering.getResourceSystem()->getImageManager(), name, renderWidth(), renderHeight(), mUBO, mNormalsSupported);

        technique->compile();

        if (technique->getStatus() != fx::Technique::Status::File_Not_exists)
            technique->setLastModificationTime(std::filesystem::last_write_time(mTechniqueFileMap[technique->getName()]));

        if (loadNextFrame)
        {
            mQueuedTemplates.push_back(technique);
            return technique;
        }

        reloadMainPass(*technique);

        mTemplates.push_back(std::move(technique));

        return mTemplates.back();
    }

    void PostProcessor::reloadTechniques()
    {
        if (!isEnabled())
            return;

        mTechniques.clear();

        std::vector<std::string> techniqueStrings = Settings::Manager::getStringArray("chain", "Post Processing");

        const std::string mainIdentifier = "main";

        auto main = loadTechnique(mainIdentifier);

        if (main)
            main->setLocked(true);

        mTechniques.push_back(std::move(main));

        for (auto& techniqueName : techniqueStrings)
        {
            if (techniqueName.empty() || Misc::StringUtils::ciEqual(techniqueName, mainIdentifier))
                continue;

            mTechniques.push_back(loadTechnique(techniqueName));
        }

        dirtyTechniques();
    }

    void PostProcessor::reloadMainPass(fx::Technique& technique)
    {
        if (!technique.getMainTemplate())
            return;

        mMainTemplate = technique.getMainTemplate();

        resize();
    }

    void PostProcessor::toggleMode()
    {
        for (auto& technique : mTemplates)
            technique->compile();

        dirtyTechniques();
    }

    void PostProcessor::disableDynamicShaders()
    {
        for (auto& technique : mTechniques)
            if (technique->getDynamic())
                disableTechnique(technique);
    }

    int PostProcessor::renderWidth() const
    {
        if (Stereo::getStereo())
            return Stereo::Manager::instance().eyeResolution().x();
        return mWidth;
    }

    int PostProcessor::renderHeight() const
    {
        if (Stereo::getStereo())
            return Stereo::Manager::instance().eyeResolution().y();
        return mHeight;
    }
}

