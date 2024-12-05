#include "types.hpp"

#include <components/esm3/loadmisc.hpp>
#include <components/esm3/loadspel.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "apps/openmw/mwworld/esmstore.hpp"

namespace MWLua
{

    void addLockableBindings(sol::table lockable)
    {
        lockable["getLockLevel"]
            = [](const Object& object) { return std::abs(object.ptr().getCellRef().getLockLevel()); };
        lockable["isLocked"] = [](const Object& object) { return object.ptr().getCellRef().isLocked(); };
        lockable["getKeyRecord"] = [](const Object& object) -> sol::optional<const ESM::Miscellaneous*> {
            ESM::RefId key = object.ptr().getCellRef().getKey();
            if (key.empty())
                return sol::nullopt;
            return MWBase::Environment::get().getESMStore()->get<ESM::Miscellaneous>().find(key);
        };
        lockable["lock"] = [](const GObject& object, sol::optional<int> lockLevel) {
            object.ptr().getCellRef().setLocked(true);

            int level = 1;

            if (lockLevel)
                level = lockLevel.value();
            else if (object.ptr().getCellRef().getLockLevel() < 0)
                level = -object.ptr().getCellRef().getLockLevel();
            else if (object.ptr().getCellRef().getLockLevel() > 0)
                level = object.ptr().getCellRef().getLockLevel();

            object.ptr().getCellRef().setLockLevel(level);
        };
        lockable["unlock"] = [](const GObject& object) {
            if (!object.ptr().getCellRef().isLocked())
                return;
            object.ptr().getCellRef().setLocked(false);

            object.ptr().getCellRef().setLockLevel(-object.ptr().getCellRef().getLockLevel());
        };
        lockable["setTrapSpell"] = [](const GObject& object, const sol::object& spellOrId) {
            if (spellOrId == sol::nil)
            {
                object.ptr().getCellRef().setTrap(ESM::RefId()); // remove the trap value
                return;
            }
            if (spellOrId.is<ESM::Spell>())
                object.ptr().getCellRef().setTrap(spellOrId.as<const ESM::Spell*>()->mId);
            else
            {
                ESM::RefId spellId = ESM::RefId::deserializeText(LuaUtil::cast<std::string_view>(spellOrId));
                const auto& spellStore = MWBase::Environment::get().getESMStore()->get<ESM::Spell>();
                const ESM::Spell* spell = spellStore.find(spellId);
                object.ptr().getCellRef().setTrap(spell->mId);
            }
        };
        lockable["setKeyRecord"] = [](const GObject& object, const sol::object& itemOrRecordId) {
            if (itemOrRecordId == sol::nil)
            {
                object.ptr().getCellRef().setKey(ESM::RefId()); // remove the trap value
                return;
            }
            if (itemOrRecordId.is<ESM::Miscellaneous>())
                object.ptr().getCellRef().setKey(itemOrRecordId.as<const ESM::Miscellaneous*>()->mId);
            else
            {
                ESM::RefId miscId = ESM::RefId::deserializeText(LuaUtil::cast<std::string_view>(itemOrRecordId));
                const auto& keyStore = MWBase::Environment::get().getESMStore()->get<ESM::Miscellaneous>();
                const ESM::Miscellaneous* key = keyStore.find(miscId);
                object.ptr().getCellRef().setKey(key->mId);
            }
        };
        lockable["getTrapSpell"] = [](sol::this_state lua, const Object& o) -> sol::optional<const ESM::Spell*> {
            ESM::RefId trap = o.ptr().getCellRef().getTrap();
            if (trap.empty())
                return sol::nullopt;
            return MWBase::Environment::get().getESMStore()->get<ESM::Spell>().find(trap);
        };
    }
}
