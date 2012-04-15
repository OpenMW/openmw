
#include "light.hpp"

#include <components/esm/loadligh.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwsound/soundmanager.hpp"

#include "../mwrender/objects.hpp"

namespace MWClass
{
    void Light::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        ESMS::LiveCellRef<ESM::Light, MWWorld::RefData> *ref =
            ptr.get<ESM::Light>();

        assert (ref->base != NULL);
        const std::string &model = ref->base->model;

        MWRender::Objects& objects = renderingInterface.getObjects();
        objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);

        if (!model.empty())
            objects.insertMesh(ptr, "meshes\\" + model);

        const int color = ref->base->data.color;
        const float r = ((color >> 0) & 0xFF) / 255.0f;
        const float g = ((color >> 8) & 0xFF) / 255.0f;
        const float b = ((color >> 16) & 0xFF) / 255.0f;
        const float radius = float (ref->base->data.radius);
        objects.insertLight (ptr, r, g, b, radius);
    }

    void Light::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics, MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Light, MWWorld::RefData> *ref =
            ptr.get<ESM::Light>();

        assert (ref->base != NULL);
        const std::string &model = ref->base->model;

        if(!model.empty()){
            physics.insertObjectPhysics(ptr, "meshes\\" + model);
        }
    }

    void Light::enable (const MWWorld::Ptr& ptr, MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Light, MWWorld::RefData> *ref =
            ptr.get<ESM::Light>();

        if (!ref->base->sound.empty())
        {
            environment.mSoundManager->playSound3D (ptr, ref->base->sound, 1.0, 1.0, true);
        }
    }

    std::string Light::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Light, MWWorld::RefData> *ref =
            ptr.get<ESM::Light>();

        if (ref->base->model.empty())
            return "";

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Light::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Light, MWWorld::RefData> *ref =
            ptr.get<ESM::Light>();

        if (!(ref->base->data.flags & ESM::Light::Carry))
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction);

        environment.mSoundManager->playSound3D (ptr, getUpSoundId(ptr, environment), 1.0, 1.0, false, true);

        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionTake (ptr));
    }

    std::string Light::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Light, MWWorld::RefData> *ref =
            ptr.get<ESM::Light>();

        return ref->base->script;
    }

    std::pair<std::vector<int>, bool> Light::getEquipmentSlots (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Light, MWWorld::RefData> *ref =
            ptr.get<ESM::Light>();

        std::vector<int> slots;

        if (ref->base->data.flags & ESM::Light::Carry)
            slots.push_back (int (MWWorld::InventoryStore::Slot_CarriedLeft));

        return std::make_pair (slots, false);
    }

    void Light::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Light);

        registerClass (typeid (ESM::Light).name(), instance);
    }

    std::string Light::getUpSoundId (const MWWorld::Ptr& ptr, const MWWorld::Environment& environment) const
    {
        return std::string("Item Misc Up");
    }

    std::string Light::getDownSoundId (const MWWorld::Ptr& ptr, const MWWorld::Environment& environment) const
    {
        return std::string("Item Misc Down");
    }

    std::string Light::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
          ESMS::LiveCellRef<ESM::Light, MWWorld::RefData> *ref =
            ptr.get<ESM::Light>();

        return ref->base->icon;
    }
}
