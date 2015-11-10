#ifndef OPENMW_COMPONENTS_RESOURCE_SCENEMANAGER_H
#define OPENMW_COMPONENTS_RESOURCE_SCENEMANAGER_H

#include <string>
#include <map>

#include <osg/ref_ptr>
#include <osg/Node>

namespace Resource
{
    class TextureManager;
}

namespace VFS
{
    class Manager;
}

namespace NifOsg
{
    class KeyframeHolder;
}

namespace osgUtil
{
    class IncrementalCompileOperation;
}

namespace Resource
{

    /// @brief Handles loading and caching of scenes, e.g. NIF files
    class SceneManager
    {
    public:
        SceneManager(const VFS::Manager* vfs, Resource::TextureManager* textureManager);
        ~SceneManager();

        /// Get a read-only copy of this scene "template"
        /// @note If the given filename does not exist or fails to load, an error marker mesh will be used instead.
        ///  If even the error marker mesh can not be found, an exception is thrown.
        osg::ref_ptr<const osg::Node> getTemplate(const std::string& name);

        /// Create an instance of the given scene template
        /// @see getTemplate
        osg::ref_ptr<osg::Node> createInstance(const std::string& name);

        /// Create an instance of the given scene template and immediately attach it to a parent node
        /// @see getTemplate
        osg::ref_ptr<osg::Node> createInstance(const std::string& name, osg::Group* parentNode);

        /// Attach the given scene instance to the given parent node
        /// @note You should have the parentNode in its intended position before calling this method,
        ///       so that world space particles of the \a instance get transformed correctly.
        /// @note Assumes the given instance was not attached to any parents before.
        void attachTo(osg::Node* instance, osg::Group* parentNode) const;

        /// Get a read-only copy of the given keyframe file.
        osg::ref_ptr<const NifOsg::KeyframeHolder> getKeyframes(const std::string& name);

        /// Manually release created OpenGL objects for the given graphics context. This may be required
        /// in cases where multiple contexts are used over the lifetime of the application.
        void releaseGLObjects(osg::State* state);

        /// Set up an IncrementalCompileOperation for background compiling of loaded scenes.
        void setIncrementalCompileOperation(osgUtil::IncrementalCompileOperation* ico);

        /// @note If you used SceneManager::attachTo, this was called automatically.
        void notifyAttached(osg::Node* node) const;

        const VFS::Manager* getVFS() const;

        Resource::TextureManager* getTextureManager();

        /// @param mask The node mask to apply to loaded particle system nodes.
        void setParticleSystemMask(unsigned int mask);

    private:
        const VFS::Manager* mVFS;
        Resource::TextureManager* mTextureManager;

        osg::ref_ptr<osgUtil::IncrementalCompileOperation> mIncrementalCompileOperation;

        unsigned int mParticleSystemMask;

        // observer_ptr?
        typedef std::map<std::string, osg::ref_ptr<const osg::Node> > Index;
        Index mIndex;

        typedef std::map<std::string, osg::ref_ptr<const NifOsg::KeyframeHolder> > KeyframeIndex;
        KeyframeIndex mKeyframeIndex;

        SceneManager(const SceneManager&);
        void operator = (const SceneManager&);
    };

}

#endif
