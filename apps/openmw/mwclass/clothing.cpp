
#include "clothing.hpp"

#include <components/esm/loadclot.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/actionequip.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/physicssystem.hpp"

#include "../mwgui/tooltips.hpp"
#include "../mwgui/window_manager.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwsound/soundmanager.hpp"

namespace MWClass
{
    void Clothing::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        const std::string model = getModel(ptr);
        if (!model.empty()) {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, model);
        }
    }

    void Clothing::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty()) {
            physics.insertObjectPhysics(ptr, model);
        }
    }
    
    std::string Clothing::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();
        assert(ref->base != NULL);

        const std::string &model = ref->base->model;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Clothing::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Clothing::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
         MWBase::Environment::get().getSoundManager()->playSound3D (ptr, getUpSoundId(ptr), 1.0, 1.0, MWSound::Play_NoTrack);

        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionTake (ptr));
    }

    std::string Clothing::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        return ref->base->script;
    }

    std::pair<std::vector<int>, bool> Clothing::getEquipmentSlots (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        std::vector<int> slots;

        if (ref->base->data.type==ESM::Clothing::Ring)
        {
            slots.push_back (int (MWWorld::InventoryStore::Slot_LeftRing));
            slots.push_back (int (MWWorld::InventoryStore::Slot_RightRing));
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
                if (sMapping[i][0]==ref->base->data.type)
                {
                    slots.push_back (int (sMapping[i][1]));
                    break;
                }
        }

        return std::make_pair (slots, false);
    }

    int Clothing::getEquipmentSkill (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        if (ref->base->data.type==ESM::Clothing::Shoes)
            return ESM::Skill::Unarmored;

        return -1;
    }

    int Clothing::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        return ref->base->data.value;
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

        if (ref->base->data.type == 8)
        {
            return std::string("Item Ring Up");
        }
        return std::string("Item Clothes Up");
    }

    std::string Clothing::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
         MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        if (ref->base->data.type == 8)
        {
            return std::string("Item Ring Down");
        }
        return std::string("Item Clothes Down");
    }

    std::string Clothing::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
          MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        return ref->base->icon;
    }

    bool Clothing::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        return (ref->base->name != "");
    }

    MWGui::ToolTipInfo Clothing::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        MWGui::ToolTipInfo info;
        info.caption = ref->base->name + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        info.icon = ref->base->icon;

        const ESMS::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        std::string text;

        text += "\n" + store.gameSettings.search("sWeight")->str + ": " + MWGui::ToolTips::toString(ref->base->data.weight);
        text += MWGui::ToolTips::getValueString(ref->base->data.value, store.gameSettings.search("sValue")->str);

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getMiscString(ref->ref.owner, "Owner");
            text += MWGui::ToolTips::getMiscString(ref->base->script, "Script");
        }

        info.enchant = ref->base->enchant;

        info.text = text;

        return info;
    }

    std::string Clothing::getEnchantment (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        return ref->base->enchant;
    }

    boost::shared_ptr<MWWorld::Action> Clothing::use (const MWWorld::Ptr& ptr) const
    {
        MWBase::Environment::get().getSoundManager()->playSound (getUpSoundId(ptr), 1.0, 1.0);

        return boost::shared_ptr<MWWorld::Action>(new MWWorld::ActionEquip(ptr));
    }

    MWWorld::Ptr
    Clothing::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Clothing> *ref =
            ptr.get<ESM::Clothing>();

        return MWWorld::Ptr(&cell.clothes.insert(*ref), &cell);
    }
}
