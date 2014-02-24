#ifndef GAME_MWWORLD_ACTION_H
#define GAME_MWWORLD_ACTION_H

#include <string>

#include "ptr.hpp"

namespace MWWorld
{
    /// \brief Abstract base for actions
    class Action
    {
            std::string mSoundId;
            bool mKeepSound;
            float mSoundOffset;
            Ptr mTarget;

            // not implemented
            Action (const Action& action);
            Action& operator= (const Action& action);

            virtual void executeImp (const Ptr& actor) = 0;

        protected:

            const Ptr& getTarget() const;

        public:

            Action (bool keepSound = false, const Ptr& target = Ptr());
            ///< \param keepSound Keep playing the sound even if the object the sound is played on is removed.

            virtual ~Action();

            void execute (const Ptr& actor);

            void setSound (const std::string& id);
            void setSoundOffset(float offset);
    };
}

#endif
