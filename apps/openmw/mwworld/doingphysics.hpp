#ifndef GAME_MWWORLD_DOINGPHYSICS_H
#define GAME_MWWORLD_DOINGPHYSICS_H

namespace MWWorld
{
    ///< Scope guard for blocking physics updates during physics simulation.
    class DoingPhysics
    {
            static int sCounter;

        private:

            DoingPhysics (const DoingPhysics&);
            DoingPhysics& operator= (const DoingPhysics&);

        public:

            DoingPhysics();

            ~DoingPhysics();

            static bool isDoingPhysics();
    };
}

#endif
