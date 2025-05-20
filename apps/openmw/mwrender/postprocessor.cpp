#include "postprocessor.hpp"

#include <SDL_opengl_glext.h>
#include <algorithm>
#include <chrono>
#include <thread>

#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Texture2DArray>
#include <osg/Texture2DMultisample>
#include <osg/Texture3D>

#include <components/files/conversion.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/lower.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/color.hpp>
#include <components/sceneutil/depth.hpp>
#include <components/sceneutil/nodecallback.hpp>
#include <components/settings/values.hpp>
#include <components/shader/shadermanager.hpp>
#include <components/stereo/multiview.hpp>
#include <components/stereo/stereomanager.hpp>
#include <components/vfs/manager.hpp>
#include <components/vfs/recursivedirectoryiterator.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwgui/postprocessorhud.hpp"

#include "distortion.hpp"
#include "pingpongcull.hpp"
#include "renderbin.hpp"
#include "renderingmanager.hpp"
#include "sky.hpp"
#include "transparentpass.hpp"
#include "vismask.hpp"

namespace
{
    struct ResizedCallback : osg::GraphicsContext::ResizedCallback
    {
        ResizedCallback(MWRender::PostProcessor* postProcessor)
            : mPostProcessor(postProcessor)
        {
        }

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
                stateset->setAttributeAndModes(
                    new osg::Viewport(0, 0, fullViewport->width() / 2, fullViewport->height()));
            if (sm.getEye(cv) == Stereo::Eye::Right)
                stateset->setAttributeAndModes(
                    new osg::Viewport(fullViewport->width() / 2, 0, fullViewport->width() / 2, fullViewport->height()));

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

    static osg::FrameBufferAttachment createFrameBufferAttachmentFromTemplate(
        Usage usage, int width, int height, osg::Texture* template_, int samples)
    {
        if (usage == Usage::RENDER_BUFFER && !Stereo::getMultiview())
        {
            osg::ref_ptr<osg::RenderBuffer> attachment
                = new osg::RenderBuffer(width, height, template_->getInternalFormat(), samples);
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

    constexpr float DistortionRatio = 0.25;
}

namespace MWRender
{
    PostProcessor::PostProcessor(
        RenderingManager& rendering, osgViewer::Viewer* viewer, osg::Group* rootNode, const VFS::Manager* vfs)
        : osg::Group()
        , mRootNode(rootNode)
        , mHUDCamera(new osg::Camera)
        , mRendering(rendering)
        , mViewer(viewer)
        , mVFS(vfs)
        , mUsePostProcessing(Settings::postProcessing().mEnabled)
        , mSamples(Settings::video().mAntialiasing)
        , mPingPongCull(new PingPongCull(this))
        , mDistortionCallback(new DistortionCallback)
    {
        auto& shaderManager = mRendering.getResourceSystem()->getSceneManager()->getShaderManager();

        std::shared_ptr<LuminanceCalculator> luminanceCalculator = std::make_shared<LuminanceCalculator>(shaderManager);

        for (auto& canvas : mCanvases)
            canvas = new PingPongCanvas(shaderManager, luminanceCalculator);

        mHUDCamera->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
        mHUDCamera->setRenderOrder(osg::Camera::POST_RENDER);
        mHUDCamera->setClearColor(osg::Vec4(0.45, 0.45, 0.14, 1.0));
        mHUDCamera->setClearMask(0);
        mHUDCamera->setProjectionMatrix(osg::Matrix::ortho2D(0, 1, 0, 1));
        mHUDCamera->setAllowEventFocus(false);
        mHUDCamera->setViewport(0, 0, mWidth, mHeight);
        mHUDCamera->setNodeMask(Mask_RenderToTexture);
        mHUDCamera->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        mHUDCamera->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
        mHUDCamera->addChild(mCanvases[0]);
        mHUDCamera->addChild(mCanvases[1]);
        mHUDCamera->setCullCallback(new HUDCullCallback);
        mViewer->getCamera()->addCullCallback(mPingPongCull);

        // resolves the multisampled depth buffer and optionally draws an additional depth postpass
        mTransparentDepthPostPass
            = new TransparentDepthBinCallback(mRendering.getResourceSystem()->getSceneManager()->getShaderManager(),
                Settings::postProcessing().mTransparentPostpass);
        osgUtil::RenderBin::getRenderBinPrototype("DepthSortedBin")->setDrawCallback(mTransparentDepthPostPass);

        osg::ref_ptr<osgUtil::RenderBin> distortionRenderBin
            = new osgUtil::RenderBin(osgUtil::RenderBin::SORT_BACK_TO_FRONT);
        // This is silly to have to do, but if nothing is drawn then the drawcallback is never called and the distortion
        // texture will never be cleared
        osg::ref_ptr<osg::Node> dummyNodeToClear = new osg::Node;
        dummyNodeToClear->setCullingActive(false);
        dummyNodeToClear->getOrCreateStateSet()->setRenderBinDetails(RenderBin_Distortion, "Distortion");
        rootNode->addChild(dummyNodeToClear);
        distortionRenderBin->setDrawCallback(mDistortionCallback);
        distortionRenderBin->getStateSet()->setDefine("DISTORTION", "1", osg::StateAttribute::ON);

        // Give the renderbin access to the opaque depth sampler so it can write its occlusion
        // Distorted geometry is drawn with ALWAYS depth function and depths writes disbled.
        const int unitSoftEffect
            = shaderManager.reserveGlobalTextureUnits(Shader::ShaderManager::Slot::OpaqueDepthTexture);
        distortionRenderBin->getStateSet()->addUniform(new osg::Uniform("opaqueDepthTex", unitSoftEffect));

        osgUtil::RenderBin::addRenderBinPrototype("Distortion", distortionRenderBin);

        auto defines = shaderManager.getGlobalDefines();
        defines["distorionRTRatio"] = std::to_string(DistortionRatio);
        shaderManager.setGlobalDefines(defines);

        createObjectsForFrame(0);
        createObjectsForFrame(1);

        populateTechniqueFiles();

        auto distortion = loadTechnique("internal_distortion");
        distortion->setInternal(true);
        distortion->setLocked(true);
        mInternalTechniques.push_back(distortion);

        osg::GraphicsContext* gc = viewer->getCamera()->getGraphicsContext();
        osg::GLExtensions* ext = gc->getState()->get<osg::GLExtensions>();

        mWidth = gc->getTraits()->width;
        mHeight = gc->getTraits()->height;

        if (!ext->glDisablei && ext->glDisableIndexedEXT)
            ext->glDisablei = ext->glDisableIndexedEXT;

        if (ext->glDisablei)
            mNormalsSupported = true;
        else
            Log(Debug::Error) << "'glDisablei' unsupported, pass normals will not be available to shaders.";

        mGLSLVersion = ext->glslLanguageVersion * 100;
        mUBO = ext->isUniformBufferObjectSupported && mGLSLVersion >= 330;
        mStateUpdater = new fx::StateUpdater(mUBO);

        addChild(mHUDCamera);
        addChild(mRootNode);

        mViewer->setSceneData(this);
        mViewer->getCamera()->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        mViewer->getCamera()->getGraphicsContext()->setResizedCallback(new ResizedCallback(this));
        mViewer->getCamera()->setUserData(this);

        setCullCallback(mStateUpdater);

        if (mUsePostProcessing)
            enable();
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

        size_t frameId = frame() % 2;

        createObjectsForFrame(frameId);

        mRendering.updateProjectionMatrix();
        mRendering.setScreenRes(renderWidth(), renderHeight());

        dirtyTechniques(true);

        mDirty = true;
        mDirtyFrameId = !frameId;
    }

    void PostProcessor::populateTechniqueFiles()
    {
        for (const auto& name : mVFS->getRecursiveDirectoryIterator(fx::Technique::sSubdir))
        {
            std::filesystem::path path = Files::pathFromUnicodeString(name);
            std::string fileExt = Misc::StringUtils::lowerCase(Files::pathToUnicodeString(path.extension()));
            if (!path.parent_path().has_parent_path() && fileExt == fx::Technique::sExt)
            {
                const auto absolutePath = mVFS->getAbsoluteFileName(path);
                mTechniqueFileMap[Files::pathToUnicodeString(absolutePath.stem())] = absolutePath;
            }
        }
    }

    void PostProcessor::enable()
    {
        mReload = true;
        mUsePostProcessing = true;
    }

    void PostProcessor::disable()
    {
        mUsePostProcessing = false;
        mRendering.getSkyManager()->setSunglare(true);
    }

    void PostProcessor::traverse(osg::NodeVisitor& nv)
    {
        size_t frameId = nv.getTraversalNumber() % 2;

        if (nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
            cull(frameId, static_cast<osgUtil::CullVisitor*>(&nv));
        else if (nv.getVisitorType() == osg::NodeVisitor::UPDATE_VISITOR)
            update(frameId);

        osg::Group::traverse(nv);
    }

    void PostProcessor::cull(size_t frameId, osgUtil::CullVisitor* cv)
    {
        if (const auto& fbo = getFbo(FBO_Intercept, frameId))
        {
            osgUtil::RenderStage* rs = cv->getRenderStage();
            if (rs && rs->getMultisampleResolveFramebufferObject())
                rs->setMultisampleResolveFramebufferObject(fbo);
        }

        mCanvases[frameId]->setPostProcessing(mUsePostProcessing);
        mCanvases[frameId]->setTextureNormals(mNormals ? getTexture(Tex_Normal, frameId) : nullptr);
        mCanvases[frameId]->setMask(mUnderwater, mExteriorFlag);
        mCanvases[frameId]->setCalculateAvgLum(mHDR);

        mCanvases[frameId]->setTextureScene(getTexture(Tex_Scene, frameId));
        mCanvases[frameId]->setTextureDepth(getTexture(Tex_Depth/*Tex_OpaqueDepth*/, frameId));
        mCanvases[frameId]->setTextureDistortion(getTexture(Tex_Distortion, frameId));

        mTransparentDepthPostPass->mFbo[frameId] = mFbos[frameId][FBO_Primary];
        mTransparentDepthPostPass->mMsaaFbo[frameId] = mFbos[frameId][FBO_Multisample];
        mTransparentDepthPostPass->mOpaqueFbo[frameId] = mFbos[frameId][FBO_OpaqueDepth];

        mDistortionCallback->setFBO(mFbos[frameId][FBO_Distortion], frameId);
        mDistortionCallback->setOriginalFBO(mFbos[frameId][FBO_Primary], frameId);

        size_t frame = cv->getTraversalNumber();

        mStateUpdater->setResolution(osg::Vec2f(cv->getViewport()->width(), cv->getViewport()->height()));

        // per-frame data
        if (frame != mLastFrameNumber)
        {
            mLastFrameNumber = frame;
            auto stamp = cv->getFrameStamp();

            mStateUpdater->setSimulationTime(static_cast<float>(stamp->getSimulationTime()));
            mStateUpdater->setDeltaSimulationTime(static_cast<float>(stamp->getSimulationTime() - mLastSimulationTime));
            // Use a signed int because 'uint' type is not supported in GLSL 120 without extensions
            mStateUpdater->setFrameNumber(static_cast<int>(stamp->getFrameNumber()));
            mLastSimulationTime = stamp->getSimulationTime();

            for (const auto& dispatchNode : mCanvases[frameId]->getPasses())
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
        if (!mEnableLiveReload && !mTriggerShaderReload)
            return;

        mTriggerShaderReload = false; // Done only once

        for (auto& technique : mTechniques)
        {
            if (!technique || technique->getStatus() == fx::Technique::Status::File_Not_exists)
                continue;

            const auto lastWriteTime = std::filesystem::last_write_time(mTechniqueFileMap[technique->getName()]);
            const bool isDirty = technique->setLastModificationTime(lastWriteTime);

            if (!isDirty)
                continue;

            // TODO: Temporary workaround to avoid conflicts with external programs saving the file, especially
            // problematic on Windows.
            //       If we move to a file watcher using native APIs this should be removed.
            std::this_thread::sleep_for(std::chrono::milliseconds(5));

            if (technique->compile())
                Log(Debug::Info) << "Reloaded technique : " << mTechniqueFileMap[technique->getName()];

            mReload = technique->isValid();
        }
    }

    void PostProcessor::reloadIfRequired()
    {
        if (!mReload)
            return;

        mReload = false;

        loadChain();
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

        mCanvases[frameId]->setNodeMask(~0u);
        mCanvases[!frameId]->setNodeMask(0);

        if (mDirty && mDirtyFrameId == frameId)
        {
            createObjectsForFrame(frameId);

            mDirty = false;
            mCanvases[frameId]->setPasses(fx::DispatchArray(mTemplateData));
        }

        if ((mNormalsSupported && mNormals != mPrevNormals) || (mPassLights != mPrevPassLights))
        {
            mPrevNormals = mNormals;
            mPrevPassLights = mPassLights;

            mViewer->stopThreading();

            if (mNormalsSupported)
            {
// Crashing with gl4es, use OSG define instead
/*
                auto& shaderManager
                    = MWBase::Environment::get().getResourceSystem()->getSceneManager()->getShaderManager();
                auto defines = shaderManager.getGlobalDefines();
                defines["disableNormals"] = mNormals ? "0" : "1";
                shaderManager.setGlobalDefines(defines);
*/
                mRendering.setWriteNormals(mNormals);
            }

            mRendering.getLightRoot()->setCollectPPLights(mPassLights);
            mStateUpdater->bindPointLights(mPassLights ? mRendering.getLightRoot()->getPPLightsBuffer() : nullptr);
            mStateUpdater->reset();

            mViewer->startThreading();

            createObjectsForFrame(frameId);

            mDirty = true;
            mDirtyFrameId = !frameId;
        }
    }

    void PostProcessor::createObjectsForFrame(size_t frameId)
    {
        auto& textures = mTextures[frameId];

        int width = renderWidth();
        int height = renderHeight();

        for (osg::ref_ptr<osg::Texture>& texture : textures)
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
            Stereo::setMultiviewCompatibleTextureSize(texture, width, height);
            texture->dirtyTextureObject();
        }

        textures[Tex_Normal]->setSourceFormat(GL_RGB);
        textures[Tex_Normal]->setInternalFormat(GL_RGB);

        textures[Tex_Distortion]->setSourceFormat(GL_RGB);
        textures[Tex_Distortion]->setInternalFormat(GL_RGB);

        Stereo::setMultiviewCompatibleTextureSize(
            textures[Tex_Distortion], width * DistortionRatio, height * DistortionRatio);
        textures[Tex_Distortion]->dirtyTextureObject();

        auto setupDepth = [](osg::Texture* tex) {
            tex->setSourceFormat(GL_DEPTH_STENCIL_EXT);
            tex->setSourceType(SceneUtil::AutoDepth::depthSourceType());
            tex->setInternalFormat(SceneUtil::AutoDepth::depthInternalFormat());
        };

        setupDepth(textures[Tex_Depth]);
        setupDepth(textures[Tex_OpaqueDepth]);
        textures[Tex_OpaqueDepth]->setName("opaqueTexMap");

        auto& fbos = mFbos[frameId];

        fbos[FBO_Primary] = new osg::FrameBufferObject;
        fbos[FBO_Primary]->setAttachment(
            osg::Camera::COLOR_BUFFER0, Stereo::createMultiviewCompatibleAttachment(textures[Tex_Scene]));
        if (mNormals && mNormalsSupported)
            fbos[FBO_Primary]->setAttachment(
                osg::Camera::COLOR_BUFFER1, Stereo::createMultiviewCompatibleAttachment(textures[Tex_Normal]));
        fbos[FBO_Primary]->setAttachment(
            osg::Camera::PACKED_DEPTH_STENCIL_BUFFER, Stereo::createMultiviewCompatibleAttachment(textures[Tex_Depth]));

        fbos[FBO_FirstPerson] = new osg::FrameBufferObject;

        auto fpDepthRb = createFrameBufferAttachmentFromTemplate(
            Usage::RENDER_BUFFER, width, height, textures[Tex_Depth], mSamples);
        fbos[FBO_FirstPerson]->setAttachment(osg::FrameBufferObject::BufferComponent::PACKED_DEPTH_STENCIL_BUFFER,
            osg::FrameBufferAttachment(fpDepthRb));

        if (mSamples > 1)
        {
            fbos[FBO_Multisample] = new osg::FrameBufferObject;
            fbos[FBO_Intercept] = new osg::FrameBufferObject;
            auto colorRB = createFrameBufferAttachmentFromTemplate(
                Usage::RENDER_BUFFER, width, height, textures[Tex_Scene], mSamples);
            if (mNormals && mNormalsSupported)
            {
                auto normalRB = createFrameBufferAttachmentFromTemplate(
                    Usage::RENDER_BUFFER, width, height, textures[Tex_Normal], mSamples);
                fbos[FBO_Multisample]->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER1, normalRB);
                fbos[FBO_FirstPerson]->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER1, normalRB);
                fbos[FBO_Intercept]->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER1,
                    Stereo::createMultiviewCompatibleAttachment(textures[Tex_Normal]));
            }
            auto depthRB = createFrameBufferAttachmentFromTemplate(
                Usage::RENDER_BUFFER, width, height, textures[Tex_Depth], mSamples);
            fbos[FBO_Multisample]->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0, colorRB);
            fbos[FBO_Multisample]->setAttachment(
                osg::FrameBufferObject::BufferComponent::PACKED_DEPTH_STENCIL_BUFFER, depthRB);
            fbos[FBO_FirstPerson]->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0, colorRB);

            fbos[FBO_Intercept]->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0,
                Stereo::createMultiviewCompatibleAttachment(textures[Tex_Scene]));
        }
        else
        {
            fbos[FBO_FirstPerson]->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0,
                Stereo::createMultiviewCompatibleAttachment(textures[Tex_Scene]));
            if (mNormals && mNormalsSupported)
                fbos[FBO_FirstPerson]->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER1,
                    Stereo::createMultiviewCompatibleAttachment(textures[Tex_Normal]));
        }

        fbos[FBO_OpaqueDepth] = new osg::FrameBufferObject;
        fbos[FBO_OpaqueDepth]->setAttachment(osg::FrameBufferObject::BufferComponent::PACKED_DEPTH_STENCIL_BUFFER,
            Stereo::createMultiviewCompatibleAttachment(textures[Tex_OpaqueDepth]));

        fbos[FBO_Distortion] = new osg::FrameBufferObject;
        fbos[FBO_Distortion]->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0,
            Stereo::createMultiviewCompatibleAttachment(textures[Tex_Distortion]));

#ifdef __APPLE__
        if (textures[Tex_OpaqueDepth])
            fbos[FBO_OpaqueDepth]->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER,
                osg::FrameBufferAttachment(new osg::RenderBuffer(textures[Tex_OpaqueDepth]->getTextureWidth(),
                    textures[Tex_OpaqueDepth]->getTextureHeight(), textures[Tex_Scene]->getInternalFormat())));
#endif

        mCanvases[frameId]->dirty();
    }

    void PostProcessor::dirtyTechniques(bool dirtyAttachments)
    {
        size_t frameId = frame() % 2;

        mDirty = true;
        mDirtyFrameId = !frameId;

        mTemplateData = {};

        bool sunglare = true;
        mHDR = false;
        mNormals = false;
        mPassLights = false;

        std::vector<fx::Types::RenderTarget> attachmentsToDirty;

        for (const auto& technique : mTechniques)
        {
            if (!technique || !technique->isValid())
                continue;

            if (technique->getGLSLVersion() > mGLSLVersion)
            {
                Log(Debug::Warning) << "Technique " << technique->getName() << " requires GLSL version "
                                    << technique->getGLSLVersion() << " which is unsupported by your hardware.";
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
            node.mRootStateSet->addUniform(new osg::Uniform("omw_SamplerDistortion", Unit_Distortion));

            if (mNormals)
                node.mRootStateSet->addUniform(new osg::Uniform("omw_SamplerNormals", Unit_Normals));

            if (technique->getHDR())
                node.mRootStateSet->addUniform(new osg::Uniform("omw_EyeAdaptation", Unit_EyeAdaptation));

            node.mRootStateSet->addUniform(new osg::Uniform("omw_SamplerDistortion", Unit_Distortion));

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
                if (uniform->mSamplerType)
                    continue;

                if (auto type = uniform->getType())
                    uniform->setUniform(node.mRootStateSet->getOrCreateUniform(
                        uniform->mName.c_str(), *type, uniform->getNumElements()));
            }

            for (const auto& pass : technique->getPasses())
            {
                int subTexUnit = texUnit;
                fx::DispatchNode::SubPass subPass;

                pass->prepareStateSet(subPass.mStateSet, technique->getName());

                node.mHandle = technique;

                if (!pass->getTarget().empty())
                {
                    auto& renderTarget = technique->getRenderTargetsMap()[pass->getTarget()];
                    subPass.mSize = renderTarget.mSize;
                    subPass.mRenderTexture = renderTarget.mTarget;
                    subPass.mMipMap = renderTarget.mMipMap;

                    const auto [w, h] = renderTarget.mSize.get(renderWidth(), renderHeight());
                    subPass.mStateSet->setAttributeAndModes(new osg::Viewport(0, 0, w, h));

                    subPass.mRenderTexture->setTextureSize(w, h);
                    subPass.mRenderTexture->dirtyTextureObject();

                    subPass.mRenderTarget = new osg::FrameBufferObject;
                    subPass.mRenderTarget->setAttachment(osg::FrameBufferObject::BufferComponent::COLOR_BUFFER0,
                        osg::FrameBufferAttachment(subPass.mRenderTexture));

                    if (std::find_if(attachmentsToDirty.cbegin(), attachmentsToDirty.cend(),
                            [renderTarget](const auto& rt) { return renderTarget.mTarget == rt.mTarget; })
                        == attachmentsToDirty.cend())
                    {
                        attachmentsToDirty.push_back(fx::Types::RenderTarget(renderTarget));
                    }
                }

                for (const auto& name : pass->getRenderTargets())
                {
                    if (name.empty())
                    {
                        continue;
                    }

                    auto& renderTarget = technique->getRenderTargetsMap()[name];
                    subPass.mStateSet->setTextureAttribute(subTexUnit, renderTarget.mTarget);
                    subPass.mStateSet->addUniform(new osg::Uniform(name.c_str(), subTexUnit));

                    if (std::find_if(attachmentsToDirty.cbegin(), attachmentsToDirty.cend(),
                            [renderTarget](const auto& rt) { return renderTarget.mTarget == rt.mTarget; })
                        == attachmentsToDirty.cend())
                    {
                        attachmentsToDirty.push_back(fx::Types::RenderTarget(renderTarget));
                    }
                    subTexUnit++;
                }

                node.mPasses.emplace_back(std::move(subPass));
            }

            node.compile();

            mTemplateData.emplace_back(std::move(node));
        }

        mCanvases[frameId]->setPasses(fx::DispatchArray(mTemplateData));

        if (auto hud = MWBase::Environment::get().getWindowManager()->getPostProcessorHud())
            hud->updateTechniques();

        mRendering.getSkyManager()->setSunglare(sunglare);

        if (dirtyAttachments)
            mCanvases[frameId]->setDirtyAttachments(attachmentsToDirty);
    }

    PostProcessor::Status PostProcessor::enableTechnique(
        std::shared_ptr<fx::Technique> technique, std::optional<int> location)
    {
        if (!technique || technique->getLocked() || (location.has_value() && location.value() < 0))
            return Status_Error;

        disableTechnique(technique, false);

        int pos = std::min<int>(location.value_or(mTechniques.size()) + mInternalTechniques.size(), mTechniques.size());

        mTechniques.insert(mTechniques.begin() + pos, technique);
        dirtyTechniques(Settings::ShaderManager::get().getMode() == Settings::ShaderManager::Mode::Debug);

        return Status_Toggled;
    }

    PostProcessor::Status PostProcessor::disableTechnique(std::shared_ptr<fx::Technique> technique, bool dirty)
    {
        if (!technique || technique->getLocked())
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
        if (!technique)
            return false;

        if (auto it = std::find(mTechniques.begin(), mTechniques.end(), technique); it == mTechniques.end())
            return false;

        return technique->isValid();
    }

    std::shared_ptr<fx::Technique> PostProcessor::loadTechnique(const std::string& name, bool loadNextFrame)
    {
        for (const auto& technique : mTemplates)
            if (Misc::StringUtils::ciEqual(technique->getName(), name))
                return technique;

        for (const auto& technique : mQueuedTemplates)
            if (Misc::StringUtils::ciEqual(technique->getName(), name))
                return technique;

        std::string realName = name;
        auto fileIter = mTechniqueFileMap.find(name);
        if (fileIter != mTechniqueFileMap.end())
            realName = fileIter->first;

        auto technique = std::make_shared<fx::Technique>(*mVFS, *mRendering.getResourceSystem()->getImageManager(),
            std::move(realName), renderWidth(), renderHeight(), mUBO, mNormalsSupported);

        technique->compile();

        if (technique->getStatus() != fx::Technique::Status::File_Not_exists)
            technique->setLastModificationTime(std::filesystem::last_write_time(fileIter->second));

        if (loadNextFrame)
        {
            mQueuedTemplates.push_back(technique);
            return technique;
        }

        mTemplates.push_back(std::move(technique));

        return mTemplates.back();
    }

    void PostProcessor::loadChain()
    {
        mTechniques.clear();

        for (const auto& technique : mInternalTechniques)
        {
            mTechniques.push_back(technique);
        }

        for (const std::string& techniqueName : Settings::postProcessing().mChain.get())
        {
            if (techniqueName.empty())
                continue;

            mTechniques.push_back(loadTechnique(techniqueName));
        }

        dirtyTechniques();
    }

    void PostProcessor::saveChain()
    {
        std::vector<std::string> chain;

        for (const auto& technique : mTechniques)
        {
            if (!technique || technique->getDynamic() || technique->getInternal())
                continue;
            chain.push_back(technique->getName());
        }

        Settings::postProcessing().mChain.set(chain);
    }

    void PostProcessor::toggleMode()
    {
        for (auto& technique : mTemplates)
            technique->compile();

        dirtyTechniques(true);
    }

    void PostProcessor::disableDynamicShaders()
    {
        for (auto& technique : mTechniques)
            if (technique && technique->getDynamic())
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

    void PostProcessor::triggerShaderReload()
    {
        mTriggerShaderReload = true;
    }
}
