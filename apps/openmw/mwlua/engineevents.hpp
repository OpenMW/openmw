#ifndef MWLUA_ENGINEEVENTS_H
#define MWLUA_ENGINEEVENTS_H

#include <variant>

#include <components/esm3/cellref.hpp> // defines RefNum that is used as a unique id

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

        struct OnNewGame
        {
        };
        struct OnActive
        {
            ESM::RefNum mObject;
        };
        struct OnInactive
        {
            ESM::RefNum mObject;
        };
        struct OnActivate
        {
            ESM::RefNum mActor;
            ESM::RefNum mObject;
        };
        struct OnConsume
        {
            ESM::RefNum mActor;
            ESM::RefNum mConsumable;
        };
        using Event = std::variant<OnNewGame, OnActive, OnInactive, OnConsume, OnActivate>;

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
