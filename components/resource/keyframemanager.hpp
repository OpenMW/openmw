#ifndef OPENMW_COMPONENTS_KEYFRAMEMANAGER_H
#define OPENMW_COMPONENTS_KEYFRAMEMANAGER_H

#include <osg/ref_ptr>
#include <string>

#include <components/sceneutil/keyframe.hpp>

#include "resourcemanager.hpp"

namespace Resource
{

    class SceneManager;

    /// @brief Managing of keyframe resources
    /// @note May be used from any thread.
    class KeyframeManager : public ResourceManager
    {
    public:
        KeyframeManager(const VFS::Manager* vfs, SceneManager* sceneManager);
        ~KeyframeManager();

        /// Retrieve a read-only keyframe resource by name (case-insensitive).
        /// @note Throws an exception if the resource is not found.
        osg::ref_ptr<const SceneUtil::KeyframeHolder> get(const std::string& name);

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const override;
    private:
        SceneManager* mSceneManager;
    };

}

#endif
