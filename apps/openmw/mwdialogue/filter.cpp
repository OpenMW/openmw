
#include "filter.hpp"

MWDialogue::Filter::Filter (const MWWorld::Ptr& actor) : mActor (actor) {}

bool MWDialogue::Filter::operator() (const ESM::DialInfo& dialogue)
{

    return true;
}
