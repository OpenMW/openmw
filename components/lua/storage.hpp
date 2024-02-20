#ifndef COMPONENTS_LUA_STORAGE_H
#define COMPONENTS_LUA_STORAGE_H

#include <map>
#include <sol/sol.hpp>
#include <stdexcept>

#include "asyncpackage.hpp"
#include "serialization.hpp"

namespace LuaUtil
{

    class LuaStorage
    {
    public:
        static void initLuaBindings(lua_State*);
        static sol::table initGlobalPackage(lua_State* lua, LuaStorage* globalStorage);
        static sol::table initLocalPackage(lua_State* lua, LuaStorage* globalStorage);
        static sol::table initPlayerPackage(lua_State* lua, LuaStorage* globalStorage, LuaStorage* playerStorage);
        static sol::table initMenuPackage(lua_State* lua, LuaStorage* globalStorage, LuaStorage* playerStorage);

        explicit LuaStorage(lua_State* lua)
            : mLua(lua)
            , mActive(false)
        {
        }

        void clearTemporaryAndRemoveCallbacks();
        void load(const std::filesystem::path& path);
        void save(const std::filesystem::path& path) const;

        sol::object getSection(std::string_view sectionName, bool readOnly, bool forMenuScripts = false);
        sol::object getMutableSection(std::string_view sectionName, bool forMenuScripts = false)
        {
            return getSection(sectionName, false, forMenuScripts);
        }
        sol::object getReadOnlySection(std::string_view sectionName) { return getSection(sectionName, true); }
        sol::table getAllSections(bool readOnly = false);

        void setSingleValue(std::string_view section, std::string_view key, const sol::object& value)
        {
            getSection(section)->set(key, value);
        }

        void setSectionValues(std::string_view section, const sol::optional<sol::table>& values)
        {
            getSection(section)->setAll(values);
        }

        class Listener
        {
        public:
            virtual ~Listener() = default;
            virtual void valueChanged(
                std::string_view section, std::string_view key, const sol::object& value) const = 0;
            virtual void sectionReplaced(std::string_view section, const sol::optional<sol::table>& values) const = 0;
        };
        void setListener(const Listener* listener) { mListener = listener; }
        void setActive(bool active) { mActive = active; }

    private:
        class Value
        {
        public:
            Value() {}
            Value(const sol::object& value)
                : mSerializedValue(serialize(value))
            {
            }
            sol::object getCopy(lua_State* L) const;
            sol::object getReadOnly(lua_State* L) const;

        private:
            std::string mSerializedValue;
            mutable sol::object mReadOnlyValue = sol::nil;
        };

        struct Section
        {
            explicit Section(LuaStorage* storage, std::string name)
                : mStorage(storage)
                , mSectionName(std::move(name))
            {
            }
            const Value& get(std::string_view key) const;
            void set(std::string_view key, const sol::object& value);
            void setAll(const sol::optional<sol::table>& values);
            sol::table asTable();
            void runCallbacks(sol::optional<std::string_view> changedKey);
            void throwIfCallbackRecursionIsTooDeep();

            LuaStorage* mStorage;
            std::string mSectionName;
            std::map<std::string, Value, std::less<>> mValues;
            std::vector<Callback> mCallbacks;
            std::vector<Callback> mMenuScriptsCallbacks; // menu callbacks are in a separate vector because we don't
                                                         // remove them in clear()
            bool mPermanent = true;
            static Value sEmpty;

            void checkIfActive() const { mStorage->checkIfActive(); }
        };
        struct SectionView
        {
            std::shared_ptr<Section> mSection;
            bool mReadOnly;
            bool mForMenuScripts = false;
        };

        const std::shared_ptr<Section>& getSection(std::string_view sectionName);

        lua_State* mLua;
        std::map<std::string_view, std::shared_ptr<Section>> mData;
        const Listener* mListener = nullptr;
        std::set<const Section*> mRunningCallbacks;
        bool mActive;
        void checkIfActive() const
        {
            if (!mActive)
                throw std::logic_error("Trying to access inactive storage");
        }
    };

}

#endif // COMPONENTS_LUA_STORAGE_H
