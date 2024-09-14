#ifndef OPENMW_COMPONENTS_RESOURCE_SCENEMANAGER_H
#define OPENMW_COMPONENTS_RESOURCE_SCENEMANAGER_H

#include <array>
#include <memory>
#include <mutex>
#include <string>

#include <osg/Texture>
#include <osg/ref_ptr>

#include "resourcemanager.hpp"

#include <components/sceneutil/lightmanager.hpp>
#include <filesystem>

namespace VFS
{
    class Manager;
}

namespace osg
{
    class Group;
    class Node;
    class Program;
    class State;
    class Stats;
}

namespace Resource
{
    class ImageManager;
    class NifFileManager;
    class BgsmFileManager;
    class SharedStateManager;
}

namespace osgUtil
{
    class IncrementalCompileOperation;
}

namespace Shader
{
    class ShaderManager;
    class ShaderVisitor;
}

namespace Resource
{
    class TemplateRef : public osg::Object
    {
    public:
        TemplateRef(const Object* object)
            : mObject(object)
        {
        }
        TemplateRef() {}
        TemplateRef(const TemplateRef& copy, const osg::CopyOp&)
            : mObject(copy.mObject)
        {
        }

        META_Object(Resource, TemplateRef)

    private:
        osg::ref_ptr<const Object> mObject;
    };

    class TemplateMultiRef : public osg::Object
    {
    public:
        TemplateMultiRef() {}
        TemplateMultiRef(const TemplateMultiRef& copy, const osg::CopyOp&)
            : mObjects(copy.mObjects)
        {
        }
        void addRef(const osg::Node* node);

        META_Object(Resource, TemplateMultiRef)

    private:
        std::vector<osg::ref_ptr<const Object>> mObjects;
    };

    /// @brief Handles loading and caching of scenes, e.g. .nif files or .osg files
    /// @note Some methods of the scene manager can be used from any thread, see the methods documentation for more
    /// details.
    class SceneManager : public ResourceManager
    {
    public:
        explicit SceneManager(const VFS::Manager* vfs, Resource::ImageManager* imageManager,
            Resource::NifFileManager* nifFileManager, Resource::BgsmFileManager* bgsmFileManager, double expiryDelay);
        ~SceneManager();

        Shader::ShaderManager& getShaderManager();

        /// Re-create shaders for this node, need to call this if alpha testing, texture stages or vertex color mode
        /// have changed.
        void recreateShaders(osg::ref_ptr<osg::Node> node, const std::string& shaderPrefix = "objects",
            bool forceShadersForNode = false, const osg::Program* programTemplate = nullptr);

        /// Applying shaders to a node may replace some fixed-function state.
        /// This restores it.
        /// When editing such state, it should be reinstated before the edits, and shaders should be recreated
        /// afterwards.
        void reinstateRemovedState(osg::ref_ptr<osg::Node> node);

        /// @see ShaderVisitor::setForceShaders
        void setForceShaders(bool force);
        bool getForceShaders() const;

        void setClampLighting(bool clamp);
        bool getClampLighting() const;

        /// @see ShaderVisitor::setAutoUseNormalMaps
        void setAutoUseNormalMaps(bool use);

        /// @see ShaderVisitor::setNormalMapPattern
        void setNormalMapPattern(const std::string& pattern);

        /// @see ShaderVisitor::setNormalHeightMapPattern
        void setNormalHeightMapPattern(const std::string& pattern);

        void setAutoUseSpecularMaps(bool use);

        void setSpecularMapPattern(const std::string& pattern);

        void setApplyLightingToEnvMaps(bool apply);

        void setSupportedLightingMethods(const SceneUtil::LightManager::SupportedMethods& supported);
        bool isSupportedLightingMethod(SceneUtil::LightingMethod method) const;

        void setOpaqueDepthTex(osg::ref_ptr<osg::Texture> texturePing, osg::ref_ptr<osg::Texture> texturePong);

        osg::ref_ptr<osg::Texture> getOpaqueDepthTex(size_t frame);

        enum class UBOBinding
        {
            // If we add more UBO's, we should probably assign their bindings dynamically according to the current count
            // of UBO's in the programTemplate
            LightBuffer,
            PostProcessor
        };
        void setLightingMethod(SceneUtil::LightingMethod method);
        SceneUtil::LightingMethod getLightingMethod() const;

        void setConvertAlphaTestToAlphaToCoverage(bool convert);
        void setAdjustCoverageForAlphaTest(bool adjustCoverage);

        void setShaderPath(const std::filesystem::path& path);

        /// Check if a given scene is loaded and if so, update its usage timestamp to prevent it from being unloaded
        bool checkLoaded(const std::string& name, double referenceTime);

        /// Get a read-only copy of this scene "template"
        /// @note If the given filename does not exist or fails to load, an error marker mesh will be used instead.
        ///  If even the error marker mesh can not be found, an exception is thrown.
        /// @note Thread safe.
        osg::ref_ptr<const osg::Node> getTemplate(VFS::Path::NormalizedView path, bool compile = true);

        /// Clone osg::Node safely.
        /// @note Thread safe.
        static osg::ref_ptr<osg::Node> cloneNode(const osg::Node* base);

        void shareState(osg::ref_ptr<osg::Node> node);

        /// Clone osg::Node and adjust it according to SceneManager's settings.
        /// @note Thread safe.
        osg::ref_ptr<osg::Node> getInstance(const osg::Node* base);

        /// Instance the given scene template.
        /// @see getTemplate
        /// @note Thread safe.
        osg::ref_ptr<osg::Node> getInstance(VFS::Path::NormalizedView path);

        /// Instance the given scene template and immediately attach it to a parent node
        /// @see getTemplate
        /// @note Not thread safe, unless parentNode is not part of the main scene graph yet.
        osg::ref_ptr<osg::Node> getInstance(VFS::Path::NormalizedView path, osg::Group* parentNode);

        /// Attach the given scene instance to the given parent node
        /// @note You should have the parentNode in its intended position before calling this method,
        ///       so that world space particles of the \a instance get transformed correctly.
        /// @note Assumes the given instance was not attached to any parents before.
        /// @note Not thread safe, unless parentNode is not part of the main scene graph yet.
        void attachTo(osg::Node* instance, osg::Group* parentNode) const;

        /// Manually release created OpenGL objects for the given graphics context. This may be required
        /// in cases where multiple contexts are used over the lifetime of the application.
        void releaseGLObjects(osg::State* state) override;

        /// Set up an IncrementalCompileOperation for background compiling of loaded scenes.
        void setIncrementalCompileOperation(osgUtil::IncrementalCompileOperation* ico);

        osgUtil::IncrementalCompileOperation* getIncrementalCompileOperation();

        Resource::ImageManager* getImageManager();

        /// @param mask The node mask to apply to loaded particle system nodes.
        void setParticleSystemMask(unsigned int mask);

        /// @warning It is unsafe to call this method while the draw thread is using textures! call
        /// Viewer::stopThreading first.
        void setFilterSettings(
            const std::string& magfilter, const std::string& minfilter, const std::string& mipmap, int maxAnisotropy);

        /// Apply filter settings to the given texture. Note, when loading an object through this scene manager (i.e.
        /// calling getTemplate or createInstance) the filter settings are applied automatically. This method is
        /// provided for textures that were created outside of the SceneManager.
        void applyFilterSettings(osg::Texture* tex);

        /// Keep a copy of the texture data around in system memory? This is needed when using multiple graphics
        /// contexts, otherwise should be disabled to reduce memory usage.
        void setUnRefImageDataAfterApply(bool unref);

        /// @see ResourceManager::updateCache
        void updateCache(double referenceTime) override;

        void clearCache() override;

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const override;

        void setSupportsNormalsRT(bool supports) { mSupportsNormalsRT = supports; }
        bool getSupportsNormalsRT() const { return mSupportsNormalsRT; }

        void setUpNormalsRTForStateSet(osg::StateSet* stateset, bool enabled);

        void setSoftParticles(bool enabled) { mSoftParticles = enabled; }
        bool getSoftParticles() const { return mSoftParticles; }

        void setWeatherParticleOcclusion(bool value) { mWeatherParticleOcclusion = value; }

    private:
        osg::ref_ptr<Shader::ShaderVisitor> createShaderVisitor(const std::string& shaderPrefix = "objects");
        osg::ref_ptr<osg::Node> loadErrorMarker();
        osg::ref_ptr<osg::Node> cloneErrorMarker();

        std::unique_ptr<Shader::ShaderManager> mShaderManager;
        bool mForceShaders;
        bool mClampLighting;
        bool mAutoUseNormalMaps;
        std::string mNormalMapPattern;
        std::string mNormalHeightMapPattern;
        bool mAutoUseSpecularMaps;
        std::string mSpecularMapPattern;
        bool mApplyLightingToEnvMaps;
        SceneUtil::LightingMethod mLightingMethod;
        SceneUtil::LightManager::SupportedMethods mSupportedLightingMethods;
        bool mConvertAlphaTestToAlphaToCoverage;
        bool mAdjustCoverageForAlphaTest;
        bool mSupportsNormalsRT;
        std::array<osg::ref_ptr<osg::Texture>, 2> mOpaqueDepthTex;
        bool mSoftParticles = false;
        bool mWeatherParticleOcclusion = false;

        osg::ref_ptr<Resource::SharedStateManager> mSharedStateManager;
        mutable std::mutex mSharedStateMutex;

        Resource::ImageManager* mImageManager;
        Resource::NifFileManager* mNifFileManager;
        Resource::BgsmFileManager* mBgsmFileManager;

        osg::Texture::FilterMode mMinFilter;
        osg::Texture::FilterMode mMagFilter;
        int mMaxAnisotropy;
        bool mUnRefImageDataAfterApply;

        osg::ref_ptr<osgUtil::IncrementalCompileOperation> mIncrementalCompileOperation;

        unsigned int mParticleSystemMask;
        mutable osg::ref_ptr<osg::Node> mErrorMarker;

        SceneManager(const SceneManager&);
        void operator=(const SceneManager&);
    };

    std::string getFileExtension(const std::string& file);
}

#endif
