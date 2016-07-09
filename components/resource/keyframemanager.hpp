#ifndef OPENMW_COMPONENTS_KEYFRAMEMANAGER_H
#define OPENMW_COMPONENTS_KEYFRAMEMANAGER_H

#include <osg/ref_ptr>
#include <string>

#include "resourcemanager.hpp"

namespace NifOsg
{
    class KeyframeHolder;
}

namespace Resource
{

    /// @brief Managing of keyframe resources
    /// @note May be used from any thread.
    class KeyframeManager : public ResourceManager
    {
    public:
        KeyframeManager(const VFS::Manager* vfs);
        ~KeyframeManager();

        /// Retrieve a read-only keyframe resource by name (case-insensitive).
        /// @note Throws an exception if the resource is not found.
        osg::ref_ptr<const NifOsg::KeyframeHolder> get(const std::string& name);
    };

}

#endif
