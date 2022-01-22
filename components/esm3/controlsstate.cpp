#include "controlsstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

ESM::ControlsState::ControlsState()
    : mViewSwitchDisabled(false),
      mControlsDisabled(false),
      mJumpingDisabled(false),
      mLookingDisabled(false),
      mVanityModeDisabled(false),
      mWeaponDrawingDisabled(false),
      mSpellDrawingDisabled(false)
{
}

void ESM::ControlsState::load(ESM::ESMReader& esm)
{
    int flags;
    esm.getHNT(flags, "CFLG");

    mViewSwitchDisabled = flags & ViewSwitchDisabled;
    mControlsDisabled = flags & ControlsDisabled;
    mJumpingDisabled = flags & JumpingDisabled;
    mLookingDisabled = flags & LookingDisabled;
    mVanityModeDisabled = flags & VanityModeDisabled;
    mWeaponDrawingDisabled = flags & WeaponDrawingDisabled;
    mSpellDrawingDisabled = flags & SpellDrawingDisabled;
}

void ESM::ControlsState::save(ESM::ESMWriter& esm) const
{
    int flags = 0;
    if (mViewSwitchDisabled) flags |= ViewSwitchDisabled;
    if (mControlsDisabled) flags |= ControlsDisabled;
    if (mJumpingDisabled) flags |= JumpingDisabled;
    if (mLookingDisabled) flags |= LookingDisabled;
    if (mVanityModeDisabled) flags |= VanityModeDisabled;
    if (mWeaponDrawingDisabled) flags |= WeaponDrawingDisabled;
    if (mSpellDrawingDisabled) flags |= SpellDrawingDisabled;

    esm.writeHNT("CFLG", flags);
}
