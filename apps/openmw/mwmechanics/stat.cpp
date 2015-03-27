
#include "stat.hpp"

void MWMechanics::AttributeValue::writeState (ESM::StatState<int>& state) const
{
    state.mBase = mBase;
    state.mMod = mModifier;
    state.mDamage = mDamage;
}

void MWMechanics::AttributeValue::readState (const ESM::StatState<int>& state)
{
    mBase = state.mBase;
    mModifier = state.mMod;
    mDamage = state.mDamage;
}

void MWMechanics::SkillValue::writeState (ESM::StatState<int>& state) const
{
    AttributeValue::writeState (state);
    state.mProgress = mProgress;
}

void MWMechanics::SkillValue::readState (const ESM::StatState<int>& state)
{
    AttributeValue::readState (state);
    mProgress = state.mProgress;
}
