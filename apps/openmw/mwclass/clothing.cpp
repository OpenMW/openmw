
#include "clothing.hpp"

#include <components/esm/loadclot.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/actionequip.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/nullaction.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    void Clothing::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        const std::string model = getModel(ptr);
        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    void Clothing::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty())
            physics.addObject(ptr,true);
    }

    std::string Clothing::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Clothing::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        return ref->mBase->mName;
    }

    boost::shared_ptr<MWWorld::Action> Clothing::activate (const MWWorld::Ptr& ptr,
            const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    std::string Clothing::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        return ref->mBase->mScript;
    }

    std::pair<std::vector<int>, bool> Clothing::getEquipmentSlots (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        std::vector<int> slots_;

        if (ref->mBase->mData.mType==ESM::Clothing::Ring)
        {
            slots_.push_back (int (MWWorld::InventoryStore::Slot_LeftRing));
            slots_.push_back (int (MWWorld::InventoryStore::Slot_RightRing));
        }
        else
        {
            const int size = 9;

            static const int sMapping[size][2] =
            {
                { ESM::Clothing::Shirt, MWWorld::InventoryStore::Slot_Shirt },
                { ESM::Clothing::Belt, MWWorld::InventoryStore::Slot_Belt },
                { ESM::Clothing::Robe, MWWorld::InventoryStore::Slot_Robe },
                { ESM::Clothing::Pants, MWWorld::InventoryStore::Slot_Pants },
                { ESM::Clothing::Shoes, MWWorld::InventoryStore::Slot_Boots },
                { ESM::Clothing::LGlove, MWWorld::InventoryStore::Slot_LeftGauntlet },
                { ESM::Clothing::RGlove, MWWorld::InventoryStore::Slot_RightGauntlet },
                { ESM::Clothing::Skirt, MWWorld::InventoryStore::Slot_Skirt },
                { ESM::Clothing::Amulet, MWWorld::InventoryStore::Slot_Amulet }
            };

            for (int i=0; i<size; ++i)
                if (sMapping[i][0]==ref->mBase->mData.mType)
                {
                    slots_.push_back (int (sMapping[i][1]));
                    break;
                }
        }

        return std::make_pair (slots_, false);
    }

    int Clothing::getEquipmentSkill (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        if (ref->mBase->mData.mType==ESM::Clothing::Shoes)
            return ESM::Skill::Unarmored;

        return -1;
    }

    int Clothing::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        return ref->mBase->mData.mValue;
    }

    void Clothing::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Clothing);

        registerClass (typeid (ESM::Clothing).name(), instance);
    }

    std::string Clothing::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
         MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        if (ref->mBase->mData.mType == 8)
        {
            return std::string("Item Ring Up");
        }
        return std::string("Item Clothes Up");
    }

    std::string Clothing::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
         MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        if (ref->mBase->mData.mType == 8)
        {
            return std::string("Item Ring Down");
        }
        return std::string("Item Clothes Down");
    }

    std::string Clothing::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
          MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        return ref->mBase->mIcon;
    }

    bool Clothing::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        return (ref->mBase->mName != "");
    }

    MWGui::ToolTipInfo Clothing::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        info.icon = ref->mBase->mIcon;

        std::string text;

        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.mWeight);
        text += MWGui::ToolTips::getValueString(getValue(ptr), "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getMiscString(ref->mRef.mOwner, "Owner");
            text += MWGui::ToolTips::getMiscString(ref->mRef.mFaction, "Faction");
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }

        info.enchant = ref->mBase->mEnchant;
        if (!info.enchant.empty())
            info.remainingEnchantCharge = ptr.getCellRef().mEnchantmentCharge;

        info.text = text;

        return info;
    }

    std::string Clothing::getEnchantment (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        return ref->mBase->mEnchant;
    }

    void Clothing::applyEnchantment(const MWWorld::Ptr &ptr, const std::string& enchId, int enchCharge, const std::string& newName) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        ESM::Clothing newItem = *ref->mBase;
        newItem.mId="";
        newItem.mName=newName;
        newItem.mData.mEnchant=enchCharge;
        newItem.mEnchant=enchId;
        const ESM::Clothing *record = MWBase::Environment::get().getWorld()->createRecord (newItem);
        ref->mBase = record;
        ref->mRef.mRefID = record->mId;
    }

    std::pair<int, std::string> Clothing::canBeEquipped(const MWWorld::Ptr &ptr, const MWWorld::Ptr &npc) const
    {
        // slots that this item can be equipped in
        std::pair<std::vector<int>, bool> slots_ = MWWorld::Class::get(ptr).getEquipmentSlots(ptr);

        if (slots_.first.empty())
            return std::make_pair(0, "");

        if (npc.getClass().isNpc())
        {
            std::string npcRace = npc.get<ESM::NPC>()->mBase->mRace;

            // Beast races cannot equip shoes / boots, or full helms (head part vs hair part)
            const ESM::Race* race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(npcRace);
            if(race->mData.mFlags & ESM::Race::Beast)
            {
                std::vector<ESM::PartReference> parts = ptr.get<ESM::Clothing>()->mBase->mParts.mParts;

                for(std::vector<ESM::PartReference>::iterator itr = parts.begin(); itr != parts.end(); ++itr)
                {
                    if((*itr).mPart == ESM::PRT_Head)
                        return std::make_pair(0, "#{sNotifyMessage13}");
                    if((*itr).mPart == ESM::PRT_LFoot || (*itr).mPart == ESM::PRT_RFoot)
                        return std::make_pair(0, "#{sNotifyMessage15}");
                }
            }
        }

        return std::make_pair (1, "");
    }

    boost::shared_ptr<MWWorld::Action> Clothing::use (const MWWorld::Ptr& ptr) const
    {
        boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionEquip(ptr));

        action->setSound(getUpSoundId(ptr));

        return action;
    }

    MWWorld::Ptr
    Clothing::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        return MWWorld::Ptr(&cell.get<ESM::Clothing>().insert(*ref), &cell);
    }

    int Clothing::getEnchantmentPoints (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
                ptr.get<ESM::Clothing>();

        return ref->mBase->mData.mEnchant;
    }

    bool Clothing::canSell (const MWWorld::Ptr& item, int npcServices) const
    {
        return npcServices & ESM::NPC::Clothing;
    }

    float Clothing::getWeight(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();
        return ref->mBase->mData.mWeight;
    }
}
