#include "weapon.hpp"

#include <MyGUI_TextIterator.h>
#include <MyGUI_UString.h>

#include <components/esm3/loadnpc.hpp>
#include <components/esm3/loadweap.hpp>
#include <components/misc/constants.hpp>
#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/actionequip.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/ptr.hpp"

#include "../mwmechanics/weapontype.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "classmodel.hpp"
#include "nameorid.hpp"

namespace MWClass
{
    Weapon::Weapon()
        : MWWorld::RegisteredClass<Weapon>(ESM::Weapon::sRecordId)
    {
    }

    void Weapon::insertObjectRendering(
        const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty())
        {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    std::string_view Weapon::getModel(const MWWorld::ConstPtr& ptr) const
    {
        return getClassModel<ESM::Weapon>(ptr);
    }

    std::string_view Weapon::getName(const MWWorld::ConstPtr& ptr) const
    {
        return getNameOrId<ESM::Weapon>(ptr);
    }

    std::unique_ptr<MWWorld::Action> Weapon::activate(const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    bool Weapon::hasItemHealth(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon>* ref = ptr.get<ESM::Weapon>();
        int type = ref->mBase->mData.mType;

        return MWMechanics::getWeaponType(type)->mFlags & ESM::WeaponType::HasHealth;
    }

    int Weapon::getItemMaxHealth(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon>* ref = ptr.get<ESM::Weapon>();

        return ref->mBase->mData.mHealth;
    }

    ESM::RefId Weapon::getScript(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon>* ref = ptr.get<ESM::Weapon>();

        return ref->mBase->mScript;
    }

    std::pair<std::vector<int>, bool> Weapon::getEquipmentSlots(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon>* ref = ptr.get<ESM::Weapon>();
        ESM::WeaponType::Class weapClass = MWMechanics::getWeaponType(ref->mBase->mData.mType)->mWeaponClass;

        std::vector<int> slots;
        bool stack = false;

        if (weapClass == ESM::WeaponType::Ammo)
        {
            slots.push_back(int(MWWorld::InventoryStore::Slot_Ammunition));
            stack = true;
        }
        else if (weapClass == ESM::WeaponType::Thrown)
        {
            slots.push_back(int(MWWorld::InventoryStore::Slot_CarriedRight));
            stack = true;
        }
        else
            slots.push_back(int(MWWorld::InventoryStore::Slot_CarriedRight));

        return std::make_pair(slots, stack);
    }

    ESM::RefId Weapon::getEquipmentSkill(const MWWorld::ConstPtr& ptr, bool useLuaInterfaceIfAvailable) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon>* ref = ptr.get<ESM::Weapon>();
        int type = ref->mBase->mData.mType;

        return MWMechanics::getWeaponType(type)->mSkill;
    }

    int Weapon::getValue(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon>* ref = ptr.get<ESM::Weapon>();

        return ref->mBase->mData.mValue;
    }

    const ESM::RefId& Weapon::getUpSoundId(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon>* ref = ptr.get<ESM::Weapon>();
        int type = ref->mBase->mData.mType;
        return MWMechanics::getWeaponType(type)->mSoundIdUp;
    }

    const ESM::RefId& Weapon::getDownSoundId(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon>* ref = ptr.get<ESM::Weapon>();
        int type = ref->mBase->mData.mType;
        return MWMechanics::getWeaponType(type)->mSoundIdDown;
    }

    const std::string& Weapon::getInventoryIcon(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon>* ref = ptr.get<ESM::Weapon>();

        return ref->mBase->mIcon;
    }

    MWGui::ToolTipInfo Weapon::getToolTipInfo(const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon>* ref = ptr.get<ESM::Weapon>();
        const ESM::WeaponType* weaponType = MWMechanics::getWeaponType(ref->mBase->mData.mType);

        MWGui::ToolTipInfo info;
        std::string_view name = getName(ptr);
        info.caption = MyGUI::TextIterator::toTagsString(MyGUI::UString(name)) + MWGui::ToolTips::getCountString(count);
        info.icon = ref->mBase->mIcon;

        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();

        std::string text;

        // weapon type & damage
        if (weaponType->mWeaponClass != ESM::WeaponType::Ammo || Settings::game().mShowProjectileDamage)
        {
            text += "\n#{sType} ";

            const ESM::Skill* skill
                = store.get<ESM::Skill>().find(MWMechanics::getWeaponType(ref->mBase->mData.mType)->mSkill);
            std::string_view oneOrTwoHanded;
            if (weaponType->mWeaponClass == ESM::WeaponType::Melee)
            {
                if (weaponType->mFlags & ESM::WeaponType::TwoHanded)
                    oneOrTwoHanded = "sTwoHanded";
                else
                    oneOrTwoHanded = "sOneHanded";
            }

            text += skill->mName;
            if (!oneOrTwoHanded.empty())
                text += ", " + store.get<ESM::GameSetting>().find(oneOrTwoHanded)->mValue.getString();

            // weapon damage
            if (weaponType->mWeaponClass == ESM::WeaponType::Thrown)
            {
                // Thrown weapons have 2x real damage applied
                // as they're both the weapon and the ammo
                text += "\n#{sAttack}: " + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mChop[0] * 2))
                    + " - " + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mChop[1] * 2));
            }
            else if (weaponType->mWeaponClass == ESM::WeaponType::Melee)
            {
                // Chop
                text += "\n#{sChop}: " + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mChop[0])) + " - "
                    + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mChop[1]));
                // Slash
                text += "\n#{sSlash}: " + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mSlash[0]))
                    + " - " + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mSlash[1]));
                // Thrust
                text += "\n#{sThrust}: " + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mThrust[0]))
                    + " - " + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mThrust[1]));
            }
            else
            {
                // marksman
                text += "\n#{sAttack}: " + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mChop[0]))
                    + " - " + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mChop[1]));
            }
        }

        if (hasItemHealth(ptr))
        {
            int remainingHealth = getItemHealth(ptr);
            text += "\n#{sCondition}: " + MWGui::ToolTips::toString(remainingHealth) + "/"
                + MWGui::ToolTips::toString(ref->mBase->mData.mHealth);
        }

        const bool verbose = Settings::game().mShowMeleeInfo;
        // add reach for melee weapon
        if (weaponType->mWeaponClass == ESM::WeaponType::Melee && verbose)
        {
            // display value in feet
            const float combatDistance
                = store.get<ESM::GameSetting>().find("fCombatDistance")->mValue.getFloat() * ref->mBase->mData.mReach;
            text += MWGui::ToolTips::getWeightString(combatDistance / Constants::UnitsPerFoot, "#{sRange}");
            text += " #{sFeet}";
        }

        // add attack speed for any weapon excepts arrows and bolts
        if (weaponType->mWeaponClass != ESM::WeaponType::Ammo && verbose)
        {
            text += MWGui::ToolTips::getPercentString(ref->mBase->mData.mSpeed, "#{sAttributeSpeed}");
        }

        text += MWGui::ToolTips::getWeightString(ref->mBase->mData.mWeight, "#{sWeight}");
        text += MWGui::ToolTips::getValueString(ref->mBase->mData.mValue, "#{sValue}");

        info.enchant = ref->mBase->mEnchant;

        if (!info.enchant.empty())
            info.remainingEnchantCharge = static_cast<int>(ptr.getCellRef().getEnchantmentCharge());

        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
        {
            info.extra += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            info.extra += MWGui::ToolTips::getMiscString(ref->mBase->mScript.getRefIdString(), "Script");
        }

        info.text = std::move(text);

        return info;
    }

    ESM::RefId Weapon::getEnchantment(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon>* ref = ptr.get<ESM::Weapon>();

        return ref->mBase->mEnchant;
    }

    const ESM::RefId& Weapon::applyEnchantment(
        const MWWorld::ConstPtr& ptr, const ESM::RefId& enchId, int enchCharge, const std::string& newName) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon>* ref = ptr.get<ESM::Weapon>();

        ESM::Weapon newItem = *ref->mBase;
        newItem.mId = ESM::RefId();
        newItem.mName = newName;
        newItem.mData.mEnchant = static_cast<uint16_t>(enchCharge);
        newItem.mEnchant = enchId;
        newItem.mData.mFlags |= ESM::Weapon::Magical;
        const ESM::Weapon* record = MWBase::Environment::get().getESMStore()->insert(newItem);
        return record->mId;
    }

    std::pair<int, std::string_view> Weapon::canBeEquipped(const MWWorld::ConstPtr& ptr, const MWWorld::Ptr& npc) const
    {
        // Do not allow equip weapons from inventory during attack
        if (npc.isInCell() && MWBase::Environment::get().getWindowManager()->isGuiMode()
            && MWBase::Environment::get().getMechanicsManager()->isAttackingOrSpell(npc))
            return { 0, "#{sCantEquipWeapWarning}" };

        if (hasItemHealth(ptr) && getItemHealth(ptr) == 0)
            return { 0, "#{sInventoryMessage1}" };

        std::pair<std::vector<int>, bool> slots = getEquipmentSlots(ptr);

        if (slots.first.empty())
            return { 0, {} };

        int type = ptr.get<ESM::Weapon>()->mBase->mData.mType;
        if (MWMechanics::getWeaponType(type)->mFlags & ESM::WeaponType::TwoHanded)
        {
            return { 2, {} };
        }

        return { 1, {} };
    }

    std::unique_ptr<MWWorld::Action> Weapon::use(const MWWorld::Ptr& ptr, bool force) const
    {
        std::unique_ptr<MWWorld::Action> action = std::make_unique<MWWorld::ActionEquip>(ptr, force);

        action->setSound(getUpSoundId(ptr));

        return action;
    }

    MWWorld::Ptr Weapon::copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon>* ref = ptr.get<ESM::Weapon>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    int Weapon::getEnchantmentPoints(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon>* ref = ptr.get<ESM::Weapon>();

        return ref->mBase->mData.mEnchant;
    }

    bool Weapon::canSell(const MWWorld::ConstPtr& item, int npcServices) const
    {
        return (npcServices & ESM::NPC::Weapon)
            || ((npcServices & ESM::NPC::MagicItems) && !getEnchantment(item).empty());
    }

    float Weapon::getWeight(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon>* ref = ptr.get<ESM::Weapon>();
        return ref->mBase->mData.mWeight;
    }
}
