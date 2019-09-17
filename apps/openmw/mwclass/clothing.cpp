#include "clothing.hpp"

#include <components/esm/loadclot.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actionequip.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwphysics/physicssystem.hpp"
#include "../mwworld/nullaction.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{

    void Clothing::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    void Clothing::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWPhysics::PhysicsSystem& physics) const
    {
        // TODO: add option somewhere to enable collision for placeable objects
    }

    std::string Clothing::getModel(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing> *ref = ptr.get<ESM::Clothing>();

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Clothing::getName (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing> *ref = ptr.get<ESM::Clothing>();
        const std::string& name = ref->mBase->mName;

        return !name.empty() ? name : ref->mBase->mId;
    }

    std::shared_ptr<MWWorld::Action> Clothing::activate (const MWWorld::Ptr& ptr,
            const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    std::string Clothing::getScript (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing> *ref = ptr.get<ESM::Clothing>();

        return ref->mBase->mScript;
    }

    std::pair<std::vector<int>, bool> Clothing::getEquipmentSlots (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing> *ref = ptr.get<ESM::Clothing>();

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

    int Clothing::getEquipmentSkill (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing> *ref = ptr.get<ESM::Clothing>();

        if (ref->mBase->mData.mType==ESM::Clothing::Shoes)
            return ESM::Skill::Unarmored;

        return -1;
    }

    int Clothing::getValue (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing> *ref = ptr.get<ESM::Clothing>();

        return ref->mBase->mData.mValue;
    }

    void Clothing::registerSelf()
    {
        std::shared_ptr<Class> instance (new Clothing);

        registerClass (typeid (ESM::Clothing).name(), instance);
    }

    std::string Clothing::getUpSoundId (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing> *ref = ptr.get<ESM::Clothing>();

        if (ref->mBase->mData.mType == 8)
        {
            return std::string("Item Ring Up");
        }
        return std::string("Item Clothes Up");
    }

    std::string Clothing::getDownSoundId (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing> *ref = ptr.get<ESM::Clothing>();

        if (ref->mBase->mData.mType == 8)
        {
            return std::string("Item Ring Down");
        }
        return std::string("Item Clothes Down");
    }

    std::string Clothing::getInventoryIcon (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing> *ref = ptr.get<ESM::Clothing>();

        return ref->mBase->mIcon;
    }

    MWGui::ToolTipInfo Clothing::getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing> *ref = ptr.get<ESM::Clothing>();

        MWGui::ToolTipInfo info;
        info.caption = MyGUI::TextIterator::toTagsString(getName(ptr)) + MWGui::ToolTips::getCountString(count);
        info.icon = ref->mBase->mIcon;

        std::string text;

        text += MWGui::ToolTips::getWeightString(ref->mBase->mData.mWeight, "#{sWeight}");
        text += MWGui::ToolTips::getValueString(ref->mBase->mData.mValue, "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }

        info.enchant = ref->mBase->mEnchant;
        if (!info.enchant.empty())
            info.remainingEnchantCharge = static_cast<int>(ptr.getCellRef().getEnchantmentCharge());

        info.text = text;

        return info;
    }

    std::string Clothing::getEnchantment (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing> *ref = ptr.get<ESM::Clothing>();

        return ref->mBase->mEnchant;
    }

    std::string Clothing::applyEnchantment(const MWWorld::ConstPtr &ptr, const std::string& enchId, int enchCharge, const std::string& newName) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing> *ref = ptr.get<ESM::Clothing>();

        ESM::Clothing newItem = *ref->mBase;
        newItem.mId="";
        newItem.mName=newName;
        newItem.mData.mEnchant=enchCharge;
        newItem.mEnchant=enchId;
        const ESM::Clothing *record = MWBase::Environment::get().getWorld()->createRecord (newItem);
        return record->mId;
    }

    std::pair<int, std::string> Clothing::canBeEquipped(const MWWorld::ConstPtr &ptr, const MWWorld::Ptr &npc) const
    {
        // slots that this item can be equipped in
        std::pair<std::vector<int>, bool> slots_ = getEquipmentSlots(ptr);

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

    std::shared_ptr<MWWorld::Action> Clothing::use (const MWWorld::Ptr& ptr, bool force) const
    {
        std::shared_ptr<MWWorld::Action> action(new MWWorld::ActionEquip(ptr, force));

        action->setSound(getUpSoundId(ptr));

        return action;
    }

    MWWorld::Ptr Clothing::copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing> *ref = ptr.get<ESM::Clothing>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    int Clothing::getEnchantmentPoints (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing> *ref = ptr.get<ESM::Clothing>();

        return ref->mBase->mData.mEnchant;
    }

    bool Clothing::canSell (const MWWorld::ConstPtr& item, int npcServices) const
    {
        return (npcServices & ESM::NPC::Clothing)
                || ((npcServices & ESM::NPC::MagicItems) && !getEnchantment(item).empty());
    }

    float Clothing::getWeight(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Clothing> *ref = ptr.get<ESM::Clothing>();
        return ref->mBase->mData.mWeight;
    }
}
