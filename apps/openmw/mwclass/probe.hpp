#ifndef GAME_MWCLASS_PROBE_H
#define GAME_MWCLASS_PROBE_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class Probe : public MWWorld::Class
    {
        public:

            virtual std::string getName (const MWWorld::Ptr& ptr) const;
            ///< \return name (the one that is to be presented to the user; not the internal one);
            /// can return an empty string.

            static void registerSelf();
    };
}

#endif
