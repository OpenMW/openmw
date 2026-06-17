#include "controlswitch.hpp"

#include <components/esm3/controlsstate.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>

#include <components/loadinglistener/loadinglistener.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

namespace MWInput
{
    ControlSwitch::ControlSwitch()
    {
        clear();
    }

    void ControlSwitch::clear()
    {
        mSwitches["playercontrols"] = true;
        mSwitches["playerfighting"] = true;
        mSwitches["playerjumping"] = true;
        mSwitches["playerlooking"] = true;
        mSwitches["playermagic"] = true;
        mSwitches["playerviewswitch"] = true;
        mSwitches["vanitymode"] = true;
    }

    bool ControlSwitch::get(std::string_view key)
    {
        auto it = mSwitches.find(key);
        if (it == mSwitches.end())
            throw std::runtime_error("Incorrect ControlSwitch: " + std::string(key));
        return it->second;
    }

    void ControlSwitch::set(std::string_view key, bool value)
    {
        if (key == "playerlooking" && !value)
        {
            auto world = MWBase::Environment::get().getWorld();
            world->rotateObject(world->getPlayerPtr(), osg::Vec3f());
        }
        auto it = mSwitches.find(key);
        if (it == mSwitches.end())
            throw std::runtime_error("Incorrect ControlSwitch: " + std::string(key));
        it->second = value;
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

        writer.startRecord(ESM::REC_INPU);
        controls.save(writer);
        writer.endRecord(ESM::REC_INPU);
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

    size_t ControlSwitch::countSavedGameRecords() const
    {
        return 1;
    }
}
