#ifndef GAME_MWWORLD_ACTION_H
#define GAME_MWWORLD_ACTION_H

namespace MWWorld
{
    /// \brief Abstract base for actions
    class Action
    {
            // not implemented
            Action (const Action& action);
            Action& operator= (const Action& action);

        public:

            Action() {}

            virtual ~Action() {}

            virtual void execute() = 0;
    };
}

#endif
