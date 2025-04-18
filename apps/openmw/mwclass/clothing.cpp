#include "clothing.hpp"

#include <MyGUI_TextIterator.h>
#include <MyGUI_UString.h>

#include <components/esm3/loadclot.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/esm3/loadrace.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/actionequip.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/ptr.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "classmodel.hpp"
#include "nameorid.hpp"

namespace MWClass
{
    Clothing::Clothing()
        : MWWorld::RegisteredClass<Clothing>(ESM::Clothing::sRecordId)
    {
    }

    void Clothing::insertObjectRendering(
        const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty())
        {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    std::string_view Clothing::getModel(const MWWorld::ConstPtr& ptr) const
    {
        return getClassModel<ESM::Clothing>(ptr);
    }

    std::string_view Clothing::getName(const MWWorld::ConstPtr& ptr) const
    {
        return getNameOrId<ESM::Clothing>(ptr);
    }

    std::unique_ptr<MWWorld::Action> Clothing::activate(const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    ESM::RefId Clothing::getScript(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing>* ref = ptr.get<ESM::Clothing>();

        return ref->mBase->mScript;
    }

    std::pair<std::vector<int>, bool> Clothing::getEquipmentSlots(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing>* ref = ptr.get<ESM::Clothing>();

        std::vector<int> slots;

        if (ref->mBase->mData.mType == ESM::Clothing::Ring)
        {
            slots.push_back(int(MWWorld::InventoryStore::Slot_LeftRing));
            slots.push_back(int(MWWorld::InventoryStore::Slot_RightRing));
        }
        else
        {
            const int size = 9;

            static const int sMapping[size][2] = { { ESM::Clothing::Shirt, MWWorld::InventoryStore::Slot_Shirt },
                { ESM::Clothing::Belt, MWWorld::InventoryStore::Slot_Belt },
                { ESM::Clothing::Robe, MWWorld::InventoryStore::Slot_Robe },
                { ESM::Clothing::Pants, MWWorld::InventoryStore::Slot_Pants },
                { ESM::Clothing::Shoes, MWWorld::InventoryStore::Slot_Boots },
                { ESM::Clothing::LGlove, MWWorld::InventoryStore::Slot_LeftGauntlet },
                { ESM::Clothing::RGlove, MWWorld::InventoryStore::Slot_RightGauntlet },
                { ESM::Clothing::Skirt, MWWorld::InventoryStore::Slot_Skirt },
                { ESM::Clothing::Amulet, MWWorld::InventoryStore::Slot_Amulet } };

            for (int i = 0; i < size; ++i)
                if (sMapping[i][0] == ref->mBase->mData.mType)
                {
                    slots.push_back(int(sMapping[i][1]));
                    break;
                }
        }

        return std::make_pair(slots, false);
    }

    ESM::RefId Clothing::getEquipmentSkill(const MWWorld::ConstPtr& ptr, bool useLuaInterfaceIfAvailable) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing>* ref = ptr.get<ESM::Clothing>();

        if (ref->mBase->mData.mType == ESM::Clothing::Shoes)
            return ESM::Skill::Unarmored;

        return {};
    }

    int Clothing::getValue(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing>* ref = ptr.get<ESM::Clothing>();

        return ref->mBase->mData.mValue;
    }

    const ESM::RefId& Clothing::getUpSoundId(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing>* ref = ptr.get<ESM::Clothing>();
        static const ESM::RefId ringUp = ESM::RefId::stringRefId("Item Ring Up");
        static const ESM::RefId clothsUp = ESM::RefId::stringRefId("Item Clothes Up");
        if (ref->mBase->mData.mType == ESM::Clothing::Ring)
        {
            return ringUp;
        }
        return clothsUp;
    }

    const ESM::RefId& Clothing::getDownSoundId(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing>* ref = ptr.get<ESM::Clothing>();
        static const ESM::RefId ringDown = ESM::RefId::stringRefId("Item Ring Down");
        static const ESM::RefId clothsDown = ESM::RefId::stringRefId("Item Clothes Down");
        if (ref->mBase->mData.mType == ESM::Clothing::Ring)
        {
            return ringDown;
        }
        return clothsDown;
    }

    const std::string& Clothing::getInventoryIcon(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing>* ref = ptr.get<ESM::Clothing>();

        return ref->mBase->mIcon;
    }

    MWGui::ToolTipInfo Clothing::getToolTipInfo(const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing>* ref = ptr.get<ESM::Clothing>();

        MWGui::ToolTipInfo info;
        std::string_view name = getName(ptr);
        info.caption = MyGUI::TextIterator::toTagsString(MyGUI::UString(name)) + MWGui::ToolTips::getCountString(count);
        info.icon = ref->mBase->mIcon;

        std::string text;

        text += MWGui::ToolTips::getWeightString(ref->mBase->mData.mWeight, "#{sWeight}");
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

    ESM::RefId Clothing::getEnchantment(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing>* ref = ptr.get<ESM::Clothing>();

        return ref->mBase->mEnchant;
    }

    const ESM::RefId& Clothing::applyEnchantment(
        const MWWorld::ConstPtr& ptr, const ESM::RefId& enchId, int enchCharge, const std::string& newName) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing>* ref = ptr.get<ESM::Clothing>();

        ESM::Clothing newItem = *ref->mBase;
        newItem.mId = ESM::RefId();
        newItem.mName = newName;
        newItem.mData.mEnchant = enchCharge;
        newItem.mEnchant = enchId;
        const ESM::Clothing* record = MWBase::Environment::get().getESMStore()->insert(newItem);
        return record->mId;
    }

    std::pair<int, std::string_view> Clothing::canBeEquipped(
        const MWWorld::ConstPtr& ptr, const MWWorld::Ptr& npc) const
    {
        // slots that this item can be equipped in
        std::pair<std::vector<int>, bool> slots = getEquipmentSlots(ptr);

        if (slots.first.empty())
            return { 0, {} };

        if (npc.getClass().isNpc())
        {
            const ESM::RefId& npcRace = npc.get<ESM::NPC>()->mBase->mRace;

            // Beast races cannot equip shoes / boots, or full helms (head part vs hair part)
            const ESM::Race* race = MWBase::Environment::get().getESMStore()->get<ESM::Race>().find(npcRace);
            if (race->mData.mFlags & ESM::Race::Beast)
            {
                std::vector<ESM::PartReference> parts = ptr.get<ESM::Clothing>()->mBase->mParts.mParts;

                for (std::vector<ESM::PartReference>::iterator itr = parts.begin(); itr != parts.end(); ++itr)
                {
                    if ((*itr).mPart == ESM::PRT_Head)
                        return { 0, "#{sNotifyMessage13}" };
                    if ((*itr).mPart == ESM::PRT_LFoot || (*itr).mPart == ESM::PRT_RFoot)
                        return { 0, "#{sNotifyMessage15}" };
                }
            }
        }

        return { 1, {} };
    }

    std::unique_ptr<MWWorld::Action> Clothing::use(const MWWorld::Ptr& ptr, bool force) const
    {
        std::unique_ptr<MWWorld::Action> action = std::make_unique<MWWorld::ActionEquip>(ptr, force);

        action->setSound(getUpSoundId(ptr));

        return action;
    }

    MWWorld::Ptr Clothing::copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing>* ref = ptr.get<ESM::Clothing>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    int Clothing::getEnchantmentPoints(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing>* ref = ptr.get<ESM::Clothing>();

        return ref->mBase->mData.mEnchant;
    }

    bool Clothing::canSell(const MWWorld::ConstPtr& item, int npcServices) const
    {
        return (npcServices & ESM::NPC::Clothing)
            || ((npcServices & ESM::NPC::MagicItems) && !getEnchantment(item).empty());
    }

    float Clothing::getWeight(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing>* ref = ptr.get<ESM::Clothing>();
        return ref->mBase->mData.mWeight;
    }
}
