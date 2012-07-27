
#include "action.hpp"

MWWorld::Action::Action() {}

MWWorld::Action::~Action() {}

void MWWorld::Action::execute (const Ptr& actor)
{
    executeImp (actor);
}
