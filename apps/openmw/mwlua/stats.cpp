#include "stats.hpp"

#include <algorithm>
#include <memory>
#include <optional>
#include <string_view>
#include <variant>

#include <components/esm3/loadclas.hpp>
#include <components/lua/luastate.hpp>

#include "context.hpp"
#include "localscripts.hpp"
#include "luamanagerimp.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "objectvariant.hpp"

namespace
{
    using SelfObject = MWLua::SelfObject;
    using ObjectVariant = MWLua::ObjectVariant;

    template <class T>
    auto addIndexedAccessor(int index)
    {
        return [index](const sol::object& o) { return T::create(ObjectVariant(o), index); };
    }

    template <class T, class G>
    void addProp(const MWLua::Context& context, sol::usertype<T>& type, std::string_view prop, G getter)
    {
        type[prop] = sol::property([=](const T& stat) { return stat.get(context, prop, getter); },
            [=](const T& stat, const sol::object& value) { stat.cache(context, prop, value); });
    }

    template <class G>
    sol::object getValue(const MWLua::Context& context, const ObjectVariant& obj, SelfObject::CachedStat::Setter setter,
        int index, std::string_view prop, G getter)
    {
        if (obj.isSelfObject())
        {
            SelfObject* self = obj.asSelfObject();
            auto it = self->mStatsCache.find({ setter, index, prop });
            if (it != self->mStatsCache.end())
                return it->second;
        }
        return sol::make_object(context.mLua->sol(), getter(obj.ptr()));
    }
}

namespace MWLua
{
    namespace
    {
        class StatUpdateAction final : public LuaManager::Action
        {
            ObjectId mId;

        public:
            StatUpdateAction(LuaUtil::LuaState* state, ObjectId id)
                : Action(state)
                , mId(id)
            {
            }

            void apply() const override
            {
                LObject obj(mId);
                LocalScripts* scripts = obj.ptr().getRefData().getLuaScripts();
                if (scripts)
                    scripts->applyStatsCache();
            }

            std::string toString() const override { return "StatUpdateAction"; }
        };
    }

    class LevelStat
    {
        ObjectVariant mObject;

        LevelStat(ObjectVariant object)
            : mObject(std::move(object))
        {
        }

    public:
        sol::object getCurrent(const Context& context) const
        {
            return getValue(context, mObject, &LevelStat::setValue, 0, "current",
                [](const MWWorld::Ptr& ptr) { return ptr.getClass().getCreatureStats(ptr).getLevel(); });
        }

        void setCurrent(const Context& context, const sol::object& value) const
        {
            SelfObject* obj = mObject.asSelfObject();
            if (obj->mStatsCache.empty())
                context.mLuaManager->addAction(std::make_unique<StatUpdateAction>(context.mLua, obj->id()));
            obj->mStatsCache[SelfObject::CachedStat{ &LevelStat::setValue, 0, "current" }] = value;
        }

        sol::object getProgress(const Context& context) const
        {
            const auto& ptr = mObject.ptr();
            if (!ptr.getClass().isNpc())
                return sol::nil;
            return sol::make_object(context.mLua->sol(), ptr.getClass().getNpcStats(ptr).getLevelProgress());
        }

        static std::optional<LevelStat> create(ObjectVariant object, int index)
        {
            if (!object.ptr().getClass().isActor())
                return {};
            return LevelStat{ std::move(object) };
        }

        static void setValue(int, std::string_view prop, const MWWorld::Ptr& ptr, const sol::object& value)
        {
            auto& stats = ptr.getClass().getCreatureStats(ptr);
            if (prop == "current")
                stats.setLevel(value.as<int>());
        }
    };

    class DynamicStat
    {
        ObjectVariant mObject;
        int mIndex;

        DynamicStat(ObjectVariant object, int index)
            : mObject(std::move(object))
            , mIndex(index)
        {
        }

    public:
        template <class G>
        sol::object get(const Context& context, std::string_view prop, G getter) const
        {
            return getValue(
                context, mObject, &DynamicStat::setValue, mIndex, prop, [this, getter](const MWWorld::Ptr& ptr) {
                    return (ptr.getClass().getCreatureStats(ptr).getDynamic(mIndex).*getter)();
                });
        }

        static std::optional<DynamicStat> create(ObjectVariant object, int index)
        {
            if (!object.ptr().getClass().isActor())
                return {};
            return DynamicStat{ std::move(object), index };
        }

        void cache(const Context& context, std::string_view prop, const sol::object& value) const
        {
            SelfObject* obj = mObject.asSelfObject();
            if (obj->mStatsCache.empty())
                context.mLuaManager->addAction(std::make_unique<StatUpdateAction>(context.mLua, obj->id()));
            obj->mStatsCache[SelfObject::CachedStat{ &DynamicStat::setValue, mIndex, prop }] = value;
        }

        static void setValue(int index, std::string_view prop, const MWWorld::Ptr& ptr, const sol::object& value)
        {
            auto& stats = ptr.getClass().getCreatureStats(ptr);
            auto stat = stats.getDynamic(index);
            float floatValue = value.as<float>();
            if (prop == "base")
                stat.setBase(floatValue);
            else if (prop == "current")
                stat.setCurrent(floatValue, true, true);
            else if (prop == "modifier")
                stat.setModifier(floatValue);
            stats.setDynamic(index, stat);
        }
    };

    class AttributeStat
    {
        ObjectVariant mObject;
        int mIndex;

        AttributeStat(ObjectVariant object, int index)
            : mObject(std::move(object))
            , mIndex(index)
        {
        }

    public:
        template <class G>
        sol::object get(const Context& context, std::string_view prop, G getter) const
        {
            return getValue(
                context, mObject, &AttributeStat::setValue, mIndex, prop, [this, getter](const MWWorld::Ptr& ptr) {
                    return (ptr.getClass().getCreatureStats(ptr).getAttribute(mIndex).*getter)();
                });
        }

        float getModified(const Context& context) const
        {
            auto base = get(context, "base", &MWMechanics::AttributeValue::getBase).as<float>();
            auto damage = get(context, "damage", &MWMechanics::AttributeValue::getDamage).as<float>();
            auto modifier = get(context, "modifier", &MWMechanics::AttributeValue::getModifier).as<float>();
            return std::max(0.f, base - damage + modifier); // Should match AttributeValue::getModified
        }

        static std::optional<AttributeStat> create(ObjectVariant object, int index)
        {
            if (!object.ptr().getClass().isActor())
                return {};
            return AttributeStat{ std::move(object), index };
        }

        void cache(const Context& context, std::string_view prop, const sol::object& value) const
        {
            SelfObject* obj = mObject.asSelfObject();
            if (obj->mStatsCache.empty())
                context.mLuaManager->addAction(std::make_unique<StatUpdateAction>(context.mLua, obj->id()));
            obj->mStatsCache[SelfObject::CachedStat{ &AttributeStat::setValue, mIndex, prop }] = value;
        }

        static void setValue(int index, std::string_view prop, const MWWorld::Ptr& ptr, const sol::object& value)
        {
            auto& stats = ptr.getClass().getCreatureStats(ptr);
            auto stat = stats.getAttribute(index);
            float floatValue = value.as<float>();
            if (prop == "base")
                stat.setBase(floatValue);
            else if (prop == "damage")
            {
                stat.restore(stat.getDamage());
                stat.damage(floatValue);
            }
            else if (prop == "modifier")
                stat.setModifier(floatValue);
            stats.setAttribute(index, stat);
        }
    };

    class SkillStat
    {
        ObjectVariant mObject;
        int mIndex;

        SkillStat(ObjectVariant object, int index)
            : mObject(std::move(object))
            , mIndex(index)
        {
        }

        static float getProgress(const MWWorld::Ptr& ptr, int index, const MWMechanics::SkillValue& stat)
        {
            float progress = stat.getProgress();
            if (progress != 0.f)
                progress /= getMaxProgress(ptr, index, stat);
            return progress;
        }

        static float getMaxProgress(const MWWorld::Ptr& ptr, int index, const MWMechanics::SkillValue& stat)
        {
            const auto& store = MWBase::Environment::get().getWorld()->getStore();
            const auto cl = store.get<ESM::Class>().find(ptr.get<ESM::NPC>()->mBase->mClass);
            return ptr.getClass().getNpcStats(ptr).getSkillProgressRequirement(index, *cl);
        }

    public:
        template <class G>
        sol::object get(const Context& context, std::string_view prop, G getter) const
        {
            return getValue(
                context, mObject, &SkillStat::setValue, mIndex, prop, [this, getter](const MWWorld::Ptr& ptr) {
                    return (ptr.getClass().getNpcStats(ptr).getSkill(mIndex).*getter)();
                });
        }

        float getModified(const Context& context) const
        {
            auto base = get(context, "base", &MWMechanics::SkillValue::getBase).as<float>();
            auto damage = get(context, "damage", &MWMechanics::SkillValue::getDamage).as<float>();
            auto modifier = get(context, "modifier", &MWMechanics::SkillValue::getModifier).as<float>();
            return std::max(0.f, base - damage + modifier); // Should match SkillValue::getModified
        }

        sol::object getProgress(const Context& context) const
        {
            return getValue(
                context, mObject, &SkillStat::setValue, mIndex, "progress", [this](const MWWorld::Ptr& ptr) {
                    return getProgress(ptr, mIndex, ptr.getClass().getNpcStats(ptr).getSkill(mIndex));
                });
        }

        static std::optional<SkillStat> create(ObjectVariant object, int index)
        {
            if (!object.ptr().getClass().isNpc())
                return {};
            return SkillStat{ std::move(object), index };
        }

        void cache(const Context& context, std::string_view prop, const sol::object& value) const
        {
            SelfObject* obj = mObject.asSelfObject();
            if (obj->mStatsCache.empty())
                context.mLuaManager->addAction(std::make_unique<StatUpdateAction>(context.mLua, obj->id()));
            obj->mStatsCache[SelfObject::CachedStat{ &SkillStat::setValue, mIndex, prop }] = value;
        }

        static void setValue(int index, std::string_view prop, const MWWorld::Ptr& ptr, const sol::object& value)
        {
            auto& stats = ptr.getClass().getNpcStats(ptr);
            auto stat = stats.getSkill(index);
            float floatValue = value.as<float>();
            if (prop == "base")
                stat.setBase(floatValue);
            else if (prop == "damage")
            {
                stat.restore(stat.getDamage());
                stat.damage(floatValue);
            }
            else if (prop == "modifier")
                stat.setModifier(floatValue);
            else if (prop == "progress")
                stat.setProgress(floatValue * getMaxProgress(ptr, index, stat));
            stats.setSkill(index, stat);
        }
    };
}

namespace sol
{
    template <>
    struct is_automagical<MWLua::LevelStat> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWLua::DynamicStat> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWLua::AttributeStat> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWLua::SkillStat> : std::false_type
    {
    };
}

namespace MWLua
{
    void addActorStatsBindings(sol::table& actor, const Context& context)
    {
        sol::table stats(context.mLua->sol(), sol::create);
        actor["stats"] = LuaUtil::makeReadOnly(stats);

        auto levelStatT = context.mLua->sol().new_usertype<LevelStat>("LevelStat");
        levelStatT["current"] = sol::property([context](const LevelStat& stat) { return stat.getCurrent(context); },
            [context](const LevelStat& stat, const sol::object& value) { stat.setCurrent(context, value); });
        levelStatT["progress"] = sol::property([context](const LevelStat& stat) { return stat.getProgress(context); });
        stats["level"] = addIndexedAccessor<LevelStat>(0);

        auto dynamicStatT = context.mLua->sol().new_usertype<DynamicStat>("DynamicStat");
        addProp(context, dynamicStatT, "base", &MWMechanics::DynamicStat<float>::getBase);
        addProp(context, dynamicStatT, "current", &MWMechanics::DynamicStat<float>::getCurrent);
        addProp(context, dynamicStatT, "modifier", &MWMechanics::DynamicStat<float>::getModifier);
        sol::table dynamic(context.mLua->sol(), sol::create);
        stats["dynamic"] = LuaUtil::makeReadOnly(dynamic);
        dynamic["health"] = addIndexedAccessor<DynamicStat>(0);
        dynamic["magicka"] = addIndexedAccessor<DynamicStat>(1);
        dynamic["fatigue"] = addIndexedAccessor<DynamicStat>(2);

        auto attributeStatT = context.mLua->sol().new_usertype<AttributeStat>("AttributeStat");
        addProp(context, attributeStatT, "base", &MWMechanics::AttributeValue::getBase);
        addProp(context, attributeStatT, "damage", &MWMechanics::AttributeValue::getDamage);
        attributeStatT["modified"]
            = sol::property([=](const AttributeStat& stat) { return stat.getModified(context); });
        addProp(context, attributeStatT, "modifier", &MWMechanics::AttributeValue::getModifier);
        sol::table attributes(context.mLua->sol(), sol::create);
        stats["attributes"] = LuaUtil::makeReadOnly(attributes);
        for (int id = ESM::Attribute::Strength; id < ESM::Attribute::Length; ++id)
            attributes[Misc::StringUtils::lowerCase(ESM::Attribute::sAttributeNames[id])]
                = addIndexedAccessor<AttributeStat>(id);
    }

    void addNpcStatsBindings(sol::table& npc, const Context& context)
    {
        sol::table npcStats(context.mLua->sol(), sol::create);
        sol::table baseMeta(context.mLua->sol(), sol::create);
        baseMeta[sol::meta_function::index] = LuaUtil::getMutableFromReadOnly(npc["baseType"]["stats"]);
        npcStats[sol::metatable_key] = baseMeta;
        npc["stats"] = LuaUtil::makeReadOnly(npcStats);

        auto skillStatT = context.mLua->sol().new_usertype<SkillStat>("SkillStat");
        addProp(context, skillStatT, "base", &MWMechanics::SkillValue::getBase);
        addProp(context, skillStatT, "damage", &MWMechanics::SkillValue::getDamage);
        skillStatT["modified"] = sol::property([=](const SkillStat& stat) { return stat.getModified(context); });
        addProp(context, skillStatT, "modifier", &MWMechanics::SkillValue::getModifier);
        skillStatT["progress"] = sol::property([context](const SkillStat& stat) { return stat.getProgress(context); },
            [context](const SkillStat& stat, const sol::object& value) { stat.cache(context, "progress", value); });
        sol::table skills(context.mLua->sol(), sol::create);
        npcStats["skills"] = LuaUtil::makeReadOnly(skills);
        for (int id = ESM::Skill::Block; id < ESM::Skill::Length; ++id)
            skills[Misc::StringUtils::lowerCase(ESM::Skill::sSkillNames[id])] = addIndexedAccessor<SkillStat>(id);
    }
}
