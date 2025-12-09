#include "stats.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include <components/esm3/loadclas.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "context.hpp"
#include "localscripts.hpp"
#include "luamanagerimp.hpp"

#include "../mwbase/environment.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "objectvariant.hpp"
#include "recordstore.hpp"

namespace
{
    using SelfObject = MWLua::SelfObject;
    using ObjectVariant = MWLua::ObjectVariant;
    using Index = const SelfObject::CachedStat::Index&;

    template <class T>
    auto addIndexedAccessor(auto index)
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
        Index index, std::string_view prop, G getter)
    {
        if (obj.isSelfObject())
        {
            SelfObject* self = obj.asSelfObject();
            auto it = self->mStatsCache.find({ setter, index, prop });
            if (it != self->mStatsCache.end())
                return it->second;
        }
        return sol::make_object(context.mLua->unsafeState(), getter(obj.ptr()));
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

    static void setCreatureValue(Index, std::string_view prop, const MWWorld::Ptr& ptr, const sol::object& value)
    {
        auto& stats = ptr.getClass().getCreatureStats(ptr);
        if (prop == "current")
            stats.setLevel(LuaUtil::cast<int>(value));
    }

    static void setNpcValue(Index index, std::string_view prop, const MWWorld::Ptr& ptr, const sol::object& value)
    {
        auto& stats = ptr.getClass().getNpcStats(ptr);
        if (prop == "progress")
            stats.setLevelProgress(LuaUtil::cast<int>(value));
        else if (prop == "skillIncreasesForAttribute")
            stats.setSkillIncreasesForAttribute(
                *std::get<ESM::RefId>(index).getIf<ESM::StringRefId>(), LuaUtil::cast<int>(value));
        else if (prop == "skillIncreasesForSpecialization")
            stats.setSkillIncreasesForSpecialization(
                static_cast<ESM::Class::Specialization>(std::get<int>(index)), LuaUtil::cast<int>(value));
    }

    class SkillIncreasesForAttributeStats
    {
        ObjectVariant mObject;

    public:
        SkillIncreasesForAttributeStats(ObjectVariant object)
            : mObject(std::move(object))
        {
        }

        sol::object get(const Context& context, ESM::StringRefId attributeId) const
        {
            if (!mObject.ptr().getClass().isNpc())
                return sol::nil;

            return getValue(context, mObject, &setNpcValue, attributeId, "skillIncreasesForAttribute",
                [attributeId](const MWWorld::Ptr& ptr) {
                    return ptr.getClass().getNpcStats(ptr).getSkillIncreasesForAttribute(attributeId);
                });
        }

        void set(const Context& context, ESM::StringRefId attributeId, const sol::object& value) const
        {
            const auto& ptr = mObject.ptr();
            if (!ptr.getClass().isNpc())
                return;

            SelfObject* obj = mObject.asSelfObject();
            addStatUpdateAction(context.mLuaManager, *obj);
            obj->mStatsCache[SelfObject::CachedStat{ &setNpcValue, attributeId, "skillIncreasesForAttribute" }]
                = sol::main_object(value);
        }
    };

    class SkillIncreasesForSpecializationStats
    {
        ObjectVariant mObject;

    public:
        SkillIncreasesForSpecializationStats(ObjectVariant object)
            : mObject(std::move(object))
        {
        }

        sol::object get(const Context& context, int specialization) const
        {
            if (!mObject.ptr().getClass().isNpc())
                return sol::nil;

            return getValue(context, mObject, &setNpcValue, specialization, "skillIncreasesForSpecialization",
                [specialization](const MWWorld::Ptr& ptr) {
                    return ptr.getClass().getNpcStats(ptr).getSkillIncreasesForSpecialization(
                        static_cast<ESM::Class::Specialization>(specialization));
                });
        }

        void set(const Context& context, int specialization, const sol::object& value) const
        {
            const auto& ptr = mObject.ptr();
            if (!ptr.getClass().isNpc())
                return;

            SelfObject* obj = mObject.asSelfObject();
            addStatUpdateAction(context.mLuaManager, *obj);
            obj->mStatsCache[SelfObject::CachedStat{ &setNpcValue, specialization, "skillIncreasesForSpecialization" }]
                = sol::main_object(value);
        }
    };

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
            return getValue(context, mObject, &setCreatureValue, std::monostate{}, "current",
                [](const MWWorld::Ptr& ptr) { return ptr.getClass().getCreatureStats(ptr).getLevel(); });
        }

        void setCurrent(const Context& context, const sol::object& value) const
        {
            SelfObject* obj = mObject.asSelfObject();
            addStatUpdateAction(context.mLuaManager, *obj);
            obj->mStatsCache[SelfObject::CachedStat{ &setCreatureValue, std::monostate{}, "current" }]
                = sol::main_object(value);
        }

        sol::object getProgress(const Context& context) const
        {
            if (!mObject.ptr().getClass().isNpc())
                return sol::nil;

            return getValue(context, mObject, &setNpcValue, std::monostate{}, "progress",
                [](const MWWorld::Ptr& ptr) { return ptr.getClass().getNpcStats(ptr).getLevelProgress(); });
        }

        void setProgress(const Context& context, const sol::object& value) const
        {
            const auto& ptr = mObject.ptr();
            if (!ptr.getClass().isNpc())
                return;

            SelfObject* obj = mObject.asSelfObject();
            addStatUpdateAction(context.mLuaManager, *obj);
            obj->mStatsCache[SelfObject::CachedStat{ &setNpcValue, std::monostate{}, "progress" }]
                = sol::main_object(value);
        }

        SkillIncreasesForAttributeStats getSkillIncreasesForAttributeStats() const
        {
            return SkillIncreasesForAttributeStats{ mObject };
        }

        SkillIncreasesForSpecializationStats getSkillIncreasesForSpecializationStats() const
        {
            return SkillIncreasesForSpecializationStats{ mObject };
        }

        static std::optional<LevelStat> create(ObjectVariant object, Index)
        {
            if (!object.ptr().getClass().isActor())
                return {};
            return LevelStat{ std::move(object) };
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

        static std::optional<DynamicStat> create(ObjectVariant object, Index i)
        {
            if (!object.ptr().getClass().isActor())
                return {};
            int index = std::get<int>(i);
            return DynamicStat{ std::move(object), index };
        }

        void cache(const Context& context, std::string_view prop, const sol::object& value) const
        {
            SelfObject* obj = mObject.asSelfObject();
            addStatUpdateAction(context.mLuaManager, *obj);
            obj->mStatsCache[SelfObject::CachedStat{ &DynamicStat::setValue, mIndex, prop }] = sol::main_object(value);
        }

        static void setValue(Index i, std::string_view prop, const MWWorld::Ptr& ptr, const sol::object& value)
        {
            int index = std::get<int>(i);
            auto& stats = ptr.getClass().getCreatureStats(ptr);
            auto stat = stats.getDynamic(index);
            float floatValue = LuaUtil::cast<float>(value);
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
        ESM::RefId mId;

        AttributeStat(ObjectVariant object, ESM::RefId id)
            : mObject(std::move(object))
            , mId(id)
        {
        }

    public:
        template <class G>
        sol::object get(const Context& context, std::string_view prop, G getter) const
        {
            return getValue(
                context, mObject, &AttributeStat::setValue, mId, prop, [this, getter](const MWWorld::Ptr& ptr) {
                    return (ptr.getClass().getCreatureStats(ptr).getAttribute(mId).*getter)();
                });
        }

        float getModified(const Context& context) const
        {
            auto base = LuaUtil::cast<float>(get(context, "base", &MWMechanics::AttributeValue::getBase));
            auto damage = LuaUtil::cast<float>(get(context, "damage", &MWMechanics::AttributeValue::getDamage));
            auto modifier = LuaUtil::cast<float>(get(context, "modifier", &MWMechanics::AttributeValue::getModifier));
            return std::max(0.f, base - damage + modifier); // Should match AttributeValue::getModified
        }

        static std::optional<AttributeStat> create(ObjectVariant object, Index i)
        {
            if (!object.ptr().getClass().isActor())
                return {};
            ESM::RefId id = std::get<ESM::RefId>(i);
            return AttributeStat{ std::move(object), id };
        }

        void cache(const Context& context, std::string_view prop, const sol::object& value) const
        {
            SelfObject* obj = mObject.asSelfObject();
            addStatUpdateAction(context.mLuaManager, *obj);
            obj->mStatsCache[SelfObject::CachedStat{ &AttributeStat::setValue, mId, prop }] = sol::main_object(value);
        }

        static void setValue(Index i, std::string_view prop, const MWWorld::Ptr& ptr, const sol::object& value)
        {
            ESM::RefId id = std::get<ESM::RefId>(i);
            auto& stats = ptr.getClass().getCreatureStats(ptr);
            auto stat = stats.getAttribute(id);
            float floatValue = LuaUtil::cast<float>(value);
            if (prop == "base")
                stat.setBase(floatValue);
            else if (prop == "damage")
            {
                stat.restore(stat.getDamage());
                stat.damage(floatValue);
            }
            else if (prop == "modifier")
                stat.setModifier(floatValue);
            stats.setAttribute(id, stat);
        }
    };

    class SkillStat
    {
        ObjectVariant mObject;
        ESM::RefId mId;

        SkillStat(ObjectVariant object, ESM::RefId id)
            : mObject(std::move(object))
            , mId(id)
        {
        }

        static float getProgress(const MWWorld::Ptr& ptr, ESM::RefId id, const MWMechanics::SkillValue& stat)
        {
            float progress = stat.getProgress();
            if (progress != 0.f)
                progress /= getMaxProgress(ptr, id, stat);
            return progress;
        }

        static float getMaxProgress(const MWWorld::Ptr& ptr, ESM::RefId id, const MWMechanics::SkillValue& stat)
        {
            const auto& store = *MWBase::Environment::get().getESMStore();
            const auto cl = store.get<ESM::Class>().find(ptr.get<ESM::NPC>()->mBase->mClass);
            return ptr.getClass().getNpcStats(ptr).getSkillProgressRequirement(id, *cl);
        }

    public:
        template <class G>
        sol::object get(const Context& context, std::string_view prop, G getter) const
        {
            return getValue(context, mObject, &SkillStat::setValue, mId, prop, [this, getter](const MWWorld::Ptr& ptr) {
                return (ptr.getClass().getNpcStats(ptr).getSkill(mId).*getter)();
            });
        }

        float getModified(const Context& context) const
        {
            auto base = LuaUtil::cast<float>(get(context, "base", &MWMechanics::SkillValue::getBase));
            auto damage = LuaUtil::cast<float>(get(context, "damage", &MWMechanics::SkillValue::getDamage));
            auto modifier = LuaUtil::cast<float>(get(context, "modifier", &MWMechanics::SkillValue::getModifier));
            return std::max(0.f, base - damage + modifier); // Should match SkillValue::getModified
        }

        sol::object getProgress(const Context& context) const
        {
            return getValue(context, mObject, &SkillStat::setValue, mId, "progress", [this](const MWWorld::Ptr& ptr) {
                return getProgress(ptr, mId, ptr.getClass().getNpcStats(ptr).getSkill(mId));
            });
        }

        static std::optional<SkillStat> create(ObjectVariant object, Index index)
        {
            if (!object.ptr().getClass().isNpc())
                return {};
            ESM::RefId id = std::get<ESM::RefId>(index);
            return SkillStat{ std::move(object), id };
        }

        void cache(const Context& context, std::string_view prop, const sol::object& value) const
        {
            SelfObject* obj = mObject.asSelfObject();
            addStatUpdateAction(context.mLuaManager, *obj);
            obj->mStatsCache[SelfObject::CachedStat{ &SkillStat::setValue, mId, prop }] = sol::main_object(value);
        }

        static void setValue(Index index, std::string_view prop, const MWWorld::Ptr& ptr, const sol::object& value)
        {
            ESM::RefId id = std::get<ESM::RefId>(index);
            auto& stats = ptr.getClass().getNpcStats(ptr);
            auto stat = stats.getSkill(id);
            float floatValue = LuaUtil::cast<float>(value);
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
                stat.setProgress(floatValue * getMaxProgress(ptr, id, stat));
            stats.setSkill(id, stat);
        }
    };

    class AIStat
    {
        ObjectVariant mObject;
        MWMechanics::AiSetting mIndex;

        AIStat(ObjectVariant object, MWMechanics::AiSetting index)
            : mObject(std::move(object))
            , mIndex(index)
        {
        }

    public:
        template <class G>
        sol::object get(const Context& context, std::string_view prop, G getter) const
        {
            return getValue(context, mObject, &AIStat::setValue, static_cast<int>(mIndex), prop,
                [this, getter](const MWWorld::Ptr& ptr) {
                    return (ptr.getClass().getCreatureStats(ptr).getAiSetting(mIndex).*getter)();
                });
        }

        int getModified(const Context& context) const
        {
            auto base = LuaUtil::cast<int>(get(context, "base", &MWMechanics::Stat<int>::getBase));
            auto modifier = LuaUtil::cast<int>(get(context, "modifier", &MWMechanics::Stat<int>::getModifier));
            return std::max(0, base + modifier);
        }

        static std::optional<AIStat> create(ObjectVariant object, MWMechanics::AiSetting index)
        {
            if (!object.ptr().getClass().isActor())
                return {};
            return AIStat{ std::move(object), index };
        }

        void cache(const Context& context, std::string_view prop, const sol::object& value) const
        {
            SelfObject* obj = mObject.asSelfObject();
            addStatUpdateAction(context.mLuaManager, *obj);
            obj->mStatsCache[SelfObject::CachedStat{ &AIStat::setValue, static_cast<int>(mIndex), prop }]
                = sol::main_object(value);
        }

        static void setValue(Index i, std::string_view prop, const MWWorld::Ptr& ptr, const sol::object& value)
        {
            auto index = static_cast<MWMechanics::AiSetting>(std::get<int>(i));
            auto& stats = ptr.getClass().getCreatureStats(ptr);
            auto stat = stats.getAiSetting(index);
            int intValue = LuaUtil::cast<int>(value);
            if (prop == "base")
                stat.setBase(intValue);
            else if (prop == "modifier")
                stat.setModifier(intValue);
            stats.setAiSetting(index, stat);
        }
    };
}

namespace sol
{
    template <>
    struct is_automagical<MWLua::SkillIncreasesForAttributeStats> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWLua::SkillIncreasesForSpecializationStats> : std::false_type
    {
    };
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
    template <>
    struct is_automagical<ESM::Attribute> : std::false_type
    {
    };
    template <>
    struct is_automagical<ESM::Skill> : std::false_type
    {
    };
    template <>
    struct is_automagical<ESM::MagicSchool> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWLua::AIStat> : std::false_type
    {
    };
}

namespace MWLua
{
    void addActorStatsBindings(sol::table& actor, const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table stats(lua, sol::create);
        actor["stats"] = LuaUtil::makeReadOnly(stats);

        auto skillIncreasesForAttributeStatsT
            = lua.new_usertype<SkillIncreasesForAttributeStats>("SkillIncreasesForAttributeStats");
        for (const auto& attribute : MWBase::Environment::get().getESMStore()->get<ESM::Attribute>())
        {
            skillIncreasesForAttributeStatsT[ESM::RefId(attribute.mId).serializeText()] = sol::property(
                [=](const SkillIncreasesForAttributeStats& stat) { return stat.get(context, attribute.mId); },
                [=](const SkillIncreasesForAttributeStats& stat, const sol::object& value) {
                    stat.set(context, attribute.mId, value);
                });
        }
        // ESM::Class::specializationIndexToLuaId.at(rec.mData.mSpecialization)
        auto skillIncreasesForSpecializationStatsT
            = lua.new_usertype<SkillIncreasesForSpecializationStats>("skillIncreasesForSpecializationStats");
        for (int i = 0; i < 3; i++)
        {
            std::string_view index = ESM::Class::specializationIndexToLuaId.at(i);
            skillIncreasesForSpecializationStatsT[index]
                = sol::property([=](const SkillIncreasesForSpecializationStats& stat) { return stat.get(context, i); },
                    [=](const SkillIncreasesForSpecializationStats& stat, const sol::object& value) {
                        stat.set(context, i, value);
                    });
        }

        auto levelStatT = lua.new_usertype<LevelStat>("LevelStat");
        levelStatT["current"] = sol::property([context](const LevelStat& stat) { return stat.getCurrent(context); },
            [context](const LevelStat& stat, const sol::object& value) { stat.setCurrent(context, value); });
        levelStatT["progress"] = sol::property([context](const LevelStat& stat) { return stat.getProgress(context); },
            [context](const LevelStat& stat, const sol::object& value) { stat.setProgress(context, value); });
        levelStatT["skillIncreasesForAttribute"]
            = sol::readonly_property([](const LevelStat& stat) { return stat.getSkillIncreasesForAttributeStats(); });
        levelStatT["skillIncreasesForSpecialization"] = sol::readonly_property(
            [](const LevelStat& stat) { return stat.getSkillIncreasesForSpecializationStats(); });
        stats["level"] = addIndexedAccessor<LevelStat>(0);

        auto dynamicStatT = lua.new_usertype<DynamicStat>("DynamicStat");
        addProp(context, dynamicStatT, "base", &MWMechanics::DynamicStat<float>::getBase);
        addProp(context, dynamicStatT, "current", &MWMechanics::DynamicStat<float>::getCurrent);
        addProp(context, dynamicStatT, "modifier", &MWMechanics::DynamicStat<float>::getModifier);
        sol::table dynamic(lua, sol::create);
        stats["dynamic"] = LuaUtil::makeReadOnly(dynamic);
        dynamic["health"] = addIndexedAccessor<DynamicStat>(0);
        dynamic["magicka"] = addIndexedAccessor<DynamicStat>(1);
        dynamic["fatigue"] = addIndexedAccessor<DynamicStat>(2);

        auto attributeStatT = lua.new_usertype<AttributeStat>("AttributeStat");
        addProp(context, attributeStatT, "base", &MWMechanics::AttributeValue::getBase);
        addProp(context, attributeStatT, "damage", &MWMechanics::AttributeValue::getDamage);
        attributeStatT["modified"]
            = sol::readonly_property([=](const AttributeStat& stat) { return stat.getModified(context); });
        addProp(context, attributeStatT, "modifier", &MWMechanics::AttributeValue::getModifier);
        sol::table attributes(lua, sol::create);
        stats["attributes"] = LuaUtil::makeReadOnly(attributes);
        for (const ESM::Attribute& attribute : MWBase::Environment::get().getESMStore()->get<ESM::Attribute>())
            attributes[ESM::RefId(attribute.mId).serializeText()] = addIndexedAccessor<AttributeStat>(attribute.mId);

        auto aiStatT = lua.new_usertype<AIStat>("AIStat");
        addProp(context, aiStatT, "base", &MWMechanics::Stat<int>::getBase);
        addProp(context, aiStatT, "modifier", &MWMechanics::Stat<int>::getModifier);
        aiStatT["modified"] = sol::readonly_property([=](const AIStat& stat) { return stat.getModified(context); });
        sol::table ai(lua, sol::create);
        stats["ai"] = LuaUtil::makeReadOnly(ai);
        ai["alarm"] = addIndexedAccessor<AIStat>(MWMechanics::AiSetting::Alarm);
        ai["fight"] = addIndexedAccessor<AIStat>(MWMechanics::AiSetting::Fight);
        ai["flee"] = addIndexedAccessor<AIStat>(MWMechanics::AiSetting::Flee);
        ai["hello"] = addIndexedAccessor<AIStat>(MWMechanics::AiSetting::Hello);
    }

    void addNpcStatsBindings(sol::table& npc, const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table npcStats(lua, sol::create);
        sol::table baseMeta(lua, sol::create);
        baseMeta[sol::meta_function::index] = LuaUtil::getMutableFromReadOnly(npc["baseType"]["stats"]);
        npcStats[sol::metatable_key] = baseMeta;
        npc["stats"] = LuaUtil::makeReadOnly(npcStats);

        auto skillStatT = lua.new_usertype<SkillStat>("SkillStat");
        addProp(context, skillStatT, "base", &MWMechanics::SkillValue::getBase);
        addProp(context, skillStatT, "damage", &MWMechanics::SkillValue::getDamage);
        skillStatT["modified"]
            = sol::readonly_property([=](const SkillStat& stat) { return stat.getModified(context); });
        addProp(context, skillStatT, "modifier", &MWMechanics::SkillValue::getModifier);
        skillStatT["progress"] = sol::property([context](const SkillStat& stat) { return stat.getProgress(context); },
            [context](const SkillStat& stat, const sol::object& value) { stat.cache(context, "progress", value); });
        sol::table skills(lua, sol::create);
        npcStats["skills"] = LuaUtil::makeReadOnly(skills);
        for (const ESM::Skill& skill : MWBase::Environment::get().getESMStore()->get<ESM::Skill>())
            skills[ESM::RefId(skill.mId).serializeText()] = addIndexedAccessor<SkillStat>(skill.mId);
    }

    sol::table initCoreStatsBindings(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table statsApi(lua, sol::create);
        auto* vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        sol::table attributes(lua, sol::create);
        addRecordFunctionBinding<ESM::Attribute>(attributes, context);
        statsApi["Attribute"] = LuaUtil::makeReadOnly(attributes);
        statsApi["Attribute"][sol::metatable_key][sol::meta_function::to_string] = ESM::Attribute::getRecordType;

        auto attributeT = lua.new_usertype<ESM::Attribute>("Attribute");
        attributeT[sol::meta_function::to_string]
            = [](const ESM::Attribute& rec) { return "ESM3_Attribute[" + rec.mId.toDebugString() + "]"; };
        attributeT["id"] = sol::readonly_property(
            [](const ESM::Attribute& rec) -> std::string { return ESM::RefId{ rec.mId }.serializeText(); });
        attributeT["name"]
            = sol::readonly_property([](const ESM::Attribute& rec) -> std::string_view { return rec.mName; });
        attributeT["description"]
            = sol::readonly_property([](const ESM::Attribute& rec) -> std::string_view { return rec.mDescription; });
        attributeT["icon"] = sol::readonly_property([vfs](const ESM::Attribute& rec) -> std::string {
            return Misc::ResourceHelpers::correctIconPath(VFS::Path::toNormalized(rec.mIcon), *vfs);
        });

        sol::table skills(lua, sol::create);
        addRecordFunctionBinding<ESM::Skill>(skills, context);
        statsApi["Skill"] = LuaUtil::makeReadOnly(skills);
        statsApi["Skill"][sol::metatable_key][sol::meta_function::to_string] = ESM::Skill::getRecordType;

        auto skillT = lua.new_usertype<ESM::Skill>("Skill");
        skillT[sol::meta_function::to_string]
            = [](const ESM::Skill& rec) { return "ESM3_Skill[" + rec.mId.toDebugString() + "]"; };
        skillT["id"] = sol::readonly_property(
            [](const ESM::Skill& rec) -> std::string { return ESM::RefId{ rec.mId }.serializeText(); });
        skillT["name"] = sol::readonly_property([](const ESM::Skill& rec) -> std::string_view { return rec.mName; });
        skillT["description"]
            = sol::readonly_property([](const ESM::Skill& rec) -> std::string_view { return rec.mDescription; });
        skillT["specialization"] = sol::readonly_property([](const ESM::Skill& rec) -> std::string_view {
            return ESM::Class::specializationIndexToLuaId.at(rec.mData.mSpecialization);
        });
        skillT["icon"] = sol::readonly_property([vfs](const ESM::Skill& rec) -> std::string {
            return Misc::ResourceHelpers::correctIconPath(VFS::Path::toNormalized(rec.mIcon), *vfs);
        });
        skillT["school"] = sol::readonly_property([](const ESM::Skill& rec) -> const ESM::MagicSchool* {
            if (!rec.mSchool)
                return nullptr;
            return &*rec.mSchool;
        });
        skillT["attribute"] = sol::readonly_property([](const ESM::Skill& rec) -> std::string {
            return ESM::Attribute::indexToRefId(rec.mData.mAttribute).serializeText();
        });
        skillT["skillGain"] = sol::readonly_property([lua](const ESM::Skill& rec) -> sol::table {
            sol::table res(lua, sol::create);
            int index = 1;
            for (auto skillGain : rec.mData.mUseValue)
                res[index++] = skillGain;
            return res;
        });

        auto schoolT = lua.new_usertype<ESM::MagicSchool>("MagicSchool");
        schoolT[sol::meta_function::to_string]
            = [](const ESM::MagicSchool& rec) { return "ESM3_MagicSchool[" + rec.mName + "]"; };
        schoolT["name"]
            = sol::readonly_property([](const ESM::MagicSchool& rec) -> std::string_view { return rec.mName; });
        schoolT["areaSound"] = sol::readonly_property(
            [](const ESM::MagicSchool& rec) -> std::string { return rec.mAreaSound.serializeText(); });
        schoolT["boltSound"] = sol::readonly_property(
            [](const ESM::MagicSchool& rec) -> std::string { return rec.mBoltSound.serializeText(); });
        schoolT["castSound"] = sol::readonly_property(
            [](const ESM::MagicSchool& rec) -> std::string { return rec.mCastSound.serializeText(); });
        schoolT["failureSound"] = sol::readonly_property(
            [](const ESM::MagicSchool& rec) -> std::string { return rec.mFailureSound.serializeText(); });
        schoolT["hitSound"] = sol::readonly_property(
            [](const ESM::MagicSchool& rec) -> std::string { return rec.mHitSound.serializeText(); });

        return LuaUtil::makeReadOnly(statsApi);
    }
}
