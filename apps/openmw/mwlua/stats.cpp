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
#include "types/usertypeutil.hpp"

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
            if (auto value = self->getCachedStat({ setter, index, prop }))
                return *value;
        }
        return sol::make_object(context.mLua->unsafeState(), getter(obj.ptr()));
    }

    struct MutableMagicSchool
    {
        MWLua::MutableRecord<ESM::Skill> mSkill;

        ESM::MagicSchool& find() { return mSkill.find().mSchool.value(); }

        const ESM::MagicSchool& find() const { return mSkill.find().mSchool.value(); }
    };

    struct MutableSkillGain
    {
        MWLua::MutableRecord<ESM::Skill> mSkill;

        std::array<float, 4>& find() { return mSkill.find().mData.mUseValue; }

        const std::array<float, 4>& find() const { return mSkill.find().mData.mUseValue; }
    };
}

namespace MWLua
{
    template <>
    struct Types::RecordType<MutableMagicSchool>
    {
        using Record = ESM::MagicSchool;
        constexpr static bool isMutable = true;

        static const Record& asRecord(const MutableMagicSchool& rec) { return rec.find(); }
    };

    namespace
    {
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
                stats.setSkillIncreasesForAttribute(std::get<ESM::RefId>(index), LuaUtil::cast<int>(value));
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

            sol::object get(const Context& context, ESM::RefId attributeId) const
            {
                if (!mObject.ptr().getClass().isNpc())
                    return sol::nil;

                return getValue(context, mObject, &setNpcValue, attributeId, "skillIncreasesForAttribute",
                    [attributeId](const MWWorld::Ptr& ptr) {
                        return ptr.getClass().getNpcStats(ptr).getSkillIncreasesForAttribute(attributeId);
                    });
            }

            void set(const Context& context, ESM::RefId attributeId, const sol::object& value) const
            {
                const auto& ptr = mObject.ptr();
                if (!ptr.getClass().isNpc())
                    return;

                SelfObject* obj = mObject.asSelfObject();
                obj->cacheStat(*context.mLuaManager,
                    SelfObject::CachedStat{ &setNpcValue, attributeId, "skillIncreasesForAttribute" }, value);
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
                obj->cacheStat(*context.mLuaManager,
                    SelfObject::CachedStat{ &setNpcValue, specialization, "skillIncreasesForSpecialization" }, value);
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
                obj->cacheStat(*context.mLuaManager,
                    SelfObject::CachedStat{ &setCreatureValue, std::monostate{}, "current" }, value);
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
                obj->cacheStat(
                    *context.mLuaManager, SelfObject::CachedStat{ &setNpcValue, std::monostate{}, "progress" }, value);
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
                obj->cacheStat(
                    *context.mLuaManager, SelfObject::CachedStat{ &DynamicStat::setValue, mIndex, prop }, value);
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
                auto modifier
                    = LuaUtil::cast<float>(get(context, "modifier", &MWMechanics::AttributeValue::getModifier));
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
                obj->cacheStat(
                    *context.mLuaManager, SelfObject::CachedStat{ &AttributeStat::setValue, mId, prop }, value);
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
                return getValue(
                    context, mObject, &SkillStat::setValue, mId, prop, [this, getter](const MWWorld::Ptr& ptr) {
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
                return getValue(
                    context, mObject, &SkillStat::setValue, mId, "progress", [this](const MWWorld::Ptr& ptr) {
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
                obj->cacheStat(*context.mLuaManager, SelfObject::CachedStat{ &SkillStat::setValue, mId, prop }, value);
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
                obj->cacheStat(*context.mLuaManager,
                    SelfObject::CachedStat{ &AIStat::setValue, static_cast<int>(mIndex), prop }, value);
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

        class ReputationStat
        {
            ObjectVariant mObject;

            ReputationStat(ObjectVariant object)
                : mObject(std::move(object))
            {
            }

        public:
            sol::object get(const Context& context, const std::string_view prop) const
            {
                return getValue(context, mObject, &ReputationStat::setValue, std::monostate{}, prop,
                    [](const MWWorld::Ptr& ptr) { return ptr.getClass().getNpcStats(ptr).getReputation(); });
            }

            static std::optional<ReputationStat> create(ObjectVariant object)
            {
                if (!object.ptr().getClass().isNpc())
                    return {};

                return ReputationStat{ std::move(object) };
            }

            void cache(const Context& context, const std::string_view prop, const sol::object& value) const
            {
                SelfObject* obj = mObject.asSelfObject();
                obj->cacheStat(*context.mLuaManager,
                    SelfObject::CachedStat{ &ReputationStat::setValue, std::monostate{}, prop }, value);
            }

            static void setValue(Index i, std::string_view prop, const MWWorld::Ptr& ptr, const sol::object& value)
            {
                MWMechanics::NpcStats& stats = ptr.getClass().getNpcStats(ptr);
                int intValue = LuaUtil::cast<int>(value);
                stats.setReputation(intValue);
            }
        };

        template <class T>
        void addAttributeType(sol::state_view& lua, std::string_view name)
        {
            sol::usertype<T> record = lua.new_usertype<T>(name);

            record[sol::meta_function::to_string]
                = [](const T& rec) -> std::string { return "ESM3_Attribute[" + rec.mId.toDebugString() + "]"; };
            record["id"] = sol::readonly_property([](const T& rec) -> ESM::RefId { return rec.mId; });

            Types::addProperty(record, "name", &ESM::Attribute::mName);
            Types::addProperty(record, "description", &ESM::Attribute::mDescription);
            Types::addIconProperty(record);
            Types::addProperty(record, "werewolfValue", &ESM::Attribute::mWerewolfValue);
        }

        template <class T>
        void addSchoolType(sol::state_view& lua, std::string_view name)
        {
            auto record = lua.new_usertype<T>(name);
            Types::addProperty(record, "name", &ESM::MagicSchool::mName);
            Types::addProperty(record, "areaSound", &ESM::MagicSchool::mAreaSound);
            Types::addProperty(record, "boltSound", &ESM::MagicSchool::mBoltSound);
            Types::addProperty(record, "castSound", &ESM::MagicSchool::mCastSound);
            Types::addProperty(record, "failureSound", &ESM::MagicSchool::mFailureSound);
            Types::addProperty(record, "hitSound", &ESM::MagicSchool::mHitSound);
            Types::addProperty(record, "autoCalcMax", &ESM::MagicSchool::mAutoCalcMax);
            if constexpr (!Types::RecordType<T>::isMutable)
            {
                record[sol::meta_function::to_string]
                    = [](const ESM::MagicSchool& rec) { return "ESM3_MagicSchool[" + rec.mName + "]"; };
            }
        }

        int32_t getSpecialization(std::string_view id)
        {
            for (int32_t i = ESM::Class::Combat; i <= ESM::Class::Stealth; ++i)
            {
                if (ESM::Class::specializationIndexToLuaId[i] == id)
                    return i;
            }
            throw std::runtime_error("invalid specialization");
        }

        void setSchoolFromTable(ESM::Skill& skill, const sol::object& value)
        {
            if (value == sol::nil)
            {
                skill.mSchool.reset();
            }
            else if (value.is<MutableMagicSchool>())
            {
                const MutableMagicSchool& other = value.as<MutableMagicSchool>();
                skill.mSchool = other.find();
            }
            else
            {
                auto table = value.as<sol::lua_table>();
                ESM::MagicSchool& school = skill.mSchool.emplace();
                school.mName = table.get_or<std::string_view>("name", {});
                school.mAreaSound = ESM::RefId::deserializeText(table.get_or<std::string_view>("areaSound", {}));
                school.mBoltSound = ESM::RefId::deserializeText(table.get_or<std::string_view>("boltSound", {}));
                school.mCastSound = ESM::RefId::deserializeText(table.get_or<std::string_view>("castSound", {}));
                school.mFailureSound = ESM::RefId::deserializeText(table.get_or<std::string_view>("failureSound", {}));
                school.mHitSound = ESM::RefId::deserializeText(table.get_or<std::string_view>("hitSound", {}));
                school.mAutoCalcMax = table.get_or("autoCalcMax", 0);
            }
        }

        void setSkillGainFromTable(ESM::Skill& skill, const sol::object& value)
        {
            if (value == sol::nil)
            {
                skill.mData.mUseValue.fill(0.f);
            }
            else if (value.is<MutableSkillGain>())
            {
                const auto& other = value.as<MutableSkillGain>().find();
                for (size_t i = 0; i < skill.mData.mUseValue.size(); ++i)
                    skill.mData.mUseValue[i] = other[i];
            }
            else
            {
                auto table = value.as<sol::lua_table>();
                const size_t length = table.size();
                skill.mData.mUseValue.fill(0.f);
                for (size_t i = 0; i < skill.mData.mUseValue.size() && i < length; ++i)
                    skill.mData.mUseValue[i] = table.get<Misc::FiniteFloat>(LuaUtil::toLuaIndex(i));
            }
        }

        template <class T>
        void addSkillType(sol::state_view& lua, std::string_view name)
        {
            sol::usertype<T> record = lua.new_usertype<T>(name);

            record[sol::meta_function::to_string]
                = [](const T& rec) -> std::string { return "ESM3_Skill[" + rec.mId.toDebugString() + "]"; };
            record["id"] = sol::readonly_property([](const T& rec) -> ESM::RefId { return rec.mId; });

            Types::addProperty(record, "name", &ESM::Skill::mName);
            Types::addProperty(record, "description", &ESM::Skill::mDescription);
            Types::addIconProperty(record);
            Types::addProperty(record, "werewolfValue", &ESM::Skill::mWerewolfValue);
            Types::addProperty(record, "attribute", &ESM::Skill::mData, &ESM::Skill::SKDTstruct::mAttribute);

            if constexpr (Types::RecordType<T>::isMutable)
            {
                record["specialization"] = sol::property(
                    [](const MutableRecord<ESM::Skill>& rec) -> std::string_view {
                        return ESM::Class::specializationIndexToLuaId.at(rec.find().mData.mSpecialization);
                    },
                    [](MutableRecord<ESM::Skill>& rec, std::string_view value) {
                        ESM::Skill& skill = rec.find();
                        skill.mData.mSpecialization = getSpecialization(value);
                    });
                record["school"] = sol::property(
                    [](const MutableRecord<ESM::Skill>& rec) -> std::optional<MutableMagicSchool> {
                        const ESM::Skill& skill = rec.find();
                        if (skill.mSchool)
                            return MutableMagicSchool{ rec };
                        return {};
                    },
                    [](MutableRecord<ESM::Skill>& rec, const sol::object& value) {
                        setSchoolFromTable(rec.find(), value);
                    });
                record["skillGain"]
                    = sol::property([](const MutableRecord<ESM::Skill>& rec) { return MutableSkillGain{ rec }; },
                        [](MutableRecord<ESM::Skill>& rec, const sol::object& value) {
                            setSkillGainFromTable(rec.find(), value);
                        });
                addSchoolType<MutableMagicSchool>(lua, "ESM3_MutableMagicSchool");

                auto gainT = lua.new_usertype<MutableSkillGain>("ESM3_MutableSkillGain");
                gainT[sol::meta_function::length] = [](const MutableSkillGain& array) { return array.find().size(); };
                gainT[sol::meta_function::index]
                    = [](const MutableSkillGain& array, uint32_t index) -> std::optional<float> {
                    const auto& values = array.find();
                    if (index == 0 || index > values.size())
                        return {};
                    return values[index - 1];
                };
                gainT[sol::meta_function::new_index]
                    = [](MutableSkillGain& array, uint32_t index, Misc::FiniteFloat value) {
                          auto& values = array.find();
                          if (index == 0 || index > values.size())
                              throw std::runtime_error("index out of range");
                          values[index - 1] = value;
                      };
                gainT[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();
                gainT[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();
            }
            else
            {
                record["specialization"] = sol::readonly_property([](const ESM::Skill& rec) -> std::string_view {
                    return ESM::Class::specializationIndexToLuaId.at(rec.mData.mSpecialization);
                });
                record["school"] = sol::readonly_property([](const ESM::Skill& rec) -> const ESM::MagicSchool* {
                    if (!rec.mSchool)
                        return nullptr;
                    return &*rec.mSchool;
                });
                record["skillGain"] = sol::readonly_property([lua](const ESM::Skill& rec) -> sol::table {
                    sol::table res(lua, sol::create);
                    int index = 1;
                    for (float skillGain : rec.mData.mUseValue)
                        res[index++] = skillGain;
                    return res;
                });
                addSchoolType<ESM::MagicSchool>(lua, "MagicSchool");
            }
        }
    }
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
    template <>
    struct is_automagical<MutableMagicSchool> : std::false_type
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

        auto reputationStatT = lua.new_usertype<ReputationStat>("ReputationStat");
        reputationStatT["current"]
            = sol::property([=](const ReputationStat& stat) { return stat.get(context, "current"); },
                [=](const ReputationStat& stat, const sol::object& value) { stat.cache(context, "current", value); });

        npcStats["reputation"] = [](const sol::object& o) { return ReputationStat::create(ObjectVariant(o)); };
    }

    sol::table initCoreStatsBindings(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table statsApi(lua, sol::create);

        sol::table attributes(lua, sol::create);
        addRecordFunctionBinding<ESM::Attribute>(attributes, context);
        statsApi["Attribute"] = LuaUtil::makeReadOnly(attributes);
        statsApi["Attribute"][sol::metatable_key][sol::meta_function::to_string] = ESM::Attribute::getRecordType;

        addAttributeType<ESM::Attribute>(lua, "Attribute");

        sol::table skills(lua, sol::create);
        addRecordFunctionBinding<ESM::Skill>(skills, context);
        statsApi["Skill"] = LuaUtil::makeReadOnly(skills);
        statsApi["Skill"][sol::metatable_key][sol::meta_function::to_string] = ESM::Skill::getRecordType;

        addSkillType<ESM::Skill>(lua, "Skill");

        return LuaUtil::makeReadOnly(statsApi);
    }

    ESM::Attribute tableToAttribute(const sol::table& rec)
    {
        auto attribute = Types::initFromTemplate<ESM::Attribute>(rec);
        if (rec["description"] != sol::nil)
            attribute.mDescription = rec["description"];
        if (rec["icon"] != sol::nil)
            attribute.mIcon = rec["icon"].get<std::string_view>();
        if (rec["name"] != sol::nil)
            attribute.mName = rec["name"];
        if (rec["werewolfValue"] != sol::nil)
            attribute.mWerewolfValue = rec["werewolfValue"].get<Misc::FiniteFloat>();
        return attribute;
    }

    void addMutableAttributeType(sol::state_view& lua)
    {
        addAttributeType<MutableRecord<ESM::Attribute>>(lua, "ESM3_MutableAttribute");
    }

    ESM::Skill tableToSkill(const sol::table& rec)
    {
        auto skill = Types::initFromTemplate<ESM::Skill>(rec);
        if (rec["description"] != sol::nil)
            skill.mDescription = rec["description"];
        if (rec["name"] != sol::nil)
            skill.mName = rec["name"];
        if (rec["icon"] != sol::nil)
            skill.mIcon = rec["icon"].get<std::string_view>();
        if (rec["werewolfValue"] != sol::nil)
            skill.mWerewolfValue = rec["werewolfValue"].get<Misc::FiniteFloat>();
        if (rec["attribute"] != sol::nil)
            skill.mData.mAttribute = ESM::RefId::deserializeText(rec["attribute"].get<std::string_view>());
        if (rec["specialization"] != sol::nil)
            skill.mData.mSpecialization = getSpecialization(rec["specialization"].get<std::string_view>());
        if (rec["school"] != sol::nil)
            setSchoolFromTable(skill, rec["school"]);
        if (rec["skillGain"] != sol::nil)
            setSkillGainFromTable(skill, rec["skillGain"]);
        return skill;
    }

    void addMutableSkillType(sol::state_view& lua)
    {
        addSkillType<MutableRecord<ESM::Skill>>(lua, "ESM3_MutableSkill");
    }
}
