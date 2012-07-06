
#include "npcstats.hpp"

MWMechanics::NpcStats::NpcStats()
: mForceRun (false), mForceSneak (false), mRun (false), mSneak (false),
  mDrawState (DrawState_Nothing)
{}

MWMechanics::DrawState MWMechanics::NpcStats::getDrawState() const
{
    return mDrawState;
}

void MWMechanics::NpcStats::setDrawState (DrawState state)
{
    mDrawState = state;
}
