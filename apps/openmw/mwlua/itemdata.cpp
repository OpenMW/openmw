#include "itemdata.hpp"

#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadench.hpp>

#include "context.hpp"
#include "luamanagerimp.hpp"
#include "objectvariant.hpp"

#include "../mwbase/environment.hpp"
#include "../mwmechanics/spellutil.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

namespace
{
    using SelfObject = MWLua::SelfObject;
    using Index = const SelfObject::CachedStat::Index&;

    constexpr std::array properties = { "condition", "enchantmentCharge", "soul" };

    void valueErr(std::string_view prop, std::string type)
    {
        throw std::logic_error("'" + std::string(prop) + "'" + " received invalid value type (" + type + ")");
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
            return sol::make_object(context.mLua->sol(), getValue(context, prop, mObject.ptr()));
        }

        void set(const Context& context, std::string_view prop, const sol::object& value) const
        {
            if (mObject.isGObject())
                setValue({}, prop, mObject.ptr(), value);
            else if (mObject.isSelfObject())
            {
                SelfObject* obj = mObject.asSelfObject();
                addStatUpdateAction(context.mLuaManager, *obj);
                obj->mStatsCache[SelfObject::CachedStat{ &ItemData::setValue, std::monostate{}, prop }] = value;
            }
            else
                throw std::runtime_error("Only global or self scripts can set the value");
        }

        static sol::object getValue(const Context& context, std::string_view prop, const MWWorld::Ptr& ptr)
        {
            if (prop == "condition")
            {
                if (ptr.mRef->getType() == ESM::REC_LIGH)
                    return sol::make_object(context.mLua->sol(), ptr.getClass().getRemainingUsageTime(ptr));
                else if (ptr.getClass().hasItemHealth(ptr))
                    return sol::make_object(context.mLua->sol(),
                        ptr.getClass().getItemHealth(ptr) + ptr.getCellRef().getChargeIntRemainder());
            }
            else if (prop == "enchantmentCharge")
            {
                const ESM::RefId& enchantmentName = ptr.getClass().getEnchantment(ptr);

                if (enchantmentName.empty())
                    return sol::lua_nil;

                float charge = ptr.getCellRef().getEnchantmentCharge();
                const auto& store = MWBase::Environment::get().getESMStore();
                const auto* enchantment = store->get<ESM::Enchantment>().find(enchantmentName);

                if (charge == -1) // return the full charge
                    return sol::make_object(context.mLua->sol(), MWMechanics::getEnchantmentCharge(*enchantment));

                return sol::make_object(context.mLua->sol(), charge);
            }
            else if (prop == "soul")
            {
                ESM::RefId soul = ptr.getCellRef().getSoul();
                if (soul.empty())
                    return sol::lua_nil;

                return sol::make_object(context.mLua->sol(), soul.serializeText());
            }

            return sol::lua_nil;
        }

        static void setValue(Index i, std::string_view prop, const MWWorld::Ptr& ptr, const sol::object& value)
        {
            if (prop == "condition")
            {
                if (value.get_type() == sol::type::number)
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
                }
                else
                    valueErr(prop, sol::type_name(value.lua_state(), value.get_type()));
            }
            else if (prop == "enchantmentCharge")
            {
                if (value.get_type() == sol::type::lua_nil)
                    ptr.getCellRef().setEnchantmentCharge(-1);
                else if (value.get_type() == sol::type::number)
                    ptr.getCellRef().setEnchantmentCharge(std::max(0.0f, LuaUtil::cast<float>(value)));
                else
                    valueErr(prop, sol::type_name(value.lua_state(), value.get_type()));
            }
            else if (prop == "soul")
            {
                if (value.get_type() == sol::type::lua_nil)
                    ptr.getCellRef().setSoul(ESM::RefId{});
                else if (value.get_type() == sol::type::string)
                {
                    std::string_view souldId = LuaUtil::cast<std::string_view>(value);
                    ESM::RefId creature = ESM::RefId::deserializeText(souldId);
                    const auto& store = *MWBase::Environment::get().getESMStore();

                    // TODO: Add Support for NPC Souls
                    if (store.get<ESM::Creature>().search(creature))
                        ptr.getCellRef().setSoul(creature);
                    else
                        throw std::runtime_error("Cannot use non-existent creature as a soul: " + std::string(souldId));
                }
                else
                    valueErr(prop, sol::type_name(value.lua_state(), value.get_type()));
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
