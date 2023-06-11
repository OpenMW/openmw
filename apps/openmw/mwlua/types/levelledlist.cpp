#include "types.hpp"

#include <components/esm3/loadlevlist.hpp>

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
        auto item = context.mLua->sol().new_usertype<ESM::LevelledListBase::LevelItem>("ESM3_LevelledListItem");
        item["id"] = sol::readonly_property(
            [](const ESM::LevelledListBase::LevelItem& rec) { return rec.mId.serializeText(); });
        item["level"] = sol::readonly_property([](const ESM::LevelledListBase::LevelItem& rec) { return rec.mLevel; });
        item[sol::meta_function::to_string] = [](const ESM::LevelledListBase::LevelItem& rec) {
            return "ESM3_LevelledListItem[" + rec.mId.toDebugString() + ", " + std::to_string(rec.mLevel) + "]";
        };

        addRecordFunctionBinding<ESM::CreatureLevList>(list, context);

        auto record = context.mLua->sol().new_usertype<ESM::CreatureLevList>("ESM3_CreatureLevelledList");
        record[sol::meta_function::to_string] = [](const ESM::CreatureLevList& rec) {
            return "ESM3_CreatureLevelledList[" + rec.mId.toDebugString() + "]";
        };
        record["id"] = sol::readonly_property([](const ESM::CreatureLevList& rec) { return rec.mId.serializeText(); });
        record["chanceNone"] = sol::readonly_property([](const ESM::CreatureLevList& rec) { return rec.mChanceNone; });
        record["creatures"] = sol::readonly_property([](const ESM::CreatureLevList& rec) { return rec.mList; });
        record["calculateFromAllLevels"] = sol::readonly_property(
            [](const ESM::CreatureLevList& rec) -> bool { return rec.mFlags & ESM::CreatureLevList::AllLevels; });

        record["getRandomId"] = [](const ESM::CreatureLevList& rec, int level) {
            auto& prng = MWBase::Environment::get().getWorld()->getPrng();
            return MWMechanics::getLevelledItem(&rec, true, prng, level).serializeText();
        };
    }
}
