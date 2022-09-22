#ifndef GAME_MWWORLD_FAILEDACTION_H
#define GAME_MWWORLD_FAILEDACTION_H

#include "action.hpp"
#include "ptr.hpp"

namespace MWWorld
{
    class FailedAction : public Action
    {
        std::string_view mMessage;

        void executeImp(const Ptr& actor) override;

    public:
        FailedAction(std::string_view message = {}, const Ptr& target = Ptr());
    };
}

#endif
