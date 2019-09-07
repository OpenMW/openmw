#include "weapontype.hpp"

#include "../luamanager.hpp"

#include "../../mwworld/esmstore.hpp"

#include <components/esm/loadweap.hpp>

namespace mwse {
	namespace lua {
		void bindTES3WeaponType() {
			// Get our lua state.
			auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
			sol::state& state = stateHandle.state;

			// Binding for ESM::WeaponType
			{
				// Start our usertype. We must finish this with state.set_usertype.
				auto usertypeDefinition = state.create_simple_usertype<ESM::WeaponType>();

				// Basic property binding.
				usertypeDefinition.set("id", &ESM::WeaponType::mId);
                usertypeDefinition.set("shortGroup", &ESM::WeaponType::mShortGroup);
                usertypeDefinition.set("longGroup", &ESM::WeaponType::mLongGroup);
                usertypeDefinition.set("sound", &ESM::WeaponType::mSoundId);
                usertypeDefinition.set("attachBone", &ESM::WeaponType::mAttachBone);
                usertypeDefinition.set("sheathBone", &ESM::WeaponType::mSheathingBone);
                usertypeDefinition.set("skill", &ESM::WeaponType::mSkill);
                usertypeDefinition.set("class", &ESM::WeaponType::mWeaponClass);
                usertypeDefinition.set("ammoType", &ESM::WeaponType::mAmmoType);
                usertypeDefinition.set("flags", &ESM::WeaponType::mFlags);

				usertypeDefinition.set("twoHanded", sol::property(
					[](ESM::WeaponType& self) { return (self.mFlags & ESM::WeaponType::TwoHanded) != 0; },
					[](ESM::WeaponType& self, bool set) { set ? self.mFlags |= ESM::WeaponType::TwoHanded : self.mFlags &= ~ESM::WeaponType::TwoHanded; }
				));
				usertypeDefinition.set("hasHealth", sol::property(
					[](ESM::WeaponType& self) { return (self.mFlags & ESM::WeaponType::HasHealth) != 0; },
					[](ESM::WeaponType& self, bool set) { set ? self.mFlags |= ESM::WeaponType::HasHealth : self.mFlags &= ~ESM::WeaponType::HasHealth; }
				));

				// Finish up our usertype.
				state.set_usertype("tes3weapontype", usertypeDefinition);
			}
		}
	}
}
