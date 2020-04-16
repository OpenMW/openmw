#include "controlswitch.hpp"

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
}
