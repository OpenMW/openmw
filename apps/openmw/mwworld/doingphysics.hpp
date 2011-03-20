#ifndef GAME_MWWORLD_DOINGPHYSICS_H
#define GAME_MWWORLD_DOINGPHYSICS_H

namespace MWWorld
{
    class SuppressDoingPhysics;

    /// Scope guard for blocking physics updates during physics simulation.
    class DoingPhysics
    {
            static int sCounter;
            static int sSuppress;

        private:

            DoingPhysics (const DoingPhysics&);
            DoingPhysics& operator= (const DoingPhysics&);

        public:

            DoingPhysics();

            ~DoingPhysics();

            static bool isDoingPhysics();

        friend  class SuppressDoingPhysics;
    };

    /// Scope guard for temporarily lifting the block issues by DoingPhysics
    class SuppressDoingPhysics
    {
        private:

            SuppressDoingPhysics (const SuppressDoingPhysics&);
            SuppressDoingPhysics& operator= (const SuppressDoingPhysics&);

        public:

            SuppressDoingPhysics();

            ~SuppressDoingPhysics();
    };
}

#endif
