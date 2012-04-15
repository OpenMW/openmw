
#include "clothing.hpp"

#include <components/esm/loadclot.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwrender/objects.hpp"

#include "../mwsound/soundmanager.hpp"

namespace MWClass
{
    void Clothing::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        ESMS::LiveCellRef<ESM::Clothing, MWWorld::RefData> *ref =
            ptr.get<ESM::Clothing>();

        assert (ref->base != NULL);
        const std::string &model = ref->base->model;

        if (!model.empty())
        {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, "meshes\\" + model);
        }
    }

    void Clothing::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics, MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Clothing, MWWorld::RefData> *ref =
            ptr.get<ESM::Clothing>();


        const std::string &model = ref->base->model;
        assert (ref->base != NULL);
        if(!model.empty()){
            physics.insertObjectPhysics(ptr, "meshes\\" + model);
        }

    }

    std::string Clothing::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Clothing, MWWorld::RefData> *ref =
            ptr.get<ESM::Clothing>();

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Clothing::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const
    {
         environment.mSoundManager->playSound3D (ptr, getUpSoundId(ptr, environment), 1.0, 1.0, false, true);

        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionTake (ptr));
    }

    std::string Clothing::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Clothing, MWWorld::RefData> *ref =
            ptr.get<ESM::Clothing>();

        return ref->base->script;
    }

    std::pair<std::vector<int>, bool> Clothing::getEquipmentSlots (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Clothing, MWWorld::RefData> *ref =
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
                { ESM::Clothing::Shirt, MWWorld::InventoryStore::Slot_Cuirass },
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

    int Clothing::getEquipmentSkill (const MWWorld::Ptr& ptr,
        const MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Clothing, MWWorld::RefData> *ref =
            ptr.get<ESM::Clothing>();

        if (ref->base->data.type==ESM::Clothing::Shoes)
            return ESM::Skill::Unarmored;

        return -1;
    }

    void Clothing::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Clothing);

        registerClass (typeid (ESM::Clothing).name(), instance);
    }

    std::string Clothing::getUpSoundId (const MWWorld::Ptr& ptr, const MWWorld::Environment& environment) const
    {
         ESMS::LiveCellRef<ESM::Clothing, MWWorld::RefData> *ref =
            ptr.get<ESM::Clothing>();

        if (ref->base->data.type == 8)
        {
            return std::string("Item Ring Up");
        }
        return std::string("Item Clothes Up");
    }

    std::string Clothing::getDownSoundId (const MWWorld::Ptr& ptr, const MWWorld::Environment& environment) const
    {
         ESMS::LiveCellRef<ESM::Clothing, MWWorld::RefData> *ref =
            ptr.get<ESM::Clothing>();

        if (ref->base->data.type == 8)
        {
            return std::string("Item Ring Down");
        }
        return std::string("Item Clothes Down");
    }

    std::string Clothing::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
          ESMS::LiveCellRef<ESM::Clothing, MWWorld::RefData> *ref =
            ptr.get<ESM::Clothing>();

        return ref->base->icon;
    }
}
