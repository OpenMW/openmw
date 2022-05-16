#ifndef OPENMW_MWRENDER_POSTPROCESSOR_H
#define OPENMW_MWRENDER_POSTPROCESSOR_H

#include <array>
#include <vector>
#include <string>
#include <unordered_map>

#include <filesystem>

#include <osg/Texture2D>
#include <osg/Group>
#include <osg/FrameBufferObject>
#include <osg/Camera>

#include <osgViewer/Viewer>

#include <components/fx/stateupdater.hpp>
#include <components/fx/technique.hpp>
#include <components/debug/debuglog.hpp>

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

    class PostProcessor : public osg::Group
    {
    public:
        using FBOArray = std::array<osg::ref_ptr<osg::FrameBufferObject>, 5>;
        using TextureArray = std::array<osg::ref_ptr<osg::Texture2D>, 5>;
        using TechniqueList = std::vector<std::shared_ptr<fx::Technique>>;

        enum TextureIndex
        {
            Tex_Scene,
            Tex_Scene_LDR,
            Tex_Depth,
            Tex_OpaqueDepth,
            Tex_Normal
        };

        enum FBOIndex
        {
            FBO_Primary,
            FBO_Multisample,
            FBO_FirstPerson,
            FBO_OpaqueDepth,
            FBO_Intercept
        };

        enum TextureUnits
        {
            Unit_LastShader = 0,
            Unit_LastPass,
            Unit_Depth,
            Unit_EyeAdaptation,
            Unit_Normals,
            Unit_NextFree
        };

        PostProcessor(RenderingManager& rendering, osgViewer::Viewer* viewer, osg::Group* rootNode, const VFS::Manager* vfs);

        ~PostProcessor();

        void traverse(osg::NodeVisitor& nv) override;

        osg::ref_ptr<osg::FrameBufferObject> getFbo(FBOIndex index, unsigned int frameId) { return mFbos[frameId][index]; }

        osg::ref_ptr<osg::Texture2D> getTexture(TextureIndex index, unsigned int frameId) { return mTextures[frameId][index]; }

        osg::ref_ptr<osg::FrameBufferObject> getPrimaryFbo(unsigned int frameId) { return mFbos[frameId][FBO_Multisample] ? mFbos[frameId][FBO_Multisample] : mFbos[frameId][FBO_Primary]; }

        osg::ref_ptr<fx::StateUpdater> getStateUpdater() { return mStateUpdater; }

        const TechniqueList& getTechniques() { return mTechniques; }

        const TechniqueList& getTemplates() const { return mTemplates; }

        osg::ref_ptr<PingPongCanvas> getCanvas() { return mPingPongCanvas; }

        const auto& getTechniqueMap() const { return mTechniqueFileMap; }

        void resize();

        bool enableTechnique(std::shared_ptr<fx::Technique> technique, std::optional<int> location = std::nullopt);

        bool disableTechnique(std::shared_ptr<fx::Technique> technique, bool dirty = true);

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

        bool isTechniqueEnabled(const std::shared_ptr<fx::Technique>& technique) const;

        void setExteriorFlag(bool exterior) { mExteriorFlag = exterior; }

        void setUnderwaterFlag(bool underwater) { mUnderwater = underwater; }

        void toggleMode();

        std::shared_ptr<fx::Technique> loadTechnique(const std::string& name, bool insert=true);

        void addTemplate(std::shared_ptr<fx::Technique> technique);

        bool isEnabled() const { return mUsePostProcessing && mEnabled; }

        bool softParticlesEnabled() const {return mSoftParticles; }

        bool getHDR() const { return mHDR; }

        void disable();

        void enable(bool usePostProcessing = true);

        void setRenderTargetSize(int width, int height) { mWidth = width; mHeight = height; }

    private:
        size_t frame() const { return mViewer->getFrameStamp()->getFrameNumber(); }

        void createObjectsForFrame(size_t frameId);

        void createTexturesAndCamera(size_t frameId);

        void reloadTechniques();

        void reloadMainPass(fx::Technique& technique);

        void dirtyTechniques();

        void update(size_t frameId);

        void cull(size_t frameId, osgUtil::CullVisitor* cv);

        osg::ref_ptr<osg::Group> mRootNode;
        osg::ref_ptr<osg::Camera> mHUDCamera;

        std::array<TextureArray, 2> mTextures;
        std::array<FBOArray, 2> mFbos;

        TechniqueList mTechniques;
        TechniqueList mTemplates;

        std::unordered_map<std::string, std::filesystem::path> mTechniqueFileMap;

        int mSamples;

        bool mDirty;
        size_t mDirtyFrameId;

        RenderingManager& mRendering;
        osgViewer::Viewer* mViewer;
        const VFS::Manager* mVFS;

        bool mReload;
        bool mEnabled;
        bool mUsePostProcessing;
        bool mSoftParticles;
        bool mDisableDepthPasses;

        size_t mLastFrameNumber;
        float mLastSimulationTime;

        bool mExteriorFlag;
        bool mUnderwater;
        bool mHDR;
        bool mNormals;
        bool mPrevNormals;
        bool mNormalsSupported;
        bool mPassLights;
        bool mPrevPassLights;
        bool mUBO;
        int mGLSLVersion;

        osg::ref_ptr<osg::Texture2D> mMainTemplate;

        osg::ref_ptr<fx::StateUpdater> mStateUpdater;
        osg::ref_ptr<PingPongCull> mPingPongCull;
        osg::ref_ptr<PingPongCanvas> mPingPongCanvas;
        osg::ref_ptr<TransparentDepthBinCallback> mTransparentDepthPostPass;
    
        int mWidth;
        int mHeight;
    };
}

#endif
