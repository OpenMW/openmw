#include "animblendrules.hpp"

#include <iterator>
#include <map>

#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/format.hpp>
#include <components/misc/strings/lower.hpp>

#include <components/debug/debuglog.hpp>
#include <components/files/configfileparser.hpp>
#include <components/files/conversion.hpp>
#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/textkeymap.hpp>

#include <stdexcept>
#include <yaml-cpp/yaml.h>

namespace SceneUtil
{
    namespace
    {
        std::pair<std::string, std::string> splitRuleName(std::string full)
        {
            std::string group;
            std::string key;
            size_t delimiterInd = full.find(":");

            Misc::StringUtils::lowerCaseInPlace(full);

            if (delimiterInd == std::string::npos)
            {
                group = full;
                Misc::StringUtils::trim(group);
            }
            else
            {
                group = full.substr(0, delimiterInd);
                key = full.substr(delimiterInd + 1);
                Misc::StringUtils::trim(group);
                Misc::StringUtils::trim(key);
            }
            return std::make_pair(group, key);
        }

    }

    using BlendRule = AnimBlendRules::BlendRule;

    AnimBlendRules::AnimBlendRules(const AnimBlendRules& copy, const osg::CopyOp& copyop)
        : mRules(copy.mRules)
    {
    }

    AnimBlendRules::AnimBlendRules(const std::vector<BlendRule>& rules)
        : mRules(rules)
    {
    }

    osg::ref_ptr<AnimBlendRules> AnimBlendRules::fromFile(const VFS::Manager* vfs, VFS::Path::NormalizedView configPath)
    {
        Log(Debug::Debug) << "Attempting to load animation blending config '" << configPath << "'";

        if (!vfs->exists(configPath))
            return nullptr;

        // Retrieving and parsing animation rules
        std::string rawYaml(std::istreambuf_iterator<char>(*vfs->get(configPath)), {});

        std::vector<BlendRule> rules;

        YAML::Node root = YAML::Load(rawYaml);

        if (!root.IsDefined() || root.IsNull() || root.IsScalar())
        {
            Log(Debug::Error) << Misc::StringUtils::format(
                "Can't parse file '%s'. Check that it's a valid YAML/JSON file.", configPath);
            return nullptr;
        }

        if (root["blending_rules"])
        {
            for (const auto& it : root["blending_rules"])
            {
                if (it["from"] && it["to"] && it["duration"] && it["easing"])
                {
                    auto fromNames = splitRuleName(it["from"].as<std::string>());
                    auto toNames = splitRuleName(it["to"].as<std::string>());

                    BlendRule ruleObj = {
                        .mFromGroup = fromNames.first,
                        .mFromKey = fromNames.second,
                        .mToGroup = toNames.first,
                        .mToKey = toNames.second,
                        .mDuration = it["duration"].as<float>(),
                        .mEasing = it["easing"].as<std::string>(),
                    };

                    rules.emplace_back(ruleObj);
                }
                else
                {
                    Log(Debug::Warning) << "Warning: Blending rule '"
                                        << (it["from"] ? it["from"].as<std::string>() : "undefined") << "->"
                                        << (it["to"] ? it["to"].as<std::string>() : "undefined")
                                        << "' is missing some properties. File: '" << configPath << "'.";
                }
            }
        }
        else
        {
            throw std::domain_error(
                Misc::StringUtils::format("'blending_rules' object not found in '%s' file!", configPath));
        }

        // If no rules then dont allocate any instance
        if (rules.size() == 0)
            return nullptr;

        return new AnimBlendRules(rules);
    }

    void AnimBlendRules::addOverrideRules(const AnimBlendRules& overrideRules)
    {
        auto rules = overrideRules.getRules();
        // Concat the rules together, overrides added at the end since the bottom-most rule has the highest priority.
        mRules.insert(mRules.end(), rules.begin(), rules.end());
    }

    inline bool AnimBlendRules::fitsRuleString(const std::string_view str, const std::string_view ruleStr) const
    {
        // A wildcard only supported in the beginning or the end of the rule string in hopes that this will be more
        // performant. And most likely this kind of support is enough.
        return ruleStr == "*" || str == ruleStr || (ruleStr.starts_with("*") && str.ends_with(ruleStr.substr(1)))
            || (ruleStr.ends_with("*") && str.starts_with(ruleStr.substr(0, ruleStr.length() - 1)));
    }

    std::optional<BlendRule> AnimBlendRules::findBlendingRule(
        std::string fromGroup, std::string fromKey, std::string toGroup, std::string toKey) const
    {
        Misc::StringUtils::lowerCaseInPlace(fromGroup);
        Misc::StringUtils::lowerCaseInPlace(fromKey);
        Misc::StringUtils::lowerCaseInPlace(toGroup);
        Misc::StringUtils::lowerCaseInPlace(toKey);
        for (auto rule = mRules.rbegin(); rule != mRules.rend(); ++rule)
        {
            bool fromMatch = false;
            bool toMatch = false;

            // Pseudocode:
            // If not a wildcard and found a wildcard
            // starts with substr(0,wildcard)
            if (fitsRuleString(fromGroup, rule->mFromGroup)
                && (rule->mFromKey.empty() || fitsRuleString(fromKey, rule->mFromKey)))
            {
                fromMatch = true;
            }

            if ((fitsRuleString(toGroup, rule->mToGroup) || (rule->mToGroup == "$" && toGroup == fromGroup))
                && (rule->mToKey.empty() || fitsRuleString(toKey, rule->mToKey)))
            {
                toMatch = true;
            }

            if (fromMatch && toMatch)
                return std::make_optional<BlendRule>(*rule);
        }

        return std::nullopt;
    }

}
