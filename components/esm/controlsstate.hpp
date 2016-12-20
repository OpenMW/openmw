#ifndef OPENMW_ESM_CONTROLSSTATE_H
#define OPENMW_ESM_CONTROLSSTATE_H

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only

    struct ControlsState
    {
        ControlsState();

        enum Flags
        {
            ViewSwitchDisabled = 0x1,
            ControlsDisabled = 0x4,
            JumpingDisabled = 0x1000,
            LookingDisabled = 0x2000,
            VanityModeDisabled = 0x4000,
            WeaponDrawingDisabled = 0x8000,
            SpellDrawingDisabled = 0x10000
        };

        bool mViewSwitchDisabled;
        bool mControlsDisabled;
        bool mJumpingDisabled;
        bool mLookingDisabled;
        bool mVanityModeDisabled;
        bool mWeaponDrawingDisabled;
        bool mSpellDrawingDisabled;

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };
}

#endif
