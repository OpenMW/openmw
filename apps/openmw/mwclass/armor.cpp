#include "armor.hpp"

#include <MyGUI_TextIterator.h>
#include <MyGUI_UString.h>

#include <components/esm3/loadarmo.hpp>
#include <components/esm3/loadgmst.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/esm3/loadrace.hpp>
#include <components/esm3/loadskil.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/actionequip.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/ptr.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/weapontype.hpp"
#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

#include "classmodel.hpp"
#include "nameorid.hpp"

namespace MWClass
{
    Armor::Armor()
        : MWWorld::RegisteredClass<Armor>(ESM::Armor::sRecordId)
    {
    }

    void Armor::insertObjectRendering(
        const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty())
        {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    std::string_view Armor::getModel(const MWWorld::ConstPtr& ptr) const
    {
        return getClassModel<ESM::Armor>(ptr);
    }

    std::string_view Armor::getName(const MWWorld::ConstPtr& ptr) const
    {
        return getNameOrId<ESM::Armor>(ptr);
    }

    std::unique_ptr<MWWorld::Action> Armor::activate(const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    bool Armor::hasItemHealth(const MWWorld::ConstPtr& ptr) const
    {
        return true;
    }

    int Armor::getItemMaxHealth(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Armor>* ref = ptr.get<ESM::Armor>();

        return ref->mBase->mData.mHealth;
    }

    ESM::RefId Armor::getScript(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Armor>* ref = ptr.get<ESM::Armor>();

        return ref->mBase->mScript;
    }

    std::pair<std::vector<int>, bool> Armor::getEquipmentSlots(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Armor>* ref = ptr.get<ESM::Armor>();

        std::vector<int> slots_;

        const int size = 11;

        static const int sMapping[size][2] = { { ESM::Armor::Helmet, MWWorld::InventoryStore::Slot_Helmet },
            { ESM::Armor::Cuirass, MWWorld::InventoryStore::Slot_Cuirass },
            { ESM::Armor::LPauldron, MWWorld::InventoryStore::Slot_LeftPauldron },
            { ESM::Armor::RPauldron, MWWorld::InventoryStore::Slot_RightPauldron },
            { ESM::Armor::Greaves, MWWorld::InventoryStore::Slot_Greaves },
            { ESM::Armor::Boots, MWWorld::InventoryStore::Slot_Boots },
            { ESM::Armor::LGauntlet, MWWorld::InventoryStore::Slot_LeftGauntlet },
            { ESM::Armor::RGauntlet, MWWorld::InventoryStore::Slot_RightGauntlet },
            { ESM::Armor::Shield, MWWorld::InventoryStore::Slot_CarriedLeft },
            { ESM::Armor::LBracer, MWWorld::InventoryStore::Slot_LeftGauntlet },
            { ESM::Armor::RBracer, MWWorld::InventoryStore::Slot_RightGauntlet } };

        for (int i = 0; i < size; ++i)
            if (sMapping[i][0] == ref->mBase->mData.mType)
            {
                slots_.push_back(int(sMapping[i][1]));
                break;
            }

        return std::make_pair(slots_, false);
    }

    ESM::RefId Armor::getEquipmentSkill(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Armor>* ref = ptr.get<ESM::Armor>();

        std::string_view typeGmst;

        switch (ref->mBase->mData.mType)
        {
            case ESM::Armor::Helmet:
                typeGmst = "iHelmWeight";
                break;
            case ESM::Armor::Cuirass:
                typeGmst = "iCuirassWeight";
                break;
            case ESM::Armor::LPauldron:
            case ESM::Armor::RPauldron:
                typeGmst = "iPauldronWeight";
                break;
            case ESM::Armor::Greaves:
                typeGmst = "iGreavesWeight";
                break;
            case ESM::Armor::Boots:
                typeGmst = "iBootsWeight";
                break;
            case ESM::Armor::LGauntlet:
            case ESM::Armor::RGauntlet:
                typeGmst = "iGauntletWeight";
                break;
            case ESM::Armor::Shield:
                typeGmst = "iShieldWeight";
                break;
            case ESM::Armor::LBracer:
            case ESM::Armor::RBracer:
                typeGmst = "iGauntletWeight";
                break;
        }

        if (typeGmst.empty())
            return {};

        const MWWorld::Store<ESM::GameSetting>& gmst
            = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();

        float iWeight = floor(gmst.find(typeGmst)->mValue.getFloat());

        float epsilon = 0.0005f;

        if (ref->mBase->mData.mWeight <= iWeight * gmst.find("fLightMaxMod")->mValue.getFloat() + epsilon)
            return ESM::Skill::LightArmor;

        if (ref->mBase->mData.mWeight <= iWeight * gmst.find("fMedMaxMod")->mValue.getFloat() + epsilon)
            return ESM::Skill::MediumArmor;

        else
            return ESM::Skill::HeavyArmor;
    }

    int Armor::getValue(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Armor>* ref = ptr.get<ESM::Armor>();

        return ref->mBase->mData.mValue;
    }

    const ESM::RefId& Armor::getUpSoundId(const MWWorld::ConstPtr& ptr) const
    {
        const ESM::RefId es = getEquipmentSkill(ptr);
        static const ESM::RefId lightUp = ESM::RefId::stringRefId("Item Armor Light Up");
        static const ESM::RefId mediumUp = ESM::RefId::stringRefId("Item Armor Medium Up");
        static const ESM::RefId heavyUp = ESM::RefId::stringRefId("Item Armor Heavy Up");

        if (es == ESM::Skill::LightArmor)
            return lightUp;
        else if (es == ESM::Skill::MediumArmor)
            return mediumUp;
        else
            return heavyUp;
    }

    const ESM::RefId& Armor::getDownSoundId(const MWWorld::ConstPtr& ptr) const
    {
        const ESM::RefId es = getEquipmentSkill(ptr);
        static const ESM::RefId lightDown = ESM::RefId::stringRefId("Item Armor Light Down");
        static const ESM::RefId mediumDown = ESM::RefId::stringRefId("Item Armor Medium Down");
        static const ESM::RefId heavyDown = ESM::RefId::stringRefId("Item Armor Heavy Down");
        if (es == ESM::Skill::LightArmor)
            return lightDown;
        else if (es == ESM::Skill::MediumArmor)
            return mediumDown;
        else
            return heavyDown;
    }

    const std::string& Armor::getInventoryIcon(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Armor>* ref = ptr.get<ESM::Armor>();

        return ref->mBase->mIcon;
    }

    MWGui::ToolTipInfo Armor::getToolTipInfo(const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Armor>* ref = ptr.get<ESM::Armor>();

        MWGui::ToolTipInfo info;
        std::string_view name = getName(ptr);
        info.caption = MyGUI::TextIterator::toTagsString(MyGUI::UString(name)) + MWGui::ToolTips::getCountString(count);
        info.icon = ref->mBase->mIcon;

        std::string text;

        // get armor type string (light/medium/heavy)
        std::string_view typeText;
        if (ref->mBase->mData.mWeight == 0)
        {
            // no type
        }
        else
        {
            const ESM::RefId armorType = getEquipmentSkill(ptr);
            if (armorType == ESM::Skill::LightArmor)
                typeText = "#{sLight}";
            else if (armorType == ESM::Skill::MediumArmor)
                typeText = "#{sMedium}";
            else
                typeText = "#{sHeavy}";
        }

        text += "\n#{sArmorRating}: "
            + MWGui::ToolTips::toString(static_cast<int>(getEffectiveArmorRating(ptr, MWMechanics::getPlayer())));

        int remainingHealth = getItemHealth(ptr);
        text += "\n#{sCondition}: " + MWGui::ToolTips::toString(remainingHealth) + "/"
            + MWGui::ToolTips::toString(ref->mBase->mData.mHealth);

        if (!typeText.empty())
        {
            text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.mWeight) + " (";
            text += typeText;
            text += ')';
        }

        text += MWGui::ToolTips::getValueString(ref->mBase->mData.mValue, "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
        {
            info.extra += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            info.extra += MWGui::ToolTips::getMiscString(ref->mBase->mScript.getRefIdString(), "Script");
        }

        info.enchant = ref->mBase->mEnchant;
        if (!info.enchant.empty())
            info.remainingEnchantCharge = static_cast<int>(ptr.getCellRef().getEnchantmentCharge());

        info.text = std::move(text);

        return info;
    }

    ESM::RefId Armor::getEnchantment(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Armor>* ref = ptr.get<ESM::Armor>();

        return ref->mBase->mEnchant;
    }

    const ESM::RefId& Armor::applyEnchantment(
        const MWWorld::ConstPtr& ptr, const ESM::RefId& enchId, int enchCharge, const std::string& newName) const
    {
        const MWWorld::LiveCellRef<ESM::Armor>* ref = ptr.get<ESM::Armor>();

        ESM::Armor newItem = *ref->mBase;
        newItem.mId = ESM::RefId();
        newItem.mName = newName;
        newItem.mData.mEnchant = enchCharge;
        newItem.mEnchant = enchId;
        const ESM::Armor* record = MWBase::Environment::get().getESMStore()->insert(newItem);
        return record->mId;
    }

    float Armor::getEffectiveArmorRating(const MWWorld::ConstPtr& ptr, const MWWorld::Ptr& actor) const
    {
        const MWWorld::LiveCellRef<ESM::Armor>* ref = ptr.get<ESM::Armor>();

        const ESM::RefId armorSkillType = getEquipmentSkill(ptr);
        float armorSkill = actor.getClass().getSkill(actor, armorSkillType);

        int iBaseArmorSkill = MWBase::Environment::get()
                                  .getESMStore()
                                  ->get<ESM::GameSetting>()
                                  .find("iBaseArmorSkill")
                                  ->mValue.getInteger();

        if (ref->mBase->mData.mWeight == 0)
            return ref->mBase->mData.mArmor;
        else
            return ref->mBase->mData.mArmor * armorSkill / static_cast<float>(iBaseArmorSkill);
    }

    std::pair<int, std::string_view> Armor::canBeEquipped(const MWWorld::ConstPtr& ptr, const MWWorld::Ptr& npc) const
    {
        const MWWorld::InventoryStore& invStore = npc.getClass().getInventoryStore(npc);

        if (getItemHealth(ptr) == 0)
            return { 0, "#{sInventoryMessage1}" };

        // slots that this item can be equipped in
        std::pair<std::vector<int>, bool> slots_ = getEquipmentSlots(ptr);

        if (slots_.first.empty())
            return { 0, {} };

        if (npc.getClass().isNpc())
        {
            const ESM::RefId& npcRace = npc.get<ESM::NPC>()->mBase->mRace;

            // Beast races cannot equip shoes / boots, or full helms (head part vs hair part)
            const ESM::Race* race = MWBase::Environment::get().getESMStore()->get<ESM::Race>().find(npcRace);
            if (race->mData.mFlags & ESM::Race::Beast)
            {
                std::vector<ESM::PartReference> parts = ptr.get<ESM::Armor>()->mBase->mParts.mParts;

                for (std::vector<ESM::PartReference>::iterator itr = parts.begin(); itr != parts.end(); ++itr)
                {
                    if ((*itr).mPart == ESM::PRT_Head)
                        return { 0, "#{sNotifyMessage13}" };
                    if ((*itr).mPart == ESM::PRT_LFoot || (*itr).mPart == ESM::PRT_RFoot)
                        return { 0, "#{sNotifyMessage14}" };
                }
            }
        }

        for (std::vector<int>::const_iterator slot = slots_.first.begin(); slot != slots_.first.end(); ++slot)
        {
            // If equipping a shield, check if there's a twohanded weapon conflicting with it
            if (*slot == MWWorld::InventoryStore::Slot_CarriedLeft)
            {
                MWWorld::ConstContainerStoreIterator weapon
                    = invStore.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
                if (weapon != invStore.end() && weapon->getType() == ESM::Weapon::sRecordId)
                {
                    const MWWorld::LiveCellRef<ESM::Weapon>* ref = weapon->get<ESM::Weapon>();
                    if (MWMechanics::getWeaponType(ref->mBase->mData.mType)->mFlags & ESM::WeaponType::TwoHanded)
                        return { 3, {} };
                }

                return { 1, {} };
            }
        }
        return { 1, {} };
    }

    std::unique_ptr<MWWorld::Action> Armor::use(const MWWorld::Ptr& ptr, bool force) const
    {
        std::unique_ptr<MWWorld::Action> action = std::make_unique<MWWorld::ActionEquip>(ptr, force);

        action->setSound(getUpSoundId(ptr));

        return action;
    }

    MWWorld::Ptr Armor::copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const
    {
        const MWWorld::LiveCellRef<ESM::Armor>* ref = ptr.get<ESM::Armor>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    int Armor::getEnchantmentPoints(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Armor>* ref = ptr.get<ESM::Armor>();

        return ref->mBase->mData.mEnchant;
    }

    bool Armor::canSell(const MWWorld::ConstPtr& item, int npcServices) const
    {
        return (npcServices & ESM::NPC::Armor)
            || ((npcServices & ESM::NPC::MagicItems) && !getEnchantment(item).empty());
    }

    float Armor::getWeight(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Armor>* ref = ptr.get<ESM::Armor>();
        return ref->mBase->mData.mWeight;
    }
}
