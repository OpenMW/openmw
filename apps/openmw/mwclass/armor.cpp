
#include "armor.hpp"

#include <components/esm/loadarmo.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/environment.hpp"

#include "../mwrender/objects.hpp"

#include "../mwsound/soundmanager.hpp"

namespace MWClass
{
    void Armor::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        ESMS::LiveCellRef<ESM::Armor, MWWorld::RefData> *ref =
            ptr.get<ESM::Armor>();

        assert (ref->base != NULL);
        const std::string &model = ref->base->model;

        if (!model.empty())
        {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, "meshes\\" + model);
        }
    }

    void Armor::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics, MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Armor, MWWorld::RefData> *ref =
            ptr.get<ESM::Armor>();

        const std::string &model = ref->base->model;
        assert (ref->base != NULL);
        if(!model.empty()){
            physics.insertObjectPhysics(ptr, "meshes\\" + model);
        }

    }

    std::string Armor::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Armor, MWWorld::RefData> *ref =
            ptr.get<ESM::Armor>();

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Armor::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const
    {
        environment.mSoundManager->playSound3D (ptr, getUpSoundId(ptr), 1.0, 1.0, false, true);

        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionTake (ptr));
    }

    bool Armor::hasItemHealth (const MWWorld::Ptr& ptr) const
    {
        return true;
    }

    int Armor::getItemMaxHealth (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Armor, MWWorld::RefData> *ref =
            ptr.get<ESM::Armor>();

        return ref->base->data.health;
    }

    std::string Armor::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Armor, MWWorld::RefData> *ref =
            ptr.get<ESM::Armor>();

        return ref->base->script;
    }

    void Armor::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Armor);

        registerClass (typeid (ESM::Armor).name(), instance);
    }

    std::string Armor::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        int wc = getWeightCategory(ptr);
        if (wc == WC_Light)
            return std::string("Item Armor Light Up");
        else if (wc == WC_Medium)
            return std::string("Item Armor Medium Up");
        else
            return std::string("Item Armor Heavy Up");
    }

    std::string Armor::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        int wc = getWeightCategory(ptr);
        if (wc == WC_Light)
            return std::string("Item Armor Light Down");
        else if (wc == WC_Medium)
            return std::string("Item Armor Medium Down");
        else
            return std::string("Item Armor Heavy Down");
    }

    int Armor::getWeightCategory (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Armor, MWWorld::RefData> *ref =
            ptr.get<ESM::Armor>();

        float weight = ref->base->data.weight;
        int type = ref->base->data.type;
        // Boots
        if (type == 5)
        {
            if (weight <= 12.0)
            {
                return WC_Light;
            }
            else if (weight > 18.0)
            {
                return WC_Heavy;
            }
            else
            {
                return WC_Medium;
            }
        }
        // Cuirass
        if (type == 1)
        {
            if (weight <= 18.0)
            {
                return WC_Light;
            }
            else if (weight > 27.0)
            {
                return WC_Heavy;
            }
            else
            {
                return WC_Medium;
            }
        }
        // Greaves, Shield
        if (type == 4 || type == 8)
        {
            if (weight <= 9.0)
            {
                return WC_Light;
            }
            else if (weight > 13.5)
            {
                return WC_Heavy;
            }
            else
            {
                return WC_Medium;
            }
        }
        // Bracer, Gauntlet, Helmet
        if (type == 6 || type == 7 || type == 9 || type == 10 || type == 0)
        {
            if (weight <= 3.0)
            {
                return WC_Light;
            }
            else if (weight > 4.5)
            {
                return WC_Heavy;
            }
            else
            {
                return WC_Medium;
            }
        }
        // Pauldrons
        if (type == 2 || type == 3)
        {
            if (weight <= 6.0)
            {
                return WC_Light;
            }
            else if (weight > 9.0)
            {
                return WC_Heavy;
            }
            else
            {
                return WC_Medium;
            }
        }

        return WC_Light;
    }
}
