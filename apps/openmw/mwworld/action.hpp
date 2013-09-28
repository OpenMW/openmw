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
            bool mTeleport;

            // not implemented
            Action (const Action& action);
            Action& operator= (const Action& action);

            virtual void executeImp (const Ptr& actor) = 0;

    public:

            Action (bool teleport = false);
            ///< \param teleport action will teleport the actor

            virtual ~Action();

            void execute (const Ptr& actor);

            void setSound (const std::string& id);
    };
}

#endif
