
#include "container.hpp"

#include <components/esm/loadcont.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/environment.hpp"

#include "../mwrender/objects.hpp"

#include "../mwsound/soundmanager.hpp"

namespace
{
    struct CustomData : public MWWorld::CustomData
    {
        MWWorld::ContainerStore mContainerStore;

        virtual MWWorld::CustomData *clone() const;
    };

    MWWorld::CustomData *CustomData::clone() const
    {
        return new CustomData (*this);
    }
}

namespace MWClass
{
    void Container::ensureCustomData (const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            std::auto_ptr<CustomData> data (new CustomData);

            // \todo add initial container content

            // store
            ptr.getRefData().setCustomData (data.release());
        }
    }

    void Container::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        ESMS::LiveCellRef<ESM::Container, MWWorld::RefData> *ref =
            ptr.get<ESM::Container>();

        assert (ref->base != NULL);
        const std::string &model = ref->base->model;

        if (!model.empty())
        {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, "meshes\\" + model);
        }
    }

    void Container::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics, MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Container, MWWorld::RefData> *ref =
            ptr.get<ESM::Container>();


        const std::string &model = ref->base->model;
        assert (ref->base != NULL);
        if(!model.empty()){
            physics.insertObjectPhysics(ptr, "meshes\\" + model);
        }

    }

    boost::shared_ptr<MWWorld::Action> Container::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const
    {
        const std::string lockedSound = "LockedChest";
        const std::string trapActivationSound = "Disarm Trap Fail";

        if (ptr.getCellRef().lockLevel>0)
        {
            // TODO check for key
            std::cout << "Locked container" << std::endl;
            environment.mSoundManager->playSound(lockedSound, 1.0, 1.0);
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction);
        }
        else
        {
            std::cout << "Unlocked container" << std::endl;
            if(ptr.getCellRef().trap.empty())
            {
                // Not trapped, Inventory GUI goes here
                return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction);
            }
            else
            {
                // Trap activation goes here
                std::cout << "Trapped with: " << ptr.getCellRef().trap << std::endl;
                environment.mSoundManager->playSound(trapActivationSound, 1.0, 1.0);
                return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction);
            }
        }
    }

    std::string Container::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Container, MWWorld::RefData> *ref =
            ptr.get<ESM::Container>();

        return ref->base->name;
    }

    MWWorld::ContainerStore& Container::getContainerStore (const MWWorld::Ptr& ptr)
        const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mContainerStore;
    }

    std::string Container::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Container, MWWorld::RefData> *ref =
            ptr.get<ESM::Container>();

        return ref->base->script;
    }

    void Container::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Container);

        registerClass (typeid (ESM::Container).name(), instance);
    }
}
