#include "contentbindings.hpp"

#include <components/lua/util.hpp>

#include "context.hpp"

#include "../mwbase/environment.hpp"
#include "../mwworld/esmstore.hpp"

namespace
{
    template <class T>
    struct MutableStore
    {
        MWWorld::Store<T>& mStore;
    };

    ESM::RefId validateId(std::string_view value)
    {
        if (value.empty())
            throw std::runtime_error("ID cannot be empty");
        if (auto id = ESM::StringRefId::deserializeExisting(value))
            return *id;
        // Check if this ID would be interpreted as something other than a StringRefId
        ESM::RefId id = ESM::RefId::deserializeText(value);
        if (!id.empty())
            throw std::runtime_error("ID not allowed");
        return ESM::RefId::stringRefId(value);
    }
}

namespace sol
{
    template <class T>
    struct is_automagical<MutableStore<T>> : std::false_type
    {
    };
}

namespace MWLua
{
    namespace
    {
        MutableStore<ESM::Global> initGlobalBindings(sol::state_view& lua, MWWorld::Store<ESM::Global>& store)
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
            storeT[sol::meta_function::new_index] = [](Store& self, std::string_view idString, float value) {
                ESM::RefId id = validateId(idString);
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
            return Store{ store };
        }
    }

    sol::table initContentPackage(const Context& context)
    {
        auto lua = context.sol();
        sol::table api(lua, sol::create);
        MWWorld::ESMStore& esmStore = *MWBase::Environment::get().getESMStore();
        api["globals"] = initGlobalBindings(lua, esmStore.getWritable<ESM::Global>());
        return LuaUtil::makeReadOnly(api);
    }
}
