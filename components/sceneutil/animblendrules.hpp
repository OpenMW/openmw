#ifndef OPENMW_COMPONENTS_SCENEUTIL_ANIMBLENDRULES_HPP
#define OPENMW_COMPONENTS_SCENEUTIL_ANIMBLENDRULES_HPP

#include <optional>
#include <string>
#include <vector>

#include <osg/Object>

#include <components/vfs/manager.hpp>

namespace SceneUtil
{
    class AnimBlendRules : public osg::Object
    {
    public:
        struct BlendRule
        {
            std::string mFromGroup;
            std::string mFromKey;
            std::string mToGroup;
            std::string mToKey;
            float mDuration;
            std::string mEasing;
        };

        AnimBlendRules() = default;
        AnimBlendRules(const std::vector<BlendRule>& rules);
        AnimBlendRules(const AnimBlendRules& copy, const osg::CopyOp& copyop);

        META_Object(SceneUtil, AnimBlendRules)

        void addOverrideRules(const AnimBlendRules& overrideRules);

        std::optional<BlendRule> findBlendingRule(
            std::string fromGroup, std::string fromKey, std::string toGroup, std::string toKey) const;

        const std::vector<BlendRule>& getRules() const { return mRules; }

        static osg::ref_ptr<AnimBlendRules> fromFile(const VFS::Manager* vfs, VFS::Path::NormalizedView yamlpath);

    private:
        std::vector<BlendRule> mRules;

        inline bool fitsRuleString(const std::string_view& str, const std::string_view& ruleStr) const;
    };
}

#endif
