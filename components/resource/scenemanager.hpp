#ifndef OPENMW_COMPONENTS_RESOURCE_SCENEMANAGER_H
#define OPENMW_COMPONENTS_RESOURCE_SCENEMANAGER_H

#include <string>
#include <map>

#include <osg/ref_ptr>
#include <osg/Node>

namespace VFS
{
    class Manager;
}

namespace Resource
{

    /// @brief Handles loading and caching of scenes, e.g. NIF files
    class SceneManager
    {
    public:
        SceneManager(const VFS::Manager* vfs);

        /// Get a read-only copy of this scene "template"
        osg::ref_ptr<const osg::Node> getTemplate(const std::string& name);

        /// Create an instance of the given scene template
        osg::ref_ptr<osg::Node> createInstance(const std::string& name);

        /// Create an instance of the given scene template and immediately attach it to a parent node
        osg::ref_ptr<osg::Node> createInstance(const std::string& name, osg::Group* parentNode);

        /// Attach the given scene instance to the given parent node
        /// @note You should have the parentNode in its intended position before calling this method,
        ///       so that world space particles of the \a instance get transformed correctly.
        /// @note Assumes the given instance was not attached to any parents before.
        void attachTo(osg::Node* instance, osg::Group* parentNode) const;

    private:
        const VFS::Manager* mVFS;

        // observer_ptr?
        typedef std::map<std::string, osg::ref_ptr<const osg::Node> > Index;
        Index mIndex;
    };

}

#endif
