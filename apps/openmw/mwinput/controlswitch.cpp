#include "controlswitch.hpp"

#include <components/esm/esmwriter.hpp>
#include <components/esm/esmreader.hpp>
#include <components/esm/controlsstate.hpp>

#include <components/loadinglistener/loadinglistener.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/player.hpp"

namespace MWInput
{
    ControlSwitch::ControlSwitch()
    {
        clear();
    }

    void ControlSwitch::clear()
    {
        mSwitches["playercontrols"]      = true;
        mSwitches["playerfighting"]      = true;
        mSwitches["playerjumping"]       = true;
        mSwitches["playerlooking"]       = true;
        mSwitches["playermagic"]         = true;
        mSwitches["playerviewswitch"]    = true;
        mSwitches["vanitymode"]          = true;
    }

    bool ControlSwitch::get(const std::string& key)
    {
        return mSwitches[key];
    }

    void ControlSwitch::set(const std::string& key, bool value)
    {
        MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();

        /// \note 7 switches at all, if-else is relevant
        if (key == "playercontrols" && !value)
        {
            player.setLeftRight(0);
            player.setForwardBackward(0);
            player.setAutoMove(false);
            player.setUpDown(0);
        }
        else if (key == "playerjumping" && !value)
        {
            /// \fixme maybe crouching at this time
            player.setUpDown(0);
        }
        else if (key == "vanitymode")
        {
            MWBase::Environment::get().getWorld()->allowVanityMode(value);
        }
        else if (key == "playerlooking" && !value)
        {
            MWBase::Environment::get().getWorld()->rotateObject(player.getPlayer(), 0.f, 0.f, 0.f);
        }
        mSwitches[key] = value;
    }

    void ControlSwitch::write(ESM::ESMWriter& writer, Loading::Listener& /*progress*/)
    {
        ESM::ControlsState controls;
        controls.mViewSwitchDisabled = !mSwitches["playerviewswitch"];
        controls.mControlsDisabled = !mSwitches["playercontrols"];
        controls.mJumpingDisabled = !mSwitches["playerjumping"];
        controls.mLookingDisabled = !mSwitches["playerlooking"];
        controls.mVanityModeDisabled = !mSwitches["vanitymode"];
        controls.mWeaponDrawingDisabled = !mSwitches["playerfighting"];
        controls.mSpellDrawingDisabled = !mSwitches["playermagic"];

        writer.startRecord (ESM::REC_INPU);
        controls.save(writer);
        writer.endRecord (ESM::REC_INPU);
    }

    void ControlSwitch::readRecord(ESM::ESMReader& reader, uint32_t type)
    {
        ESM::ControlsState controls;
        controls.load(reader);

        set("playerviewswitch", !controls.mViewSwitchDisabled);
        set("playercontrols", !controls.mControlsDisabled);
        set("playerjumping", !controls.mJumpingDisabled);
        set("playerlooking", !controls.mLookingDisabled);
        set("vanitymode", !controls.mVanityModeDisabled);
        set("playerfighting", !controls.mWeaponDrawingDisabled);
        set("playermagic", !controls.mSpellDrawingDisabled);
    }

    int ControlSwitch::countSavedGameRecords() const
    {
        return 1;
    }
}
