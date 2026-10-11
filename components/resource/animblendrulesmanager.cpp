#include "animblendrulesmanager.hpp"

#include <array>

#include <components/vfs/manager.hpp>

#include <osg/Stats>
#include <osgAnimation/Animation>
#include <osgAnimation/BasicAnimationManager>
#include <osgAnimation/Channel>

#include <components/debug/debuglog.hpp>
#include <components/misc/pathhelpers.hpp>

#include <components/sceneutil/osgacontroller.hpp>
#include <components/vfs/pathutil.hpp>

#include <components/resource/scenemanager.hpp>

#include "objectcache.hpp"
#include "scenemanager.hpp"

namespace Resource
{
    using AnimBlendRules = SceneUtil::AnimBlendRules;

    AnimBlendRulesManager::AnimBlendRulesManager(const VFS::Manager* vfs, double expiryDelay)
        : ResourceManager(vfs, expiryDelay)
    {
    }

    std::shared_ptr<const AnimBlendRules> AnimBlendRulesManager::getRules(
        const VFS::Path::NormalizedView path, const VFS::Path::NormalizedView overridePath)
    {
        // Note: Providing a non-existing path but an existing overridePath is not supported!
        auto tmpl = loadRules(path);
        if (!tmpl)
            return nullptr;

        // Cached rules are immutable, so callers without overrides can share them. Only a ruleset with overrides
        // applied needs its own copy.
        if (overridePath.value().empty())
            return tmpl;

        auto blendRuleOverrides = loadRules(overridePath);
        if (!blendRuleOverrides)
            return tmpl;

        // Keep the cached sources referenced for as long as the merged rules live, so the cache doesn't expire and
        // reparse them while actors still use the result.
        struct MergedRules
        {
            std::shared_ptr<const AnimBlendRules> mBase;
            std::shared_ptr<const AnimBlendRules> mOverrides;
            AnimBlendRules mRules;
        };
        auto merged = std::make_shared<MergedRules>(MergedRules{ tmpl, blendRuleOverrides, *tmpl });
        merged->mRules.addOverrideRules(*blendRuleOverrides);
        return std::shared_ptr<const AnimBlendRules>(merged, &merged->mRules);
    }

    std::shared_ptr<const AnimBlendRules> AnimBlendRulesManager::loadRules(VFS::Path::NormalizedView path)
    {
        if (std::optional<std::shared_ptr<const AnimBlendRules>> cached = mCache->getRefFromObjectCacheOrNone(path))
            return *cached;

        std::shared_ptr<AnimBlendRules> blendRules = AnimBlendRules::fromFile(mVFS, path);
        mCache->addEntryToObjectCache(path.value(), blendRules);
        return blendRules;
    }

    void AnimBlendRulesManager::reportStats(unsigned int frameNumber, osg::Stats* stats) const
    {
        Resource::reportStats("Blending Rules", frameNumber, mCache->getStats(), *stats);
    }

}
