#ifndef COMPONENTS_LUA_STORAGE_H
#define COMPONENTS_LUA_STORAGE_H

#include <map>
#include <sol/sol.hpp>

#include "serialization.hpp"

namespace LuaUtil
{

    class LuaStorage
    {
    public:
        static void initLuaBindings(lua_State*);

        explicit LuaStorage(lua_State* lua) : mLua(lua) {}

        void clearTemporary();
        void load(const std::string& path);
        void save(const std::string& path) const;

        sol::object getReadOnlySection(std::string_view sectionName);
        sol::object getMutableSection(std::string_view sectionName);
        sol::table getAllSections();

        void set(std::string_view section, std::string_view key, const sol::object& value) { getSection(section)->set(key, value); }

        using ListenerFn = std::function<void(std::string_view, std::string_view, const sol::object&)>;
        void setListener(ListenerFn fn) { mListener = std::move(fn); }

    private:
        class Value
        {
        public:
            Value() {}
            Value(const sol::object& value) : mSerializedValue(serialize(value)) {}
            sol::object getCopy(lua_State* L) const;
            sol::object getReadOnly(lua_State* L) const;

        private:
            std::string mSerializedValue;
            mutable sol::object mReadOnlyValue = sol::nil;
        };

        struct Section
        {
            explicit Section(LuaStorage* storage, std::string name) : mStorage(storage), mSectionName(std::move(name)) {}
            const Value& get(std::string_view key) const;
            void set(std::string_view key, const sol::object& value);
            bool wasChanged(int64_t& lastCheck);
            sol::table asTable();

            LuaStorage* mStorage;
            std::string mSectionName;
            std::map<std::string, Value, std::less<>> mValues;
            bool mPermanent = true;
            int64_t mChangeCounter = 0;
            static Value sEmpty;
        };
        struct SectionMutableView
        {
            Section* mSection = nullptr;
            int64_t mLastCheck = 0;
        };
        struct SectionReadOnlyView
        {
            Section* mSection = nullptr;
            int64_t mLastCheck = 0;
        };

        Section* getSection(std::string_view sectionName);

        lua_State* mLua;
        std::map<std::string_view, std::unique_ptr<Section>> mData;
        std::optional<ListenerFn> mListener;
    };

}

#endif // COMPONENTS_LUA_STORAGE_H
