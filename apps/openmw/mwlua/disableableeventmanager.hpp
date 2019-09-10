#ifndef GAME_MWLUA_EVENTMANAGER_H
#define GAME_MWLUA_EVENTMANAGER_H

namespace MWLua
{
    namespace Event
    {
        class DisableableEventManager
        {
        public:
            static void bindToLua();
        };
    }
}

#endif
