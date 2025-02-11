#include "types.hpp"

#include <components/esm3/loadlevlist.hpp>
#include <components/lua/util.hpp>

#include "../../mwbase/environment.hpp"
#include "../../mwbase/world.hpp"
#include "../../mwmechanics/levelledlist.hpp"

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
}

namespace MWLua
{
    void addLevelledCreatureBindings(sol::table list, const Context& context)
    {
        auto state = context.sol();
        auto item = state.new_usertype<ESM::LevelledListBase::LevelItem>("ESM3_LevelledListItem");
        item["id"] = sol::readonly_property(
            [](const ESM::LevelledListBase::LevelItem& rec) -> std::string { return rec.mId.serializeText(); });
        item["level"]
            = sol::readonly_property([](const ESM::LevelledListBase::LevelItem& rec) -> int { return rec.mLevel; });
        item[sol::meta_function::to_string] = [](const ESM::LevelledListBase::LevelItem& rec) -> std::string {
            return "ESM3_LevelledListItem[" + rec.mId.toDebugString() + ", " + std::to_string(rec.mLevel) + "]";
        };

        addRecordFunctionBinding<ESM::CreatureLevList>(list, context);

        auto record = state.new_usertype<ESM::CreatureLevList>("ESM3_CreatureLevelledList");
        record[sol::meta_function::to_string] = [](const ESM::CreatureLevList& rec) -> std::string {
            return "ESM3_CreatureLevelledList[" + rec.mId.toDebugString() + "]";
        };
        record["id"] = sol::readonly_property(
            [](const ESM::CreatureLevList& rec) -> std::string { return rec.mId.serializeText(); });
        record["chanceNone"] = sol::readonly_property(
            [](const ESM::CreatureLevList& rec) -> float { return std::clamp(rec.mChanceNone / 100.f, 0.f, 1.f); });
        record["creatures"] = sol::readonly_property([state](const ESM::CreatureLevList& rec) -> sol::table {
            sol::table res(state, sol::create);
            for (size_t i = 0; i < rec.mList.size(); ++i)
                res[LuaUtil::toLuaIndex(i)] = rec.mList[i];
            return res;
        });
        record["calculateFromAllLevels"] = sol::readonly_property(
            [](const ESM::CreatureLevList& rec) -> bool { return rec.mFlags & ESM::CreatureLevList::AllLevels; });

        record["getRandomId"] = [](const ESM::CreatureLevList& rec, int level) -> std::string {
            auto& prng = MWBase::Environment::get().getWorld()->getPrng();
            return MWMechanics::getLevelledItem(&rec, true, prng, level).serializeText();
        };
    }
}
