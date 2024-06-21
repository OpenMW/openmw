#include "localscripts.hpp"

#include <components/esm3/loadcell.hpp>
#include <components/esm3/loadweap.hpp>
#include <components/misc/strings/lower.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwmechanics/aicombat.hpp"
#include "../mwmechanics/aiescort.hpp"
#include "../mwmechanics/aifollow.hpp"
#include "../mwmechanics/aipackage.hpp"
#include "../mwmechanics/aipursue.hpp"
#include "../mwmechanics/aisequence.hpp"
#include "../mwmechanics/aitravel.hpp"
#include "../mwmechanics/aiwander.hpp"
#include "../mwmechanics/attacktype.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"

#include "context.hpp"

namespace sol
{
    template <>
    struct is_automagical<MWBase::LuaManager::ActorControls> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWLua::SelfObject> : std::false_type
    {
    };
}

namespace MWLua
{

    void LocalScripts::initializeSelfPackage(const Context& context)
    {
        using ActorControls = MWBase::LuaManager::ActorControls;
        sol::usertype<ActorControls> controls = context.mLua->sol().new_usertype<ActorControls>("ActorControls");

#define CONTROL(TYPE, FIELD)                                                                                           \
    sol::property([](const ActorControls& c) { return c.FIELD; },                                                      \
        [](ActorControls& c, const TYPE& v) {                                                                          \
            c.FIELD = v;                                                                                               \
            c.mChanged = true;                                                                                         \
        })
        controls["movement"] = CONTROL(float, mMovement);
        controls["sideMovement"] = CONTROL(float, mSideMovement);
        controls["pitchChange"] = CONTROL(float, mPitchChange);
        controls["yawChange"] = CONTROL(float, mYawChange);
        controls["run"] = CONTROL(bool, mRun);
        controls["sneak"] = CONTROL(bool, mSneak);
        controls["jump"] = CONTROL(bool, mJump);
        controls["use"] = CONTROL(int, mUse);
#undef CONTROL

        sol::usertype<SelfObject> selfAPI = context.mLua->sol().new_usertype<SelfObject>(
            "SelfObject", sol::base_classes, sol::bases<LObject, Object>());
        selfAPI[sol::meta_function::to_string]
            = [](SelfObject& self) { return "openmw.self[" + self.toString() + "]"; };
        selfAPI["object"] = sol::readonly_property([](SelfObject& self) -> LObject { return LObject(self); });
        selfAPI["controls"] = sol::readonly_property([](SelfObject& self) { return &self.mControls; });
        selfAPI["isActive"] = [](SelfObject& self) { return &self.mIsActive; };
        selfAPI["enableAI"] = [](SelfObject& self, bool v) { self.mControls.mDisableAI = !v; };
        selfAPI["AttackTYPE"]
            = LuaUtil::makeStrictReadOnly(context.mLua->tableFromPairs<std::string_view, MWMechanics::AttackType>(
                { { "NoAttack", MWMechanics::AttackType::NoAttack }, { "Any", MWMechanics::AttackType::Any },
                    { "Chop", MWMechanics::AttackType::Chop }, { "Slash", MWMechanics::AttackType::Slash },
                    { "Thrust", MWMechanics::AttackType::Thrust } }));

        using AiPackage = MWMechanics::AiPackage;
        sol::usertype<AiPackage> aiPackage = context.mLua->sol().new_usertype<AiPackage>("AiPackage");
        aiPackage["type"] = sol::readonly_property([](const AiPackage& p) -> std::string_view {
            switch (p.getTypeId())
            {
                case MWMechanics::AiPackageTypeId::Wander:
                    return "Wander";
                case MWMechanics::AiPackageTypeId::Travel:
                    return "Travel";
                case MWMechanics::AiPackageTypeId::Escort:
                    return "Escort";
                case MWMechanics::AiPackageTypeId::Follow:
                    return "Follow";
                case MWMechanics::AiPackageTypeId::Activate:
                    return "Activate";
                case MWMechanics::AiPackageTypeId::Combat:
                    return "Combat";
                case MWMechanics::AiPackageTypeId::Pursue:
                    return "Pursue";
                case MWMechanics::AiPackageTypeId::AvoidDoor:
                    return "AvoidDoor";
                case MWMechanics::AiPackageTypeId::Face:
                    return "Face";
                case MWMechanics::AiPackageTypeId::Breathe:
                    return "Breathe";
                case MWMechanics::AiPackageTypeId::Cast:
                    return "Cast";
                default:
                    return "Unknown";
            }
        });
        aiPackage["target"] = sol::readonly_property([](const AiPackage& p) -> sol::optional<LObject> {
            MWWorld::Ptr target = p.getTarget();
            if (target.isEmpty())
                return sol::nullopt;
            else
                return LObject(getId(target));
        });
        aiPackage["sideWithTarget"] = sol::readonly_property([](const AiPackage& p) { return p.sideWithTarget(); });
        aiPackage["destPosition"] = sol::readonly_property([](const AiPackage& p) { return p.getDestination(); });
        aiPackage["distance"] = sol::readonly_property([](const AiPackage& p) { return p.getDistance(); });
        aiPackage["duration"] = sol::readonly_property([](const AiPackage& p) { return p.getDuration(); });
        aiPackage["idle"] = sol::readonly_property([context](const AiPackage& p) -> sol::optional<sol::table> {
            if (p.getTypeId() == MWMechanics::AiPackageTypeId::Wander)
            {
                sol::table idles(context.mLua->sol(), sol::create);
                const std::vector<unsigned char>& idle = static_cast<const MWMechanics::AiWander&>(p).getIdle();
                if (!idle.empty())
                {
                    for (size_t i = 0; i < idle.size(); ++i)
                    {
                        std::string_view groupName = MWMechanics::AiWander::getIdleGroupName(i);
                        idles[groupName] = idle[i];
                    }
                    return idles;
                }
            }
            return sol::nullopt;
        });

        aiPackage["isRepeat"] = sol::readonly_property([](const AiPackage& p) { return p.getRepeat(); });
        selfAPI["_getActiveAiPackage"] = [](SelfObject& self) -> sol::optional<std::shared_ptr<AiPackage>> {
            const MWWorld::Ptr& ptr = self.ptr();
            MWMechanics::AiSequence& ai = ptr.getClass().getCreatureStats(ptr).getAiSequence();
            if (ai.isEmpty())
                return sol::nullopt;
            else
                return *ai.begin();
        };
        selfAPI["_iterateAndFilterAiSequence"] = [](SelfObject& self, sol::function callback) {
            const MWWorld::Ptr& ptr = self.ptr();
            MWMechanics::AiSequence& ai = ptr.getClass().getCreatureStats(ptr).getAiSequence();

            ai.erasePackagesIf([&](auto& entry) {
                bool keep = LuaUtil::call(callback, entry).template get<bool>();
                return !keep;
            });
        };
        selfAPI["_startAiCombat"] = [](SelfObject& self, const LObject& target, bool cancelOther) {
            const MWWorld::Ptr& ptr = self.ptr();
            MWMechanics::AiSequence& ai = ptr.getClass().getCreatureStats(ptr).getAiSequence();
            ai.stack(MWMechanics::AiCombat(target.ptr()), ptr, cancelOther);
        };
        selfAPI["_startAiPursue"] = [](SelfObject& self, const LObject& target, bool cancelOther) {
            const MWWorld::Ptr& ptr = self.ptr();
            MWMechanics::AiSequence& ai = ptr.getClass().getCreatureStats(ptr).getAiSequence();
            ai.stack(MWMechanics::AiPursue(target.ptr()), ptr, cancelOther);
        };
        selfAPI["_startAiFollow"] = [](SelfObject& self, const LObject& target, sol::optional<LCell> cell,
                                        float duration, const osg::Vec3f& dest, bool repeat, bool cancelOther) {
            const MWWorld::Ptr& ptr = self.ptr();
            MWMechanics::AiSequence& ai = ptr.getClass().getCreatureStats(ptr).getAiSequence();
            if (cell)
            {
                ai.stack(MWMechanics::AiFollow(target.ptr().getCellRef().getRefId(),
                             cell->mStore->getCell()->getNameId(), duration, dest.x(), dest.y(), dest.z(), repeat),
                    ptr, cancelOther);
            }
            else
            {
                ai.stack(MWMechanics::AiFollow(
                             target.ptr().getCellRef().getRefId(), duration, dest.x(), dest.y(), dest.z(), repeat),
                    ptr, cancelOther);
            }
        };
        selfAPI["_startAiEscort"] = [](SelfObject& self, const LObject& target, LCell cell, float duration,
                                        const osg::Vec3f& dest, bool repeat, bool cancelOther) {
            const MWWorld::Ptr& ptr = self.ptr();
            MWMechanics::AiSequence& ai = ptr.getClass().getCreatureStats(ptr).getAiSequence();
            // TODO: change AiEscort implementation to accept ptr instead of a non-unique refId.
            const ESM::RefId& refId = target.ptr().getCellRef().getRefId();
            int gameHoursDuration = static_cast<int>(std::ceil(duration / 3600.0));
            auto* esmCell = cell.mStore->getCell();
            if (esmCell->isExterior())
                ai.stack(MWMechanics::AiEscort(refId, gameHoursDuration, dest.x(), dest.y(), dest.z(), repeat), ptr,
                    cancelOther);
            else
                ai.stack(MWMechanics::AiEscort(
                             refId, esmCell->getNameId(), gameHoursDuration, dest.x(), dest.y(), dest.z(), repeat),
                    ptr, cancelOther);
        };
        selfAPI["_startAiWander"]
            = [](SelfObject& self, int distance, int duration, sol::table luaIdle, bool repeat, bool cancelOther) {
                  const MWWorld::Ptr& ptr = self.ptr();
                  MWMechanics::AiSequence& ai = ptr.getClass().getCreatureStats(ptr).getAiSequence();
                  std::vector<unsigned char> idle;
                  // Lua index starts at 1
                  for (size_t i = 1; i <= luaIdle.size(); i++)
                      idle.emplace_back(luaIdle.get<unsigned char>(i));
                  ai.stack(MWMechanics::AiWander(distance, duration, 0, idle, repeat), ptr, cancelOther);
              };
        selfAPI["_startAiTravel"] = [](SelfObject& self, const osg::Vec3f& target, bool repeat, bool cancelOther) {
            const MWWorld::Ptr& ptr = self.ptr();
            MWMechanics::AiSequence& ai = ptr.getClass().getCreatureStats(ptr).getAiSequence();
            ai.stack(MWMechanics::AiTravel(target.x(), target.y(), target.z(), repeat), ptr, cancelOther);
        };
        selfAPI["_enableLuaAnimations"] = [](SelfObject& self, bool enable) {
            const MWWorld::Ptr& ptr = self.ptr();
            MWBase::Environment::get().getMechanicsManager()->enableLuaAnimations(ptr, enable);
        };
    }

    LocalScripts::LocalScripts(LuaUtil::LuaState* lua, const LObject& obj)
        : LuaUtil::ScriptsContainer(lua, "L" + obj.id().toString())
        , mData(obj)
    {
        this->addPackage("openmw.self", sol::make_object(lua->sol(), &mData));
        registerEngineHandlers({ &mOnActiveHandlers, &mOnInactiveHandlers, &mOnConsumeHandlers, &mOnActivatedHandlers,
            &mOnTeleportedHandlers, &mOnAnimationTextKeyHandlers, &mOnPlayAnimationHandlers, &mOnSkillUse,
            &mOnSkillLevelUp });
    }

    void LocalScripts::setActive(bool active)
    {
        mData.mIsActive = active;
        if (active)
            callEngineHandlers(mOnActiveHandlers);
        else
            callEngineHandlers(mOnInactiveHandlers);
    }

    void LocalScripts::applyStatsCache()
    {
        const auto& ptr = mData.ptr();
        for (auto& [stat, value] : mData.mStatsCache)
            stat(ptr, value);
        mData.mStatsCache.clear();
    }
}
