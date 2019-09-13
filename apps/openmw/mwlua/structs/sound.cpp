#include "sound.hpp"

#include "../luamanager.hpp"

#include <components/esm/loadsndg.hpp>
#include <components/esm/loadsoun.hpp>

namespace MWLua
{
    void bindTES3Sound()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Binding for ESM::Sound.
        {
            // Start our usertype. We must finish this with state.set_usertype.
            auto usertypeDefinition = state.create_simple_usertype<ESM::Sound>();
            usertypeDefinition.set("new", sol::no_constructor);

            usertypeDefinition.set(sol::meta_function::to_string, sol::readonly_property([](ESM::Sound& self) { return self.mId; }));
            usertypeDefinition.set("id", sol::readonly_property([](ESM::Sound& self) { return self.mId; }));
            usertypeDefinition.set("volume", sol::property(
                [](ESM::Sound& self) { return self.mData.mVolume; },
                [](ESM::Sound& self, unsigned char value) { self.mData.mVolume = value; }
            ));
            usertypeDefinition.set("minRange", sol::property(
                [](ESM::Sound& self) { return self.mData.mMinRange; },
                [](ESM::Sound& self, unsigned char value) { self.mData.mMinRange = value; }
            ));
            usertypeDefinition.set("maxRange", sol::property(
                [](ESM::Sound& self) { return self.mData.mMaxRange; },
                [](ESM::Sound& self, unsigned char value) { self.mData.mMaxRange = value; }
            ));

            // Allow object to be serialized to json.
            usertypeDefinition.set("__tojson", [](ESM::Sound& self, sol::table state)
            {
                std::ostringstream ss;
                ss << "\"tes3sound:" << self.mId << "\"";
                return ss.str();
            });

            // Access to other objects that need to be packaged.
            usertypeDefinition.set("filename", sol::readonly_property(&ESM::Sound::mSound));

            // Finish up our usertype.
            state.set_usertype("tes3sound", usertypeDefinition);
        }

        // Binding for ESM::SoundGenerator.
        {
            // Start our usertype. We must finish this with state.set_usertype.
            auto usertypeDefinition = state.create_simple_usertype<ESM::SoundGenerator>();
            usertypeDefinition.set("new", sol::no_constructor);

            usertypeDefinition.set(sol::meta_function::to_string, sol::readonly_property([](ESM::SoundGenerator& self) { return self.mId; }));
            usertypeDefinition.set("id", sol::readonly_property([](ESM::SoundGenerator& self) { return self.mId; }));

            // Allow object to be serialized to json.
            usertypeDefinition.set("__tojson", [](ESM::SoundGenerator& self, sol::table state)
            {
                std::ostringstream ss;
                ss << "\"tes3soundGenerator:" << self.mId << "\"";
                return ss.str();
            });

            // Basic property binding.
            usertypeDefinition.set("creature", sol::readonly_property(&ESM::SoundGenerator::mCreature));
            usertypeDefinition.set("sound", sol::readonly_property(&ESM::SoundGenerator::mSound));
            usertypeDefinition.set("type", sol::readonly_property(&ESM::SoundGenerator::mType));

            // Finish up our usertype.
            state.set_usertype("tes3soundGenerator", usertypeDefinition);
        }
    }
}
