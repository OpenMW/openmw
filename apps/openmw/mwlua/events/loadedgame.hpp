#ifndef GAME_MWLUA_EVENT_LOADEDGAME_H
#define GAME_MWLUA_EVENT_LOADEDGAME_H

#include "../genericevent.hpp"
#include "../disableableevent.hpp"

namespace MWLua
{
    namespace Event
    {
        class LoadedGameEvent : public GenericEvent, public DisableableEvent<LoadedGameEvent>
        {
        public:
            LoadedGameEvent(const char* fileName, bool quickLoad = false, bool newGame = false);
            sol::table createEventTable();
            sol::object getEventOptions();

        protected:
            const char* mFileName;
            bool mQuickLoad;
            bool mNewGame;
        };
    }
}

#endif
