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

    osg::ref_ptr<const AnimBlendRules> AnimBlendRulesManager::getRules(
        const VFS::Path::NormalizedView path, const VFS::Path::NormalizedView overridePath)
    {
        // Note: Providing a non-existing path but an existing overridePath is not supported!
        auto tmpl = loadRules(path);
        if (!tmpl)
            return nullptr;

        // Create an instance based on template and store template reference inside so the template will not be removed
        // from cache
        osg::ref_ptr<SceneUtil::AnimBlendRules> blendRules(new AnimBlendRules(*tmpl, osg::CopyOp::SHALLOW_COPY));
        blendRules->getOrCreateUserDataContainer()->addUserObject(new Resource::TemplateRef(tmpl));

        if (!overridePath.value().empty())
        {
            auto blendRuleOverrides = loadRules(overridePath);
            if (blendRuleOverrides)
            {
                blendRules->addOverrideRules(*blendRuleOverrides);
            }
            blendRules->getOrCreateUserDataContainer()->addUserObject(new Resource::TemplateRef(blendRuleOverrides));
        }

        return blendRules;
    }

    osg::ref_ptr<const AnimBlendRules> AnimBlendRulesManager::loadRules(VFS::Path::NormalizedView path)
    {
        std::optional<osg::ref_ptr<osg::Object>> obj = mCache->getRefFromObjectCacheOrNone(path);
        if (obj.has_value())
        {
            return osg::ref_ptr<AnimBlendRules>(static_cast<AnimBlendRules*>(obj->get()));
        }

        osg::ref_ptr<AnimBlendRules> blendRules = AnimBlendRules::fromFile(mVFS, path);
        mCache->addEntryToObjectCache(path.value(), blendRules);
        return blendRules;
    }

    void AnimBlendRulesManager::reportStats(unsigned int frameNumber, osg::Stats* stats) const
    {
        Resource::reportStats("Blending Rules", frameNumber, mCache->getStats(), *stats);
    }

}
