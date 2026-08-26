#include "types.hpp"

#include "usertypeutil.hpp"

#include <components/esm3/loadappa.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/util.hpp>
#include <components/misc/resourcehelpers.hpp>

namespace sol
{
    template <>
    struct is_automagical<ESM::Apparatus> : std::false_type
    {
    };
}

namespace MWLua
{
    namespace
    {
        void validateApparatusType(int value)
        {
            if (value < ESM::Apparatus::MortarPestle || value > ESM::Apparatus::Retort)
                throw std::runtime_error("Invalid apparatus type");
        }

        template <class T>
        void addTypeProperty(sol::usertype<T>& record)
        {
            const auto getter = [](const T& rec) -> int { return Types::RecordType<T>::asRecord(rec).mData.mType; };
            if constexpr (Types::RecordType<T>::isMutable)
            {
                record["type"] = sol::property(std::move(getter), [](T& rec, int value) {
                    validateApparatusType(value);
                    rec.find().mData.mType = value;
                });
            }
            else
                record["type"] = sol::readonly_property(std::move(getter));
        }

        template <class T>
        void addUserType(sol::state_view& lua, std::string_view name)
        {
            sol::usertype<T> record = lua.new_usertype<T>(name);
            record[sol::meta_function::to_string]
                = [](const T& rec) -> std::string { return "ESM3_Apparatus[" + rec.mId.toDebugString() + "]"; };
            record["id"] = sol::readonly_property([](const T& rec) -> ESM::RefId { return rec.mId; });

            Types::addProperty(record, "name", &ESM::Apparatus::mName);
            Types::addModelProperty(record);
            Types::addIconProperty(record);
            Types::addProperty(record, "mwscript", &ESM::Apparatus::mScript);
            addTypeProperty(record);
            Types::addProperty(record, "quality", &ESM::Apparatus::mData, &ESM::Apparatus::AADTstruct::mQuality);
            Types::addProperty(record, "weight", &ESM::Apparatus::mData, &ESM::Apparatus::AADTstruct::mWeight);
            Types::addProperty(record, "value", &ESM::Apparatus::mData, &ESM::Apparatus::AADTstruct::mValue);
        }
    }

    sol::table makeApparatusTypeTable(sol::state_view lua)
    {
        return LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string_view, int>(lua,
            {
                { "MortarPestle", ESM::Apparatus::MortarPestle },
                { "Alembic", ESM::Apparatus::Alembic },
                { "Calcinator", ESM::Apparatus::Calcinator },
                { "Retort", ESM::Apparatus::Retort },
            }));
    }

    ESM::Apparatus tableToApparatus(const sol::table& rec)
    {
        auto apparatus = Types::initFromTemplate<ESM::Apparatus>(rec);
        if (rec["name"] != sol::nil)
            apparatus.mName = rec["name"];
        if (rec["model"] != sol::nil)
            apparatus.mModel = Misc::ResourceHelpers::meshPathForESM3(rec["model"].get<std::string_view>());
        if (rec["icon"] != sol::nil)
            apparatus.mIcon = rec["icon"].get<std::string_view>();
        if (rec["mwscript"] != sol::nil)
            apparatus.mScript = ESM::RefId::deserializeText(rec["mwscript"].get<std::string_view>());
        if (rec["type"] != sol::nil)
            apparatus.mData.mType = rec["type"];
        validateApparatusType(apparatus.mData.mType);
        if (rec["quality"] != sol::nil)
            apparatus.mData.mQuality = rec["quality"].get<Misc::FiniteFloat>();
        if (rec["weight"] != sol::nil)
            apparatus.mData.mWeight = rec["weight"].get<Misc::FiniteFloat>();
        if (rec["value"] != sol::nil)
            apparatus.mData.mValue = rec["value"];
        return apparatus;
    }

    void addMutableApparatusType(sol::state_view& lua)
    {
        addUserType<MutableRecord<ESM::Apparatus>>(lua, "ESM3_MutableApparatus");
    }

    void addApparatusBindings(sol::table apparatus, const Context& context)
    {
        addRecordFunctionBinding<ESM::Apparatus>(apparatus, context);
        sol::state_view lua = context.sol();
        apparatus["TYPE"] = makeApparatusTypeTable(lua);
        addUserType<ESM::Apparatus>(lua, "ESM3_Apparatus");
    }
}
