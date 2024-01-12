#ifndef OPENMW_MWRENDER_POSTPROCESSOR_H
#define OPENMW_MWRENDER_POSTPROCESSOR_H

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

#include <filesystem>

#include <osg/Camera>
#include <osg/FrameBufferObject>
#include <osg/Group>
#include <osg/Texture2D>

#include <osgViewer/Viewer>

#include <components/debug/debuglog.hpp>
#include <components/fx/stateupdater.hpp>
#include <components/fx/technique.hpp>

#include "pingpongcanvas.hpp"
#include "transparentpass.hpp"

#include <memory>

namespace osgViewer
{
    class Viewer;
}

namespace Stereo
{
    class MultiviewFramebuffer;
}

namespace VFS
{
    class Manager;
}

namespace Shader
{
    class ShaderManager;
}

namespace MWRender
{
    class RenderingManager;
    class PingPongCull;
    class PingPongCanvas;
    class TransparentDepthBinCallback;
    class DistortionCallback;

    class PostProcessor : public osg::Group
    {
    public:
        using FBOArray = std::array<osg::ref_ptr<osg::FrameBufferObject>, 6>;
        using TextureArray = std::array<osg::ref_ptr<osg::Texture>, 6>;
        using TechniqueList = std::vector<std::shared_ptr<fx::Technique>>;

        enum TextureIndex
        {
            Tex_Scene,
            Tex_Scene_LDR,
            Tex_Depth,
            Tex_OpaqueDepth,
            Tex_Normal,
            Tex_Distortion,
        };

        enum FBOIndex
        {
            FBO_Primary,
            FBO_Multisample,
            FBO_FirstPerson,
            FBO_OpaqueDepth,
            FBO_Intercept,
            FBO_Distortion,
        };

        enum TextureUnits
        {
            Unit_LastShader = 0,
            Unit_LastPass,
            Unit_Depth,
            Unit_EyeAdaptation,
            Unit_Normals,
            Unit_Distortion,
            Unit_NextFree
        };

        enum Status
        {
            Status_Error,
            Status_Toggled,
            Status_Unchanged
        };

        PostProcessor(
            RenderingManager& rendering, osgViewer::Viewer* viewer, osg::Group* rootNode, const VFS::Manager* vfs);

        ~PostProcessor();

        void traverse(osg::NodeVisitor& nv) override;

        osg::ref_ptr<osg::FrameBufferObject> getFbo(FBOIndex index, unsigned int frameId)
        {
            return mFbos[frameId][index];
        }

        osg::ref_ptr<osg::Texture> getTexture(TextureIndex index, unsigned int frameId)
        {
            return mTextures[frameId][index];
        }

        osg::ref_ptr<osg::FrameBufferObject> getPrimaryFbo(unsigned int frameId)
        {
            return mFbos[frameId][FBO_Multisample] ? mFbos[frameId][FBO_Multisample] : mFbos[frameId][FBO_Primary];
        }

        osg::ref_ptr<osg::Camera> getHUDCamera() { return mHUDCamera; }

        osg::ref_ptr<fx::StateUpdater> getStateUpdater() { return mStateUpdater; }

        const TechniqueList& getTechniques() { return mTechniques; }

        const TechniqueList& getTemplates() const { return mTemplates; }

        const auto& getTechniqueMap() const { return mTechniqueFileMap; }

        void resize();

        Status enableTechnique(std::shared_ptr<fx::Technique> technique, std::optional<int> location = std::nullopt);

        Status disableTechnique(std::shared_ptr<fx::Technique> technique, bool dirty = true);

        bool getSupportsNormalsRT() const { return mNormalsSupported; }

        template <class T>
        void setUniform(std::shared_ptr<fx::Technique> technique, const std::string& name, const T& value)
        {
            if (!isEnabled())
                return;

            auto it = technique->findUniform(name);

            if (it == technique->getUniformMap().end())
                return;

            if ((*it)->mStatic)
            {
                Log(Debug::Warning) << "Attempting to set a configration variable [" << name << "] as a uniform";
                return;
            }

            (*it)->setValue(value);
        }

        std::optional<size_t> getUniformSize(std::shared_ptr<fx::Technique> technique, const std::string& name)
        {
            auto it = technique->findUniform(name);

            if (it == technique->getUniformMap().end())
                return std::nullopt;

            return (*it)->getNumElements();
        }

        bool isTechniqueEnabled(const std::shared_ptr<fx::Technique>& technique) const;

        void setExteriorFlag(bool exterior) { mExteriorFlag = exterior; }

        void setUnderwaterFlag(bool underwater) { mUnderwater = underwater; }

        void toggleMode();

        std::shared_ptr<fx::Technique> loadTechnique(const std::string& name, bool loadNextFrame = false);

        bool isEnabled() const { return mUsePostProcessing; }

        void disable();

        void enable();

        void setRenderTargetSize(int width, int height)
        {
            mWidth = width;
            mHeight = height;
        }

        void disableDynamicShaders();

        int renderWidth() const;
        int renderHeight() const;

        void triggerShaderReload();

        bool mEnableLiveReload = false;

        void loadChain();
        void saveChain();

    private:
        void populateTechniqueFiles();

        size_t frame() const { return mViewer->getFrameStamp()->getFrameNumber(); }

        void createObjectsForFrame(size_t frameId);

        void dirtyTechniques(bool dirtyAttachments = false);

        void update(size_t frameId);

        void reloadIfRequired();

        void updateLiveReload();

        void cull(size_t frameId, osgUtil::CullVisitor* cv);

        osg::ref_ptr<osg::Group> mRootNode;
        osg::ref_ptr<osg::Camera> mHUDCamera;

        std::array<TextureArray, 2> mTextures;
        std::array<FBOArray, 2> mFbos;

        TechniqueList mTechniques;
        TechniqueList mTemplates;
        TechniqueList mQueuedTemplates;
        TechniqueList mInternalTechniques;

        std::unordered_map<std::string, std::filesystem::path> mTechniqueFileMap;

        RenderingManager& mRendering;
        osgViewer::Viewer* mViewer;
        const VFS::Manager* mVFS;

        size_t mDirtyFrameId = 0;
        size_t mLastFrameNumber = 0;
        float mLastSimulationTime = 0.f;

        bool mDirty = false;
        bool mReload = true;
        bool mTriggerShaderReload = false;
        bool mUsePostProcessing = false;

        bool mUBO = false;
        bool mHDR = false;
        bool mNormals = false;
        bool mUnderwater = false;
        bool mPassLights = false;
        bool mPrevNormals = false;
        bool mExteriorFlag = false;
        bool mNormalsSupported = false;
        bool mPrevPassLights = false;

        int mGLSLVersion;
        int mWidth;
        int mHeight;
        int mSamples;

        osg::ref_ptr<fx::StateUpdater> mStateUpdater;
        osg::ref_ptr<PingPongCull> mPingPongCull;
        std::array<osg::ref_ptr<PingPongCanvas>, 2> mCanvases;
        osg::ref_ptr<TransparentDepthBinCallback> mTransparentDepthPostPass;
        osg::ref_ptr<DistortionCallback> mDistortionCallback;

        fx::DispatchArray mTemplateData;
    };
}

#endif
