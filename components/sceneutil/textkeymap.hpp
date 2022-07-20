#ifndef OPENMW_COMPONENTS_SCENEUTIL_TEXTKEYMAP
#define OPENMW_COMPONENTS_SCENEUTIL_TEXTKEYMAP

#include <algorithm>
#include <map>
#include <set>
#include <string>

namespace SceneUtil
{
    class TextKeyMap
    {
    public:
        using ConstIterator = std::multimap<float, std::string>::const_iterator;

        auto begin() const noexcept
        {
            return mTextKeyByTime.begin();
        }

        auto end() const noexcept
        {
            return mTextKeyByTime.end();
        }

        auto rbegin() const noexcept
        {
            return mTextKeyByTime.rbegin();
        }

        auto rend() const noexcept
        {
            return mTextKeyByTime.rend();
        }

        auto lowerBound(float time) const
        {
            return mTextKeyByTime.lower_bound(time);
        }

        auto upperBound(float time) const
        {
            return mTextKeyByTime.upper_bound(time);
        }

        void emplace(float time, std::string&& textKey)
        {
            const auto separator = textKey.find(": ");
            if (separator != std::string::npos)
                mGroups.emplace(textKey.substr(0, separator));

            mTextKeyByTime.emplace(time, std::move(textKey));
        }

        bool empty() const noexcept
        {
            return mTextKeyByTime.empty();
        }

        auto findGroupStart(const std::string &groupName) const
        {
            return std::find_if(mTextKeyByTime.begin(), mTextKeyByTime.end(), IsGroupStart{groupName});
        }

        bool hasGroupStart(const std::string &groupName) const
        {
            return mGroups.count(groupName) > 0;
        }

    private:
        struct IsGroupStart
        {
            const std::string &mGroupName;

            bool operator ()(const std::multimap<float, std::string>::value_type& value) const
            {
                return value.second.compare(0, mGroupName.size(), mGroupName) == 0 &&
                        value.second.compare(mGroupName.size(), 2, ": ") == 0;
            }
        };

        std::set<std::string> mGroups;
        std::multimap<float, std::string> mTextKeyByTime;
    };
}

#endif
