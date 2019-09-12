#include "book.hpp"

#include "../luamanager.hpp"

#include <components/esm/loadbook.hpp>

namespace MWLua
{
    void bindTES3Book()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::Book>();

        usertypeDefinition.set("new", sol::no_constructor);

        // Basic property binding.
        usertypeDefinition.set("id", &ESM::Book::mId);
        usertypeDefinition.set("mesh", &ESM::Book::mModel);
        usertypeDefinition.set("icon", sol::property(
            [](ESM::Book& self) { return self.mIcon; },
            [](ESM::Book& self, const char* value) { if (strlen(value) < 32) self.mIcon = value; }
        ));
        usertypeDefinition.set("script", &ESM::Book::mScript);
        usertypeDefinition.set("name", &ESM::Book::mName);
        usertypeDefinition.set("enchantment", &ESM::Book::mEnchant);
        usertypeDefinition.set("text", &ESM::Book::mText);

        // Allow object to be converted to strings using their object ID.
        usertypeDefinition.set(sol::meta_function::to_string, &ESM::Book::mId);

        usertypeDefinition.set("weight", sol::property(
            [](ESM::Book& self) { return self.mData.mWeight; },
            [](ESM::Book& self, float value) { self.mData.mWeight = value; }
        ));
        usertypeDefinition.set("value", sol::property(
            [](ESM::Book& self) { return self.mData.mValue; },
            [](ESM::Book& self, int value) { self.mData.mValue = value; }
        ));
        usertypeDefinition.set("skill", sol::property(
            [](ESM::Book& self) { return self.mData.mSkillId; },
            [](ESM::Book& self, int value) { self.mData.mSkillId = value; }
        ));
         usertypeDefinition.set("enchantCapacity", sol::property(
            [](ESM::Book& self) { return self.mData.mEnchant; },
            [](ESM::Book& self, int value) { self.mData.mEnchant = value; }
        ));
        usertypeDefinition.set("isScroll", sol::property(
            [](ESM::Book& self) { return self.mData.mIsScroll > 0; },
            [](ESM::Book& self, bool value) { self.mData.mIsScroll = value; }
        ));

        // Finish up our usertype.
        state.set_usertype("tes3book", usertypeDefinition);
    }
}
