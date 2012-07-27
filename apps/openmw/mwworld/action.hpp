#ifndef GAME_MWWORLD_ACTION_H
#define GAME_MWWORLD_ACTION_H

#include <string>

namespace MWWorld
{
    class Ptr;

    /// \brief Abstract base for actions
    class Action
    {
            std::string mSoundId;

            // not implemented
            Action (const Action& action);
            Action& operator= (const Action& action);

            virtual void executeImp (const Ptr& actor) = 0;

        public:

            Action();

            virtual ~Action();

            void execute (const Ptr& actor);

            void setSound (const std::string& id);
    };
}

#endif
