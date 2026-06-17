#ifndef COMPONENTS_LUA_INPUTACTIONS
#define COMPONENTS_LUA_INPUTACTIONS

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <sol/sol.hpp>

#include <components/lua/asyncpackage.hpp>
#include <components/lua/scriptscontainer.hpp>
#include <components/misc/algorithm.hpp>

namespace LuaUtil::InputAction
{
    enum class Type
    {
        Boolean,
        Number,
        Range,
    };

    struct Info
    {
        std::string mKey;
        Type mType;
        std::string mL10n;
        std::string mName;
        std::string mDescription;
        sol::main_object mDefaultValue;
        bool mPersistent;
    };

    class MultiTree
    {
    public:
        using Node = size_t;

        Node insert();
        bool multiEdge(Node target, const std::vector<Node>& source);
        size_t size() const { return mParents.size(); }

        template <typename Function> // Function = void(Node)
        void traverse(Function callback) const;

        void clear()
        {
            mParents.clear();
            mChildren.clear();
        }

    private:
        std::vector<std::vector<Node>> mParents;
        std::vector<std::vector<Node>> mChildren;

        bool validateTree() const;
    };

    class Registry
    {
    public:
        using ConstIterator = std::vector<Info>::const_iterator;
        void insert(const Info& info);
        size_t size() const { return mKeys.size(); }
        std::optional<std::string> firstKey() const { return mKeys.empty() ? std::nullopt : std::optional(mKeys[0]); }
        std::optional<std::string> nextKey(std::string_view key) const;
        std::optional<Info> operator[](std::string_view actionKey);
        bool bind(
            std::string_view key, const LuaUtil::Callback& callback, const std::vector<std::string_view>& dependencies);
        sol::object valueOfType(std::string_view key, Type type);
        void update(double dt);
        void registerHandler(std::string_view key, const LuaUtil::Callback& handler)
        {
            mHandlers[safeIdByKey(key)].push_back(handler);
        }
        void clear(bool force = false);

    private:
        using Id = MultiTree::Node;
        Id safeIdByKey(std::string_view key);
        struct Binding
        {
            LuaUtil::Callback mCallback;
            std::vector<Id> mDependencies;
        };
        std::vector<std::string> mKeys;
        std::unordered_map<std::string, Id, Misc::StringUtils::StringHash, std::equal_to<>> mIds;
        std::vector<Info> mInfo;
        std::vector<std::vector<LuaUtil::Callback>> mHandlers;
        std::vector<std::vector<Binding>> mBindings;
        std::vector<sol::object> mValues;
        MultiTree mBindingTree;
    };
}

namespace LuaUtil::InputTrigger
{
    struct Info
    {
        std::string mKey;
        std::string mL10n;
        std::string mName;
        std::string mDescription;
        bool mPersistent;
    };

    class Registry
    {
    public:
        std::optional<std::string> firstKey() const
        {
            return mIds.empty() ? std::nullopt : std::optional(mIds.begin()->first);
        }
        std::optional<std::string> nextKey(std::string_view key) const
        {
            auto it = mIds.find(key);
            if (it == mIds.end() || ++it == mIds.end())
                return std::nullopt;
            return it->first;
        }
        std::optional<Info> operator[](std::string_view key);
        void insert(const Info& info);
        void registerHandler(std::string_view key, const LuaUtil::Callback& callback);
        void activate(std::string_view key);
        void clear(bool force = false);

    private:
        using Id = size_t;
        Id safeIdByKey(std::string_view key);
        std::unordered_map<std::string, Id, Misc::StringUtils::StringHash, std::equal_to<>> mIds;
        std::vector<Info> mInfo;
        std::vector<std::vector<LuaUtil::Callback>> mHandlers;
    };
}

#endif // COMPONENTS_LUA_INPUTACTIONS
