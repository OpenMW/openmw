#include "types.hpp"

#include "modelproperty.hpp"

#include <components/esm3/loadbody.hpp>
#include <components/lua/luastate.hpp>

namespace sol
{
    template <>
    struct is_automagical<ESM::BodyPart> : std::false_type
    {
    };
}

namespace MWLua
{
    void addBodyPartBindings(sol::table bodypart, const Context& context)
    {
        addRecordFunctionBinding<ESM::BodyPart>(bodypart, context);

        sol::state_view lua = context.sol();
        sol::usertype<ESM::BodyPart> record = lua.new_usertype<ESM::BodyPart>("ESM3_BodyPart");
        record[sol::meta_function::to_string]
            = [](const ESM::BodyPart& rec) { return "ESM3_BodyPart[" + rec.mId.toDebugString() + "]"; };
        record["id"] = sol::readonly_property([](const ESM::BodyPart& rec) -> ESM::RefId { return rec.mId; });
        record["race"] = sol::readonly_property([](const ESM::BodyPart& rec) -> ESM::RefId { return rec.mRace; });
        addModelProperty(record);
        record["isFemale"] = sol::readonly_property(
            [](const ESM::BodyPart& rec) -> bool { return rec.mData.mFlags & ESM::BodyPart::BPF_Female; });
        record["isPlayable"] = sol::readonly_property(
            [](const ESM::BodyPart& rec) -> bool { return !(rec.mData.mFlags & ESM::BodyPart::BPF_NotPlayable); });
        record["isVampire"]
            = sol::readonly_property([](const ESM::BodyPart& rec) -> bool { return rec.mData.mVampire; });
        record["type"] = sol::readonly_property([](const ESM::BodyPart& rec) -> std::optional<std::string_view> {
            if (rec.mData.mType == ESM::BodyPart::MT_Skin)
                return "skin";
            else if (rec.mData.mType == ESM::BodyPart::MT_Clothing)
                return "clothing";
            else if (rec.mData.mType == ESM::BodyPart::MT_Armor)
                return "armor";
            return {};
        });
    }
}
