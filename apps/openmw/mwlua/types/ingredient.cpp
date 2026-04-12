#include "types.hpp"

#include "usertypeutil.hpp"

#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadingr.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/util.hpp>
#include <components/misc/resourcehelpers.hpp>

namespace
{
    struct MutableIngredientEffectsList
    {
        MWLua::MutableRecord<ESM::Ingredient> mIngredient;
    };
    struct MutableIngredientEffectsListItem
    {
        MWLua::MutableRecord<ESM::Ingredient> mIngredient;
        uint32_t mIndex;
    };
}

namespace sol
{
    template <>
    struct is_automagical<ESM::Ingredient> : std::false_type
    {
    };
    template <>
    struct is_automagical<MutableIngredientEffectsList> : std::false_type
    {
    };
    template <>
    struct is_automagical<MutableIngredientEffectsListItem> : std::false_type
    {
    };
}

namespace MWLua
{
    namespace
    {
        constexpr uint32_t numEffects = 4;

        void blankIndex(ESM::Ingredient::IRDTstruct& data, uint32_t i)
        {
            data.mEffectID[i] = {};
            data.mAttributes[i] = {};
            data.mSkills[i] = {};
        }

        void addPropertyFromTable(const sol::lua_table& rec, std::string_view key, ESM::RefId& value)
        {
            if (rec[key] != sol::nil)
            {
                std::string_view id = rec[key].get<std::string_view>();
                value = ESM::RefId::deserializeText(id);
            }
        }

        void setFromTable(ESM::Ingredient::IRDTstruct& data, uint32_t i, const sol::lua_table& table)
        {
            addPropertyFromTable(table, "id", data.mEffectID[i]);
            addPropertyFromTable(table, "affectedAttribute", data.mAttributes[i]);
            addPropertyFromTable(table, "affectedSkill", data.mSkills[i]);
        }

        template <class T>
        void addUserType(sol::state_view& lua, std::string_view name)
        {
            sol::usertype<T> record = lua.new_usertype<T>(name);

            record[sol::meta_function::to_string]
                = [](const T& rec) -> std::string { return "ESM3_Ingredient[" + rec.mId.toDebugString() + "]"; };
            record["id"] = sol::readonly_property([](const T& rec) -> ESM::RefId { return rec.mId; });

            Types::addProperty(record, "name", &ESM::Ingredient::mName);
            Types::addModelProperty(record);
            Types::addProperty(record, "mwscript", &ESM::Ingredient::mScript);
            Types::addIconProperty(record);
            Types::addProperty(record, "weight", &ESM::Ingredient::mData, &ESM::Ingredient::IRDTstruct::mWeight);
            Types::addProperty(record, "value", &ESM::Ingredient::mData, &ESM::Ingredient::IRDTstruct::mValue);

            if constexpr (Types::RecordType<T>::isMutable)
            {
                record["effects"] = sol::property(
                    [](const MutableRecord<ESM::Ingredient>& rec) { return MutableIngredientEffectsList{ rec }; },
                    [](MutableRecord<ESM::Ingredient>& rec, const sol::object& value) {
                        auto& ingred = rec.find();
                        if (value == sol::nil)
                        {
                            for (uint32_t i = 0; i < numEffects; ++i)
                                blankIndex(ingred.mData, i);
                        }
                        else if (value.is<MutableIngredientEffectsList>())
                        {
                            const ESM::Ingredient& other = value.as<MutableIngredientEffectsList>().mIngredient.find();
                            for (uint32_t i = 0; i < numEffects; ++i)
                            {
                                ingred.mData.mEffectID[i] = other.mData.mEffectID[i];
                                ingred.mData.mAttributes[i] = other.mData.mAttributes[i];
                                ingred.mData.mSkills[i] = other.mData.mSkills[i];
                            }
                        }
                        else
                        {
                            const sol::table table = value;
                            const size_t length = table.size();
                            for (uint32_t i = 0; i < numEffects; ++i)
                            {
                                if (i >= length)
                                    blankIndex(ingred.mData, i);
                                else
                                    setFromTable(ingred.mData, i, table[LuaUtil::toLuaIndex(i)]);
                            }
                        }
                    });

                auto listType = lua.new_usertype<MutableIngredientEffectsList>("ESM3_MutableIngredientEffectsList");
                listType[sol::meta_function::length]
                    = [](const MutableIngredientEffectsList& list) { return numEffects; };
                listType[sol::meta_function::index]
                    = [](const MutableIngredientEffectsList& list,
                          uint32_t index) -> std::optional<MutableIngredientEffectsListItem> {
                    list.mIngredient.find();
                    if (index == 0 || index > numEffects)
                        return {};
                    return MutableIngredientEffectsListItem{ list.mIngredient, index - 1 };
                };
                listType[sol::meta_function::new_index]
                    = [](MutableIngredientEffectsList& list, uint32_t i, const sol::object& value) {
                          ESM::Ingredient& ingred = list.mIngredient.find();
                          if (i == 0 || i > numEffects)
                              throw std::runtime_error("index out of range");
                          --i;
                          if (value == sol::nil)
                              blankIndex(ingred.mData, i);
                          else
                              setFromTable(ingred.mData, i, value.as<sol::lua_table>());
                      };
                listType[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();
                listType[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();

                auto itemType
                    = lua.new_usertype<MutableIngredientEffectsListItem>("ESM3_MutableIngredientEffectsListItem");
                constexpr auto addRefIdProperty
                    = [](sol::usertype<MutableIngredientEffectsListItem>& type,
                          ESM::RefId(ESM::Ingredient::IRDTstruct::*member)[numEffects], std::string_view key) {
                          type[key] = sol::property(
                              [=](const MutableIngredientEffectsListItem& item) -> ESM::RefId {
                                  const ESM::Ingredient& ingred = item.mIngredient.find();
                                  auto& values = ingred.mData.*member;
                                  return values[item.mIndex];
                              },
                              [=](MutableIngredientEffectsListItem& item, std::optional<std::string_view> value) {
                                  ESM::Ingredient& ingred = item.mIngredient.find();
                                  auto& values = ingred.mData.*member;
                                  values[item.mIndex] = ESM::RefId::deserializeText(value.value_or(std::string_view()));
                              });
                      };
                addRefIdProperty(itemType, &ESM::Ingredient::IRDTstruct::mEffectID, "id");
                addRefIdProperty(itemType, &ESM::Ingredient::IRDTstruct::mAttributes, "affectedAttribute");
                addRefIdProperty(itemType, &ESM::Ingredient::IRDTstruct::mSkills, "affectedSkill");
            }
            else
            {
                record["effects"]
                    = sol::readonly_property([lua = lua.lua_state()](const ESM::Ingredient& rec) -> sol::table {
                          sol::table res(lua, sol::create);
                          for (uint32_t i = 0; i < 4; ++i)
                          {
                              if (rec.mData.mEffectID[i].empty())
                                  continue;
                              ESM::IndexedENAMstruct effect;
                              effect.mData.mEffectID = rec.mData.mEffectID[i];
                              effect.mData.mSkill = rec.mData.mSkills[i];
                              effect.mData.mAttribute = rec.mData.mAttributes[i];
                              effect.mData.mRange = ESM::RT_Self;
                              effect.mData.mArea = 0;
                              effect.mData.mDuration = 0;
                              effect.mData.mMagnMin = 0;
                              effect.mData.mMagnMax = 0;
                              effect.mIndex = i;
                              res[LuaUtil::toLuaIndex(i)] = effect;
                          }
                          return res;
                      });
            }
        }
    }

    ESM::Ingredient tableToIngredient(const sol::table& rec)
    {
        auto ingred = Types::initFromTemplate<ESM::Ingredient>(rec);
        if (rec["name"] != sol::nil)
            ingred.mName = rec["name"];
        if (rec["model"] != sol::nil)
            ingred.mModel = Misc::ResourceHelpers::meshPathForESM3(rec["model"].get<std::string_view>());
        if (rec["mwscript"] != sol::nil)
        {
            std::string_view scriptId = rec["mwscript"].get<std::string_view>();
            ingred.mScript = ESM::RefId::deserializeText(scriptId);
        }
        if (rec["icon"] != sol::nil)
            ingred.mIcon = rec["icon"];
        if (rec["weight"] != sol::nil)
            ingred.mData.mWeight = rec["weight"].get<Misc::FiniteFloat>();
        if (rec["value"] != sol::nil)
            ingred.mData.mValue = rec["value"];
        if (rec["effects"] != sol::nil)
        {
            const sol::table effects = rec["effects"];
            const size_t length = effects.size();
            for (uint32_t i = 0; i < numEffects; ++i)
            {
                if (i >= length)
                    blankIndex(ingred.mData, i);
                else
                    setFromTable(ingred.mData, i, effects[LuaUtil::toLuaIndex(i)]);
            }
        }
        return ingred;
    }

    void addMutableIngredientType(sol::state_view& lua)
    {
        addUserType<MutableRecord<ESM::Ingredient>>(lua, "ESM3_MutableIngredient");
    }

    void addIngredientBindings(sol::table ingredient, const Context& context)
    {
        addRecordFunctionBinding<ESM::Ingredient>(ingredient, context);
        sol::state_view lua = context.sol();
        addUserType<ESM::Ingredient>(lua, "ESM3_Ingredient");
    }
}
