#include "weapon.hpp"

#include <components/esm/loadweap.hpp>
#include <components/misc/constants.hpp>
#include <components/settings/settings.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actionequip.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwphysics/physicssystem.hpp"
#include "../mwworld/nullaction.hpp"

#include "../mwmechanics/weapontype.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{

    void Weapon::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    void Weapon::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWPhysics::PhysicsSystem& physics) const
    {
        // TODO: add option somewhere to enable collision for placeable objects
    }

    std::string Weapon::getModel(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon> *ref = ptr.get<ESM::Weapon>();

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Weapon::getName (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon> *ref = ptr.get<ESM::Weapon>();
        const std::string& name = ref->mBase->mName;

        return !name.empty() ? name : ref->mBase->mId;
    }

    std::shared_ptr<MWWorld::Action> Weapon::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    bool Weapon::hasItemHealth (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon> *ref = ptr.get<ESM::Weapon>();
        int type = ref->mBase->mData.mType;

        return MWMechanics::getWeaponType(type)->mFlags & ESM::WeaponType::HasHealth;
    }

    int Weapon::getItemMaxHealth (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon> *ref = ptr.get<ESM::Weapon>();

        return ref->mBase->mData.mHealth;
    }

    std::string Weapon::getScript (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon> *ref =
            ptr.get<ESM::Weapon>();

        return ref->mBase->mScript;
    }

    std::pair<std::vector<int>, bool> Weapon::getEquipmentSlots (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon> *ref = ptr.get<ESM::Weapon>();
        ESM::WeaponType::Class weapClass = MWMechanics::getWeaponType(ref->mBase->mData.mType)->mWeaponClass;

        std::vector<int> slots_;
        bool stack = false;

        if (weapClass == ESM::WeaponType::Ammo)
        {
            slots_.push_back (int (MWWorld::InventoryStore::Slot_Ammunition));
            stack = true;
        }
        else if (weapClass == ESM::WeaponType::Thrown)
        {
            slots_.push_back (int (MWWorld::InventoryStore::Slot_CarriedRight));
            stack = true;
        }
        else
            slots_.push_back (int (MWWorld::InventoryStore::Slot_CarriedRight));

        return std::make_pair (slots_, stack);
    }

    int Weapon::getEquipmentSkill (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon> *ref = ptr.get<ESM::Weapon>();
        int type = ref->mBase->mData.mType;

        return MWMechanics::getWeaponType(type)->mSkill;
    }

    int Weapon::getValue (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon> *ref = ptr.get<ESM::Weapon>();

        return ref->mBase->mData.mValue;
    }

    void Weapon::registerSelf()
    {
        std::shared_ptr<Class> instance (new Weapon);

        registerClass (typeid (ESM::Weapon).name(), instance);
    }

    std::string Weapon::getUpSoundId (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon> *ref = ptr.get<ESM::Weapon>();
        int type = ref->mBase->mData.mType;
        std::string soundId = MWMechanics::getWeaponType(type)->mSoundId;
        return soundId + " Up";
    }

    std::string Weapon::getDownSoundId (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon> *ref = ptr.get<ESM::Weapon>();
        int type = ref->mBase->mData.mType;
        std::string soundId = MWMechanics::getWeaponType(type)->mSoundId;
        return soundId + " Down";
    }

    std::string Weapon::getInventoryIcon (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon> *ref = ptr.get<ESM::Weapon>();

        return ref->mBase->mIcon;
    }

    MWGui::ToolTipInfo Weapon::getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon> *ref = ptr.get<ESM::Weapon>();
        const ESM::WeaponType* weaponType = MWMechanics::getWeaponType(ref->mBase->mData.mType);

        MWGui::ToolTipInfo info;
        info.caption = MyGUI::TextIterator::toTagsString(getName(ptr)) + MWGui::ToolTips::getCountString(count);
        info.icon = ref->mBase->mIcon;

        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        std::string text;

        // weapon type & damage
        if (weaponType->mWeaponClass != ESM::WeaponType::Ammo || Settings::Manager::getBool("show projectile damage", "Game"))
        {
            text += "\n#{sType} ";

            int skill = MWMechanics::getWeaponType(ref->mBase->mData.mType)->mSkill;
            const std::string type = ESM::Skill::sSkillNameIds[skill];
            std::string oneOrTwoHanded;
            if (weaponType->mWeaponClass == ESM::WeaponType::Melee)
            {
                if (weaponType->mFlags & ESM::WeaponType::TwoHanded)
                    oneOrTwoHanded = "sTwoHanded";
                else
                    oneOrTwoHanded = "sOneHanded";
            }

            text += store.get<ESM::GameSetting>().find(type)->mValue.getString() +
                ((oneOrTwoHanded != "") ? ", " + store.get<ESM::GameSetting>().find(oneOrTwoHanded)->mValue.getString() : "");

            // weapon damage
            if (weaponType->mWeaponClass == ESM::WeaponType::Thrown)
            {
                // Thrown weapons have 2x real damage applied
                // as they're both the weapon and the ammo
                text += "\n#{sAttack}: "
                    + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mChop[0] * 2))
                    + " - " + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mChop[1] * 2));
            }
            else if (weaponType->mWeaponClass == ESM::WeaponType::Melee)
            {
                // Chop
                text += "\n#{sChop}: "
                    + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mChop[0]))
                    + " - " + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mChop[1]));
                // Slash
                text += "\n#{sSlash}: "
                    + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mSlash[0]))
                    + " - " + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mSlash[1]));
                // Thrust
                text += "\n#{sThrust}: "
                    + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mThrust[0]))
                    + " - " + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mThrust[1]));
            }
            else
            {
                // marksman
                text += "\n#{sAttack}: "
                    + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mChop[0]))
                    + " - " + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mChop[1]));
            }
        }

        if (hasItemHealth(ptr))
        {
            int remainingHealth = getItemHealth(ptr);
            text += "\n#{sCondition}: " + MWGui::ToolTips::toString(remainingHealth) + "/"
                    + MWGui::ToolTips::toString(ref->mBase->mData.mHealth);
        }

        const bool verbose = Settings::Manager::getBool("show melee info", "Game");
        // add reach for melee weapon
        if (weaponType->mWeaponClass == ESM::WeaponType::Melee && verbose)
        {
            // display value in feet
            const float combatDistance = store.get<ESM::GameSetting>().find("fCombatDistance")->mValue.getFloat() * ref->mBase->mData.mReach;
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

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }

        info.text = text;

        return info;
    }

    std::string Weapon::getEnchantment (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon> *ref = ptr.get<ESM::Weapon>();

        return ref->mBase->mEnchant;
    }

    std::string Weapon::applyEnchantment(const MWWorld::ConstPtr &ptr, const std::string& enchId, int enchCharge, const std::string& newName) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon> *ref = ptr.get<ESM::Weapon>();

        ESM::Weapon newItem = *ref->mBase;
        newItem.mId="";
        newItem.mName=newName;
        newItem.mData.mEnchant=enchCharge;
        newItem.mEnchant=enchId;
        newItem.mData.mFlags |= ESM::Weapon::Magical;
        const ESM::Weapon *record = MWBase::Environment::get().getWorld()->createRecord (newItem);
        return record->mId;
    }

    std::pair<int, std::string> Weapon::canBeEquipped(const MWWorld::ConstPtr &ptr, const MWWorld::Ptr &npc) const
    {
        if (hasItemHealth(ptr) && getItemHealth(ptr) == 0)
            return std::make_pair(0, "#{sInventoryMessage1}");

        // Do not allow equip weapons from inventory during attack
        if (MWBase::Environment::get().getMechanicsManager()->isAttackingOrSpell(npc)
            && MWBase::Environment::get().getWindowManager()->isGuiMode())
            return std::make_pair(0, "#{sCantEquipWeapWarning}");

        std::pair<std::vector<int>, bool> slots_ = getEquipmentSlots(ptr);

        if (slots_.first.empty())
            return std::make_pair (0, "");

        int type = ptr.get<ESM::Weapon>()->mBase->mData.mType;
        if(MWMechanics::getWeaponType(type)->mFlags & ESM::WeaponType::TwoHanded)
        {
            return std::make_pair (2, "");
        }

        return std::make_pair(1, "");
    }

    std::shared_ptr<MWWorld::Action> Weapon::use (const MWWorld::Ptr& ptr, bool force) const
    {
        std::shared_ptr<MWWorld::Action> action(new MWWorld::ActionEquip(ptr, force));

        action->setSound(getUpSoundId(ptr));

        return action;
    }

    MWWorld::Ptr Weapon::copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon> *ref = ptr.get<ESM::Weapon>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    int Weapon::getEnchantmentPoints (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon> *ref = ptr.get<ESM::Weapon>();

        return ref->mBase->mData.mEnchant;
    }

    bool Weapon::canSell (const MWWorld::ConstPtr& item, int npcServices) const
    {
        return (npcServices & ESM::NPC::Weapon)
                || ((npcServices & ESM::NPC::MagicItems) && !getEnchantment(item).empty());
    }

    float Weapon::getWeight(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Weapon> *ref = ptr.get<ESM::Weapon>();
        return ref->mBase->mData.mWeight;
    }
}
