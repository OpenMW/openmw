#include "race.hpp"

#include <osg/Vec4f>

#include "../luamanager.hpp"

#include <components/esm/loadrace.hpp>

namespace MWLua
{
    void bindTES3Race()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Binding for ESM::Race::SkillBonus.
        {
            // Start our usertype. We must finish this with state.set_usertype.
            auto usertypeDefinition = state.create_simple_usertype<ESM::Race::SkillBonus>();
            usertypeDefinition.set("new", sol::no_constructor);

            // Basic property binding.
            usertypeDefinition.set("skill", &ESM::Race::SkillBonus::mSkill);
            usertypeDefinition.set("bonus", &ESM::Race::SkillBonus::mBonus);

            // Finish up our usertype.
            state.set_usertype("tes3raceSkillBonus", usertypeDefinition);
        }

        // Binding for ESM::Race::MaleFemale.
        {
            // Start our usertype. We must finish this with state.set_usertype.
            auto usertypeDefinition = state.create_simple_usertype<ESM::Race::MaleFemale>();
            usertypeDefinition.set("new", sol::no_constructor);

            // Basic property binding.
            usertypeDefinition.set("male", &ESM::Race::MaleFemale::mMale);
            usertypeDefinition.set("female", &ESM::Race::MaleFemale::mFemale);

            // Finish up our usertype.
            state.set_usertype("tes3raceBaseAttribute", usertypeDefinition);
        }

        // Binding for ESM::Race::MaleFemaleF.
        {
            // Start our usertype. We must finish this with state.set_usertype.
            auto usertypeDefinition = state.create_simple_usertype<ESM::Race::MaleFemaleF>();
            usertypeDefinition.set("new", sol::no_constructor);

            // Basic property binding.
            usertypeDefinition.set("male", &ESM::Race::MaleFemaleF::mMale);
            usertypeDefinition.set("female", &ESM::Race::MaleFemaleF::mFemale);

            // Finish up our usertype.
            state.set_usertype("tes3raceHeightWeight", usertypeDefinition);
        }

        // We do not keep it here.
        /*
        // Binding for TES3::Race::BodyParts.
        {
            // Start our usertype. We must finish this with state.set_usertype.
            auto usertypeDefinition = state.create_simple_usertype<TES3::Race::BodyParts>();
            usertypeDefinition.set("new", sol::no_constructor);

            // Basic property binding.
            usertypeDefinition.set("head1", &TES3::Race::BodyParts::head1);
            usertypeDefinition.set("hair", &TES3::Race::BodyParts::hair);
            usertypeDefinition.set("neck", &TES3::Race::BodyParts::neck);
            usertypeDefinition.set("chest", &TES3::Race::BodyParts::chest);
            usertypeDefinition.set("groin", &TES3::Race::BodyParts::groin);
            usertypeDefinition.set("hands", &TES3::Race::BodyParts::hands);
            usertypeDefinition.set("wrist", &TES3::Race::BodyParts::wrist);
            usertypeDefinition.set("forearm", &TES3::Race::BodyParts::forearm);
            usertypeDefinition.set("upperArm", &TES3::Race::BodyParts::upperArm);
            usertypeDefinition.set("foot", &TES3::Race::BodyParts::foot);
            usertypeDefinition.set("ankle", &TES3::Race::BodyParts::ankle);
            usertypeDefinition.set("knee", &TES3::Race::BodyParts::knee);
            usertypeDefinition.set("upperLeg", &TES3::Race::BodyParts::upperLeg);
            usertypeDefinition.set("clavicle", &TES3::Race::BodyParts::clavicle);
            usertypeDefinition.set("tail", &TES3::Race::BodyParts::tail);
            usertypeDefinition.set("head2", &TES3::Race::BodyParts::head2);

            // Finish up our usertype.
            state.set_usertype("tes3raceBodyParts", usertypeDefinition);
        }
        */

        // Binding for TES3::Race.
        {
            // Start our usertype. We must finish this with state.set_usertype.
            auto usertypeDefinition = state.create_simple_usertype<ESM::Race>();
            usertypeDefinition.set("new", sol::no_constructor);

            usertypeDefinition.set("id", sol::readonly_property([](ESM::Race& self) { return self.mId; }));
            usertypeDefinition.set("description", &ESM::Race::mDescription);

            // Basic property binding.
            /*
            usertypeDefinition.set("femaleBody", sol::readonly_property(&TES3::Race::femaleBody));
            usertypeDefinition.set("maleBody", sol::readonly_property(&TES3::Race::maleBody));
            */
            usertypeDefinition.set("height", sol::readonly_property([](ESM::Race& self) { return self.mData.mHeight; }));
            usertypeDefinition.set("weight", sol::readonly_property([](ESM::Race& self) { return self.mData.mWeight; }));

            // Indirect bindings to unions and arrays.
            usertypeDefinition.set("baseAttributes", sol::readonly_property([](ESM::Race& self) { return std::ref(self.mData.mAttributeValues); }));
            usertypeDefinition.set("skillBonuses", sol::readonly_property([](ESM::Race& self) { return std::ref(self.mData.mBonus); }));

            // Functions exposed as properties.
            usertypeDefinition.set("name", sol::readonly_property([](ESM::Race& self) { return self.mName; }));

            // Allow access to flags.
            usertypeDefinition.set("flags", sol::property(
                [](ESM::Race& self) { return self.mData.mFlags; },
                [](ESM::Race& self, int value) { self.mData.mFlags = value; }
            ));

            // User-friendly access to those flags.
            usertypeDefinition.set("playable", sol::property(
                [](ESM::Race& self) { return (self.mData.mFlags & ESM::Race::Playable) != 0; },
                [](ESM::Race& self, bool set) { set ? self.mData.mFlags |= ESM::Race::Playable : self.mData.mFlags &= ~ESM::Race::Playable; }
            ));
            usertypeDefinition.set("beast", sol::property(
                [](ESM::Race& self) { return (self.mData.mFlags & ESM::Race::Beast) != 0; },
                [](ESM::Race& self, bool set) { set ? self.mData.mFlags |= ESM::Race::Beast : self.mData.mFlags &= ~ESM::Race::Beast; }
            ));

            // Indirect bindings to unions and arrays.
            usertypeDefinition.set("powers", sol::readonly_property([](ESM::Race& self) { return self.mPowers.mList; }));

            // Finish up our usertype.
            state.set_usertype("tes3race", usertypeDefinition);
        }
    }
}
