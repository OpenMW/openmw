#include "types.hpp"

#include <components/esm4/loadterm.hpp>
#include <components/lua/utilpackage.hpp>
#include <components/misc/convert.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

namespace sol
{
    template <>
    struct is_automagical<ESM4::Terminal> : std::false_type
    {
    };
}

namespace MWLua
{

    void addESM4TerminalBindings(sol::table term, const Context& context)
    {
        addRecordFunctionBinding<ESM4::Terminal>(term, context, "ESM4Terminal");

        sol::usertype<ESM4::Terminal> record = context.sol().new_usertype<ESM4::Terminal>("ESM4_Terminal");
        record[sol::meta_function::to_string] = [](const ESM4::Terminal& rec) -> std::string {
            return "ESM4_Terminal[" + ESM::RefId(rec.mId).toDebugString() + "]";
        };
        record["id"] = sol::readonly_property(
            [](const ESM4::Terminal& rec) -> std::string { return ESM::RefId(rec.mId).serializeText(); });
        record["editorId"]
            = sol::readonly_property([](const ESM4::Terminal& rec) -> std::string { return rec.mEditorId; });
        record["text"] = sol::readonly_property([](const ESM4::Terminal& rec) -> std::string { return rec.mText; });
        record["resultText"]
            = sol::readonly_property([](const ESM4::Terminal& rec) -> std::string { return rec.mResultText; });
        record["name"] = sol::readonly_property([](const ESM4::Terminal& rec) -> std::string { return rec.mFullName; });
        record["model"] = sol::readonly_property([](const ESM4::Terminal& rec) -> std::string {
            return Misc::ResourceHelpers::correctMeshPath(rec.mModel);
        });
    }
}
