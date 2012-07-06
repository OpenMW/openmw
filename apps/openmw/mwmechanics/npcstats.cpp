
#include "npcstats.hpp"

#include <stdexcept>

MWMechanics::NpcStats::NpcStats()
: mMovementFlags (0), mDrawState (DrawState_Nothing)
{}

MWMechanics::DrawState MWMechanics::NpcStats::getDrawState() const
{
    return mDrawState;
}

void MWMechanics::NpcStats::setDrawState (DrawState state)
{
    mDrawState = state;
}

bool MWMechanics::NpcStats::getMovementFlag (Flag flag) const
{
    return mMovementFlags & flag;
}

void MWMechanics::NpcStats::setMovementFlag (Flag flag, bool state)
{
    if (state)
        mMovementFlags |= flag;
    else
        mMovementFlags &= ~flag;
}

const MWMechanics::Stat<float>& MWMechanics::NpcStats::getSkill (int index) const
{
    if (index<0 || index>=27)
        throw std::runtime_error ("skill index out of range");

    return mSkill[index];
}

MWMechanics::Stat<float>& MWMechanics::NpcStats::getSkill (int index)
{
    if (index<0 || index>=27)
        throw std::runtime_error ("skill index out of range");

    return mSkill[index];
}

std::map<std::string, int>& MWMechanics::NpcStats::getFactionRanks()
{
    return mFactionRank;
}

const std::map<std::string, int>& MWMechanics::NpcStats::getFactionRanks() const
{
    return mFactionRank;
}
