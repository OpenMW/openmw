#ifndef GAME_MWWORLD_FAILEDACTION_H
#define GAME_MWWORLD_FAILEDACTION_H

#include "action.hpp"
#include "ptr.hpp"

namespace MWWorld
{
    class FailedAction : public Action
    {
        std::string mMessage;

        virtual void executeImp(const Ptr &actor);

    public:
        FailedAction(const std::string &message = std::string(), const Ptr& target = Ptr());
    };
}

#endif
