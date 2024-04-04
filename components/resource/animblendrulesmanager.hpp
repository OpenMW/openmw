#ifndef OPENMW_COMPONENTS_ANIMBLENDRULESMANAGER_H
#define OPENMW_COMPONENTS_ANIMBLENDRULESMANAGER_H

#include <osg/ref_ptr>
#include <string>

#include <components/sceneutil/animblendrules.hpp>

#include "resourcemanager.hpp"

namespace Resource
{
    /// @brief Managing of keyframe resources
    /// @note May be used from any thread.
    class AnimBlendRulesManager : public ResourceManager
    {
    public:
        explicit AnimBlendRulesManager(const VFS::Manager* vfs, double expiryDelay);
        ~AnimBlendRulesManager() = default;

        /// Retrieve a read-only keyframe resource by name (case-insensitive).
        /// @note Throws an exception if the resource is not found.
        osg::ref_ptr<const SceneUtil::AnimBlendRules> getRules(
            std::string_view path, std::string_view overridePath = "");

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const override;

    private:
        osg::ref_ptr<const SceneUtil::AnimBlendRules> loadRules(std::string_view path);

        const VFS::Manager* mVfs;
    };

}

#endif
