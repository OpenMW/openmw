#ifndef OPENMW_COMPONENTS_RESOURCE_SCENEMANAGER_H
#define OPENMW_COMPONENTS_RESOURCE_SCENEMANAGER_H

#include <string>
#include <map>
#include <memory>

#include <osg/ref_ptr>
#include <osg/Node>
#include <osg/Texture>

#include "resourcemanager.hpp"

namespace Resource
{
    class ImageManager;
    class NifFileManager;
}

namespace osgUtil
{
    class IncrementalCompileOperation;
}

namespace Shader
{
    class ShaderManager;
}

namespace Resource
{

    class MultiObjectCache;

    /// @brief Handles loading and caching of scenes, e.g. .nif files or .osg files
    /// @note Some methods of the scene manager can be used from any thread, see the methods documentation for more details.
    class SceneManager : public ResourceManager
    {
    public:
        SceneManager(const VFS::Manager* vfs, Resource::ImageManager* imageManager, Resource::NifFileManager* nifFileManager);
        ~SceneManager();

        Shader::ShaderManager& getShaderManager();

        /// Re-create shaders for this node, need to call this if texture stages or vertex color mode have changed.
        void recreateShaders(osg::ref_ptr<osg::Node> node);

        /// @see ShaderVisitor::setForceShaders
        void setForceShaders(bool force);
        bool getForceShaders() const;

        /// @see ShaderVisitor::setClampLighting
        void setClampLighting(bool clamp);
        bool getClampLighting() const;

        /// @see ShaderVisitor::setForcePerPixelLighting
        void setForcePerPixelLighting(bool force);
        bool getForcePerPixelLighting() const;

        /// @see ShaderVisitor::setAutoUseNormalMaps
        void setAutoUseNormalMaps(bool use);

        /// @see ShaderVisitor::setNormalMapPattern
        void setNormalMapPattern(const std::string& pattern);

        /// @see ShaderVisitor::setNormalHeightMapPattern
        void setNormalHeightMapPattern(const std::string& pattern);

        void setAutoUseSpecularMaps(bool use);

        void setSpecularMapPattern(const std::string& pattern);

        void setShaderPath(const std::string& path);

        /// Get a read-only copy of this scene "template"
        /// @note If the given filename does not exist or fails to load, an error marker mesh will be used instead.
        ///  If even the error marker mesh can not be found, an exception is thrown.
        /// @note Thread safe.
        osg::ref_ptr<const osg::Node> getTemplate(const std::string& name);

        /// Create an instance of the given scene template and cache it for later use, so that future calls to getInstance() can simply
        /// return this cached object instead of creating a new one.
        /// @note The returned ref_ptr may be kept around by the caller to ensure that the object stays in cache for as long as needed.
        /// @note Thread safe.
        osg::ref_ptr<osg::Node> cacheInstance(const std::string& name);

        /// Get an instance of the given scene template
        /// @see getTemplate
        /// @note Thread safe.
        osg::ref_ptr<osg::Node> getInstance(const std::string& name);

        /// Get an instance of the given scene template and immediately attach it to a parent node
        /// @see getTemplate
        /// @note Not thread safe, unless parentNode is not part of the main scene graph yet.
        osg::ref_ptr<osg::Node> getInstance(const std::string& name, osg::Group* parentNode);

        /// Attach the given scene instance to the given parent node
        /// @note You should have the parentNode in its intended position before calling this method,
        ///       so that world space particles of the \a instance get transformed correctly.
        /// @note Assumes the given instance was not attached to any parents before.
        /// @note Not thread safe, unless parentNode is not part of the main scene graph yet.
        void attachTo(osg::Node* instance, osg::Group* parentNode) const;

        /// Manually release created OpenGL objects for the given graphics context. This may be required
        /// in cases where multiple contexts are used over the lifetime of the application.
        void releaseGLObjects(osg::State* state);

        /// Set up an IncrementalCompileOperation for background compiling of loaded scenes.
        void setIncrementalCompileOperation(osgUtil::IncrementalCompileOperation* ico);

        /// @note SceneManager::attachTo calls this method automatically, only needs to be called by users if manually attaching
        void notifyAttached(osg::Node* node) const;

        Resource::ImageManager* getImageManager();

        /// @param mask The node mask to apply to loaded particle system nodes.
        void setParticleSystemMask(unsigned int mask);

        /// @warning It is unsafe to call this method while the draw thread is using textures! call Viewer::stopThreading first.
        void setFilterSettings(const std::string &magfilter, const std::string &minfilter,
                               const std::string &mipmap, int maxAnisotropy);

        /// Apply filter settings to the given texture. Note, when loading an object through this scene manager (i.e. calling getTemplate or createInstance)
        /// the filter settings are applied automatically. This method is provided for textures that were created outside of the SceneManager.
        void applyFilterSettings (osg::Texture* tex);

        /// Keep a copy of the texture data around in system memory? This is needed when using multiple graphics contexts,
        /// otherwise should be disabled to reduce memory usage.
        void setUnRefImageDataAfterApply(bool unref);

        /// @see ResourceManager::updateCache
        virtual void updateCache(double referenceTime);

    private:

        osg::ref_ptr<osg::Node> createInstance(const std::string& name);

        std::auto_ptr<Shader::ShaderManager> mShaderManager;
        bool mForceShaders;
        bool mClampLighting;
        bool mForcePerPixelLighting;
        bool mAutoUseNormalMaps;
        std::string mNormalMapPattern;
        std::string mNormalHeightMapPattern;
        bool mAutoUseSpecularMaps;
        std::string mSpecularMapPattern;

        osg::ref_ptr<MultiObjectCache> mInstanceCache;

        OpenThreads::Mutex mSharedStateMutex;

        Resource::ImageManager* mImageManager;
        Resource::NifFileManager* mNifFileManager;

        osg::Texture::FilterMode mMinFilter;
        osg::Texture::FilterMode mMagFilter;
        int mMaxAnisotropy;
        bool mUnRefImageDataAfterApply;

        osg::ref_ptr<osgUtil::IncrementalCompileOperation> mIncrementalCompileOperation;

        unsigned int mParticleSystemMask;

        SceneManager(const SceneManager&);
        void operator = (const SceneManager&);
    };

}

#endif
