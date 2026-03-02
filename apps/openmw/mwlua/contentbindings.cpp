#include "contentbindings.hpp"

#include <components/esm3/loaddoor.hpp>
#include <components/esm3/loadstat.hpp>
#include <components/lua/util.hpp>

#include "context.hpp"
#include "types/modelproperty.hpp"
#include "types/types.hpp"

#include "../mwbase/environment.hpp"
#include "../mwworld/esmstore.hpp"

namespace
{
    template <class T>
    struct MutableStore
    {
        MWWorld::Store<T>& mStore;
    };

    template <class T>
    using TableToRecord = T (*)(const sol::table&);

    ESM::RefId validateId(std::string_view value)
    {
        if (value.empty())
            throw std::runtime_error("ID cannot be empty");
        if (auto id = ESM::StringRefId::deserializeExisting(value))
            return *id;
        // Check if this ID would be interpreted as something other than a StringRefId
        ESM::RefId id = ESM::RefId::deserializeText(value);
        if (!id.empty())
            throw std::runtime_error("Non-string ID not allowed");
        return ESM::RefId::stringRefId(value);
    }
}

namespace sol
{
    template <class T>
    struct is_automagical<MutableStore<T>> : std::false_type
    {
    };

    template <class T>
    struct is_automagical<MWLua::MutableRecord<T>> : std::false_type
    {
    };
}

namespace MWLua
{
    namespace
    {
        sol::table initGlobalVariableBindings(sol::state_view& lua, MWWorld::Store<ESM::Global>& store)
        {
            using Store = MutableStore<ESM::Global>;
            sol::usertype<Store> storeT = lua.new_usertype<Store>("GlobalsContentStore");
            storeT[sol::meta_function::length] = [](const Store& self) { return self.mStore.getSize(); };
            storeT[sol::meta_function::index] = sol::overload(
                [](const Store& self, size_t index) -> std::optional<float> {
                    if (index == 0 || index > self.mStore.getSize())
                        return {};
                    return self.mStore.at(LuaUtil::fromLuaIndex(index))->mValue.getFloat();
                },
                [](const Store& self, std::string_view id) -> std::optional<float> {
                    const ESM::Global* global = self.mStore.search(ESM::RefId::deserializeText(id));
                    if (global == nullptr)
                        return {};
                    return global->mValue.getFloat();
                });
            storeT[sol::meta_function::new_index] = [](Store& self, std::string_view idString, const sol::object& obj) {
                ESM::RefId id = validateId(idString);
                if (obj == sol::nil)
                {
                    self.mStore.eraseStatic(id);
                    return;
                }
                float value = LuaUtil::cast<float>(obj);
                ESM::Global global;
                if (auto* found = self.mStore.search(id))
                    global = *found;
                else
                {
                    global.blank();
                    global.mId = id;
                }
                int32_t intV = static_cast<int32_t>(value);
                if (intV == value)
                {
                    global.mValue.setType(ESM::VT_Long);
                    global.mValue.setInteger(intV);
                }
                else
                {
                    global.mValue.setType(ESM::VT_Float);
                    global.mValue.setFloat(value);
                }
                self.mStore.insertStatic(global);
            };
            storeT[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();
            storeT[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();
            sol::table api(lua, sol::create);
            api["records"] = Store{ store };
            return LuaUtil::makeReadOnly(api);
        }

        template <class T>
        void addRecordStoreBindings(sol::state_view& lua, TableToRecord<T> parseRecord)
        {
            using Store = MutableStore<T>;
            sol::usertype<Store> storeT = lua.new_usertype<Store>(std::string(T::getRecordType()) + "sContentStore");
            storeT[sol::meta_function::length] = [](const Store& self) { return self.mStore.getSize(); };
            storeT[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();
            storeT[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();
            storeT[sol::meta_function::index] = sol::overload(
                [](const Store& self, size_t index) -> std::optional<MutableRecord<T>> {
                    if (index == 0 || index > self.mStore.getSize())
                        return {};
                    return MutableRecord<T>{ self.mStore, self.mStore.at(LuaUtil::fromLuaIndex(index))->mId };
                },
                [](const Store& self, std::string_view id) -> std::optional<MutableRecord<T>> {
                    const auto* record = self.mStore.search(ESM::RefId::deserializeText(id));
                    if (record == nullptr)
                        return {};
                    return MutableRecord<T>{ self.mStore, record->mId };
                });
            storeT[sol::meta_function::new_index] = sol::overload(
                [](Store& self, std::string_view idString, const MutableRecord<T>& otherRecord) {
                    ESM::RefId id = validateId(idString);
                    T record = otherRecord.find();
                    record.mId = id;
                    self.mStore.insertStatic(record);
                },
                [parseRecord](Store& self, std::string_view idString, sol::lua_table table) {
                    ESM::RefId id = validateId(idString);
                    T record = parseRecord(table);
                    record.mId = id;
                    self.mStore.insertStatic(record);
                },
                [](Store& self, std::string_view idString, const sol::nil_t&) {
                    self.mStore.eraseStatic(validateId(idString));
                });
        }

        sol::table initDoorBindings(sol::state_view& lua, MWWorld::Store<ESM::Door>& store)
        {
            addRecordStoreBindings<ESM::Door>(lua, &MWLua::tableToDoor);
            addMutableDoorType(lua);
            sol::table api(lua, sol::create);
            api["records"] = MutableStore<ESM::Door>{ store };
            return LuaUtil::makeReadOnly(api);
        }

        sol::table initStaticBindings(sol::state_view& lua, MWWorld::Store<ESM::Static>& store)
        {
            addRecordStoreBindings<ESM::Static>(lua, &MWLua::tableToStatic);
            addMutableStaticType(lua);
            sol::table api(lua, sol::create);
            api["records"] = MutableStore<ESM::Static>{ store };
            return LuaUtil::makeReadOnly(api);
        }
    }

    sol::table initContentPackage(const Context& context)
    {
        auto lua = context.sol();
        sol::table api(lua, sol::create);
        MWWorld::ESMStore& esmStore = *MWBase::Environment::get().getESMStore();
        api["doors"] = initDoorBindings(lua, esmStore.getWritable<ESM::Door>());
        api["globals"] = initGlobalVariableBindings(lua, esmStore.getWritable<ESM::Global>());
        api["statics"] = initStaticBindings(lua, esmStore.getWritable<ESM::Static>());
        return LuaUtil::makeReadOnly(api);
    }
}
