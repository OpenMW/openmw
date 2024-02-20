#include "itemdata.hpp"

#include "context.hpp"

#include "luamanagerimp.hpp"

#include "../mwworld/class.hpp"

#include "objectvariant.hpp"

namespace
{
    using SelfObject = MWLua::SelfObject;
    using Index = const SelfObject::CachedStat::Index&;

    constexpr std::array properties = { "condition", /*"enchantmentCharge", "soul", "owner", etc..*/ };

    void invalidPropErr(std::string_view prop, const MWWorld::Ptr& ptr)
    {
        throw std::runtime_error("'" + std::string(prop) + "'" + " property does not exist for item "
            + std::string(ptr.getClass().getName(ptr)) + "(" + std::string(ptr.getTypeDescription()) + ")");
    }
}

namespace MWLua
{
    static void addStatUpdateAction(MWLua::LuaManager* manager, const SelfObject& obj)
    {
        if (!obj.mStatsCache.empty())
            return; // was already added before
        manager->addAction(
            [obj = Object(obj)] {
                LocalScripts* scripts = obj.ptr().getRefData().getLuaScripts();
                if (scripts)
                    scripts->applyStatsCache();
            },
            "StatUpdateAction");
    }

    class ItemData
    {
        ObjectVariant mObject;

    public:
        ItemData(const ObjectVariant& object)
            : mObject(object)
        {
        }

        sol::object get(const Context& context, std::string_view prop) const
        {
            if (mObject.isSelfObject())
            {
                SelfObject* self = mObject.asSelfObject();
                auto it = self->mStatsCache.find({ &ItemData::setValue, std::monostate{}, prop });
                if (it != self->mStatsCache.end())
                    return it->second;
            }
            return sol::make_object(context.mLua->sol(), getValue(context, prop));
        }

        void set(const Context& context, std::string_view prop, const sol::object& value) const
        {
            SelfObject* obj = mObject.asSelfObject();
            addStatUpdateAction(context.mLuaManager, *obj);
            obj->mStatsCache[SelfObject::CachedStat{ &ItemData::setValue, std::monostate{}, prop }] = value;
        }

        sol::object getValue(const Context& context, std::string_view prop) const
        {
            if (prop == "condition")
            {
                MWWorld::Ptr o = mObject.ptr();
                if (o.mRef->getType() == ESM::REC_LIGH)
                    return sol::make_object(context.mLua->sol(), o.getClass().getRemainingUsageTime(o));
                else if (o.getClass().hasItemHealth(o))
                    return sol::make_object(
                        context.mLua->sol(), o.getClass().getItemHealth(o) + o.getCellRef().getChargeIntRemainder());
            }

            return sol::lua_nil;
        }

        static void setValue(Index i, std::string_view prop, const MWWorld::Ptr& ptr, const sol::object& value)
        {
            if (prop == "condition")
            {
                float cond = LuaUtil::cast<float>(value);
                if (ptr.mRef->getType() == ESM::REC_LIGH)
                    ptr.getClass().setRemainingUsageTime(ptr, cond);
                else if (ptr.getClass().hasItemHealth(ptr))
                {
                    // if the value set is less than 0, chargeInt and chargeIntRemainder is set to 0
                    ptr.getCellRef().setChargeIntRemainder(std::max(0.f, std::modf(cond, &cond)));
                    ptr.getCellRef().setCharge(std::max(0.f, cond));
                }
                else
                    invalidPropErr(prop, ptr);
            }
        }
    };
}

namespace sol
{
    template <>
    struct is_automagical<MWLua::ItemData> : std::false_type
    {
    };
}

namespace MWLua
{
    void addItemDataBindings(sol::table& item, const Context& context)
    {
        item["itemData"] = [](const sol::object& object) -> sol::optional<ItemData> {
            ObjectVariant o(object);
            if (o.ptr().getClass().isItem(o.ptr()) || o.ptr().mRef->getType() == ESM::REC_LIGH)
                return ItemData(o);
            return {};
        };

        sol::usertype<ItemData> itemData = context.mLua->sol().new_usertype<ItemData>("ItemData");
        itemData[sol::meta_function::new_index] = [](const ItemData& stat, const sol::variadic_args args) {
            throw std::runtime_error("Unknown ItemData property '" + args.get<std::string>() + "'");
        };

        for (std::string_view prop : properties)
        {
            itemData[prop] = sol::property([context, prop](const ItemData& stat) { return stat.get(context, prop); },
                [context, prop](const ItemData& stat, const sol::object& value) { stat.set(context, prop, value); });
        }
    }
}
