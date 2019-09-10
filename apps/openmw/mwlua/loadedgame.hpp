#pragma once

#include "genericevent.hpp"
#include "disableableevent.hpp"

namespace mwse
{
    namespace lua
    {
        namespace event
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
}
