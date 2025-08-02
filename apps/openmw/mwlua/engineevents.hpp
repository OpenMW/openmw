#ifndef MWLUA_ENGINEEVENTS_H
#define MWLUA_ENGINEEVENTS_H

#include <variant>

#include <components/esm3/cellref.hpp> // defines RefNum that is used as a unique id

#include "../mwworld/cellstore.hpp"

namespace MWLua
{
    class GlobalScripts;

    class EngineEvents
    {
    public:
        explicit EngineEvents(GlobalScripts& globalScripts)
            : mGlobalScripts(globalScripts)
        {
        }

        struct OnActive
        {
            ESM::RefNum mObject;
        };
        struct OnInactive
        {
            ESM::RefNum mObject;
        };
        struct OnTeleported
        {
            ESM::RefNum mObject;
        };
        struct OnActivate
        {
            ESM::RefNum mActor;
            ESM::RefNum mObject;
        };
        struct OnUseItem
        {
            ESM::RefNum mActor;
            ESM::RefNum mObject;
            bool mForce;
        };
        struct OnConsume
        {
            ESM::RefNum mActor;
            ESM::RefNum mConsumable;
        };
        struct OnNewExterior
        {
            MWWorld::CellStore& mCell;
        };
        struct OnAnimationTextKey
        {
            ESM::RefNum mActor;
            std::string mGroupname;
            std::string mKey;
        };
        struct OnSkillUse
        {
            ESM::RefNum mActor;
            std::string mSkill;
            int useType;
            float scale;
        };
        struct OnSkillLevelUp
        {
            ESM::RefNum mActor;
            std::string mSkill;
            std::string mSource;
        };
        struct OnJailTimeServed
        {
            ESM::RefNum mActor;
            int mDays;
        };
        using Event = std::variant<OnActive, OnInactive, OnConsume, OnActivate, OnUseItem, OnNewExterior, OnTeleported,
            OnAnimationTextKey, OnSkillUse, OnSkillLevelUp, OnJailTimeServed>;

        void clear() { mQueue.clear(); }
        void addToQueue(Event e) { mQueue.push_back(std::move(e)); }
        void callEngineHandlers();

    private:
        class Visitor;

        GlobalScripts& mGlobalScripts;
        std::vector<Event> mQueue;
    };

}

#endif // MWLUA_ENGINEEVENTS_H
