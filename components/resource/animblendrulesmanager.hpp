#ifndef OPENMW_COMPONENTS_ANIMBLENDRULESMANAGER_H
#define OPENMW_COMPONENTS_ANIMBLENDRULESMANAGER_H

#include <memory>
#include <string>

#include <components/sceneutil/animblendrules.hpp>

#include "resourcemanager.hpp"

namespace Resource
{
    /// @brief Managing of keyframe resources
    /// @note May be used from any thread.
    class AnimBlendRulesManager : public ResourceManager<std::shared_ptr<const SceneUtil::AnimBlendRules>>
    {
    public:
        explicit AnimBlendRulesManager(const VFS::Manager* vfs, double expiryDelay);
        ~AnimBlendRulesManager() = default;

        /// Retrieve a read-only keyframe resource by name (case-insensitive).
        /// @note Throws an exception if the resource is not found.
        std::shared_ptr<const SceneUtil::AnimBlendRules> getRules(
            const VFS::Path::NormalizedView path, const VFS::Path::NormalizedView overridePath);

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const override;

    private:
        std::shared_ptr<const SceneUtil::AnimBlendRules> loadRules(VFS::Path::NormalizedView path);
    };

}

#endif
