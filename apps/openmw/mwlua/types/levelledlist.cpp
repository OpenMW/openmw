#include "types.hpp"

#include "usertypeutil.hpp"

#include <components/esm3/loadlevlist.hpp>
#include <components/lua/util.hpp>

#include "../../mwbase/environment.hpp"
#include "../../mwbase/world.hpp"
#include "../../mwmechanics/levelledlist.hpp"

#include <format>
#include <variant>

namespace
{
    struct MutableLevelItemList
    {
        std::variant<MWLua::MutableRecord<ESM::CreatureLevList>, MWLua::MutableRecord<ESM::ItemLevList>> mRecord;

        std::vector<ESM::LevelledListBase::LevelItem>& find()
        {
            return std::visit(
                [](auto& rec) -> auto& { return rec.find().mList; }, mRecord);
        }

        const std::vector<ESM::LevelledListBase::LevelItem>& find() const
        {
            return std::visit(
                [](const auto& rec) -> const auto& { return rec.find().mList; }, mRecord);
        }
    };

    struct MutableLevelItem
    {
        MutableLevelItemList mList;
        uint32_t mIndex;

        ESM::LevelledListBase::LevelItem& find() { return mList.find().at(mIndex); }

        const ESM::LevelledListBase::LevelItem& find() const { return mList.find().at(mIndex); }
    };
}

namespace sol
{
    template <>
    struct is_automagical<ESM::CreatureLevList> : std::false_type
    {
    };
    template <>
    struct is_automagical<ESM::LevelledListBase::LevelItem> : std::false_type
    {
    };
    template <>
    struct is_automagical<MutableLevelItemList> : std::false_type
    {
    };
    template <>
    struct is_automagical<MutableLevelItem> : std::false_type
    {
    };
}

namespace MWLua
{
    template <>
    struct Types::RecordType<MutableLevelItem>
    {
        using Record = ESM::LevelledListBase::LevelItem;
        constexpr static bool isMutable = true;

        static const Record& asRecord(const MutableLevelItem& rec) { return rec.find(); }
    };

    namespace
    {
        ESM::LevelledListBase::LevelItem tableToLevelItem(const sol::object& value)
        {
            if (value.is<MutableLevelItem>())
                return value.as<MutableLevelItem>().find();
            else if (value.is<ESM::LevelledListBase::LevelItem>())
                return value.as<ESM::LevelledListBase::LevelItem>();
            sol::lua_table table = value.as<sol::lua_table>();
            ESM::LevelledListBase::LevelItem out;
            out.mLevel = table.get_or("level", uint16_t{});
            if (table["id"] != sol::nil)
            {
                std::string_view id = table["id"].get<std::string_view>();
                out.mId = ESM::RefId::deserializeText(id);
            }
            return out;
        }

        std::vector<ESM::LevelledListBase::LevelItem> tableToLevelItemList(const sol::table& itemTable)
        {
            size_t numEffects = itemTable.size();
            std::vector<ESM::LevelledListBase::LevelItem> out;
            out.resize(numEffects);
            for (size_t i = 0; i < numEffects; ++i)
            {
                out[i] = tableToLevelItem(itemTable[LuaUtil::toLuaIndex(i)]);
            }
            return out;
        }

        template <class T>
        void addLevelItemBindings(sol::state_view& lua, std::string_view name)
        {
            auto item = lua.new_usertype<T>(name);
            if constexpr (!Types::RecordType<T>::isMutable)
            {
                item[sol::meta_function::to_string] = [](const ESM::LevelledListBase::LevelItem& rec) -> std::string {
                    return std::format("ESM3_LevelledListItem[{}, {}]", rec.mId.toDebugString(), rec.mLevel);
                };
            }
            Types::addProperty(item, "id", &ESM::LevelledListBase::LevelItem::mId);
            Types::addProperty(item, "level", &ESM::LevelledListBase::LevelItem::mLevel);
        }

        template <class T>
        void addLevListProps(sol::state_view& lua, sol::usertype<T>& record, std::string_view items)
        {
            using List = Types::RecordType<T>::Record;
            const auto getChanceNone = [](const T& rec) -> float {
                const List& list = Types::RecordType<T>::asRecord(rec);
                return std::clamp(list.mChanceNone / 100.f, 0.f, 1.f);
            };
            if constexpr (Types::RecordType<T>::isMutable)
            {
                record["chanceNone"] = sol::property(std::move(getChanceNone), [](T& rec, float chance) {
                    List& list = rec.find();
                    list.mChanceNone = static_cast<unsigned char>(std::clamp(chance, 0.f, 1.f) * 100.f);
                });

                record[items] = sol::property([](const T& rec) { return MutableLevelItemList{ rec }; },
                    [](T& rec, const sol::object& value) {
                        auto& list = rec.find();
                        if (value == sol::nil)
                            list.mList.clear();
                        else if (value.is<MutableLevelItemList>())
                            list.mList = value.as<MutableLevelItemList>().find();
                        else
                            list.mList = tableToLevelItemList(value);
                    });
            }
            else
            {
                record["chanceNone"] = sol::readonly_property(std::move(getChanceNone));

                record[items] = sol::readonly_property([lua = lua.lua_state()](const List& rec) -> sol::table {
                    sol::table res(lua, sol::create);
                    for (size_t i = 0; i < rec.mList.size(); ++i)
                        res[LuaUtil::toLuaIndex(i)] = rec.mList[i];
                    return res;
                });

                record["getRandomId"] = [](const List& rec, int level) -> ESM::RefId {
                    auto& prng = MWBase::Environment::get().getWorld()->getPrng();
                    return MWMechanics::getLevelledItem(&rec, true, prng, level);
                };
            }
        }

        template <class T>
        void addCreatureUserType(sol::state_view& lua, std::string_view name)
        {
            sol::usertype<T> record = lua.new_usertype<T>(name);

            record[sol::meta_function::to_string] = [](const T& rec) -> std::string {
                return std::format("ESM3_CreatureLevelledList[{}]", rec.mId.toDebugString());
            };
            record["id"] = sol::readonly_property([](const T& rec) -> ESM::RefId { return rec.mId; });

            Types::addFlagProperty(
                record, "calculateFromAllLevels", ESM::CreatureLevList::AllLevels, &ESM::CreatureLevList::mFlags);

            addLevListProps(lua, record, "creatures");
        }

        void setFlagProperty(const sol::table& rec, std::string_view key, int32_t& flags, int flag)
        {
            if (rec[key] != sol::nil)
            {
                if (rec[key])
                    flags |= flag;
                else
                    flags &= ~flag;
            }
        }

        void addLevelItemListBindings(sol::state_view& lua)
        {
            auto listType = lua.new_usertype<MutableLevelItemList>("ESM3_MutableLevelItemList");
            listType[sol::meta_function::length] = [](const MutableLevelItemList& list) { return list.find().size(); };
            listType[sol::meta_function::index]
                = [](const MutableLevelItemList& list, uint32_t index) -> std::optional<MutableLevelItem> {
                const auto& entries = list.find();
                if (index == 0 || index > entries.size())
                    return {};
                return MutableLevelItem{ list, index - 1 };
            };
            listType[sol::meta_function::new_index]
                = [](MutableLevelItemList& list, uint32_t i, const sol::object& value) {
                      auto& entries = list.find();
                      if (i == 0 || i > entries.size() + 1)
                          throw std::runtime_error("index out of range");
                      --i;
                      if (value == sol::nil)
                      {
                          if (i >= entries.size())
                              return;
                          entries.erase(entries.begin() + i);
                      }
                      else if (i == entries.size())
                          entries.emplace_back(tableToLevelItem(value));
                      else
                          entries[i] = tableToLevelItem(value);
                  };
            listType[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();
            listType[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();

            addLevelItemBindings<MutableLevelItem>(lua, "ESM3_MutableLevelledListItem");
        }

        template <class T>
        void addItemUserType(sol::state_view& lua, std::string_view name)
        {
            sol::usertype<T> record = lua.new_usertype<T>(name);

            record[sol::meta_function::to_string] = [](const T& rec) -> std::string {
                return std::format("ESM3_ItemLevelledList[{}]", rec.mId.toDebugString());
            };
            record["id"] = sol::readonly_property([](const T& rec) -> ESM::RefId { return rec.mId; });

            Types::addFlagProperty(
                record, "calculateFromAllLevels", ESM::ItemLevList::AllLevels, &ESM::ItemLevList::mFlags);
            Types::addFlagProperty(record, "calculateForEach", ESM::ItemLevList::Each, &ESM::ItemLevList::mFlags);

            addLevListProps(lua, record, "items");
        }
    }

    ESM::CreatureLevList tableToCreatureLevList(const sol::table& rec)
    {
        auto list = Types::initFromTemplate<ESM::CreatureLevList>(rec);
        if (rec["chanceNone"] != sol::nil)
        {
            float chance = rec["chanceNone"];
            list.mChanceNone = static_cast<unsigned char>(std::clamp(chance, 0.f, 1.f) * 100.f);
        }
        if (rec["creatures"] != sol::nil)
            list.mList = tableToLevelItemList(rec["creatures"]);
        setFlagProperty(rec, "calculateFromAllLevels", list.mFlags, ESM::CreatureLevList::AllLevels);
        return list;
    }

    void addLevelledCreatureBindings(sol::table list, const Context& context)
    {
        sol::state_view lua = context.sol();
        list["createRecordDraft"] = tableToCreatureLevList;
        addLevelItemBindings<ESM::LevelledListBase::LevelItem>(lua, "ESM3_LevelledListItem");
        addRecordFunctionBinding<ESM::CreatureLevList>(list, context);
        addCreatureUserType<ESM::CreatureLevList>(lua, "ESM3_CreatureLevelledList");
    }

    void addMutableCreatureLevListType(sol::state_view& lua)
    {
        addLevelItemListBindings(lua);
        addCreatureUserType<MutableRecord<ESM::CreatureLevList>>(lua, "ESM3_MutableCreatureLevelledList");
    }

    ESM::ItemLevList tableToItemLevList(const sol::table& rec)
    {
        auto list = Types::initFromTemplate<ESM::ItemLevList>(rec);
        if (rec["chanceNone"] != sol::nil)
        {
            float chance = rec["chanceNone"];
            list.mChanceNone = static_cast<unsigned char>(std::clamp(chance, 0.f, 1.f) * 100.f);
        }
        if (rec["items"] != sol::nil)
            list.mList = tableToLevelItemList(rec["items"]);
        setFlagProperty(rec, "calculateFromAllLevels", list.mFlags, ESM::ItemLevList::AllLevels);
        setFlagProperty(rec, "calculateForEach", list.mFlags, ESM::ItemLevList::Each);
        return list;
    }

    sol::table addLevelledItemBindings(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table lists(lua, sol::create);
        lists["createRecordDraft"] = tableToItemLevList;
        addRecordFunctionBinding<ESM::ItemLevList>(lists, context);
        addItemUserType<ESM::ItemLevList>(lua, "ESM3_ItemLevelledList");
        return LuaUtil::makeReadOnly(lists);
    }
}
