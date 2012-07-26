
#include "container.hpp"

#include <components/esm/loadcont.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/actionopen.hpp"
#include "../mwworld/physicssystem.hpp"

#include "../mwgui/window_manager.hpp"
#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

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
        const std::string model = getModel(ptr);
        if (!model.empty()) {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, model);
        }
    }

    void Container::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty()) {
            physics.insertObjectPhysics(ptr, model);
        }
    }
    
    std::string Container::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Container> *ref =
            ptr.get<ESM::Container>();
        assert(ref->base != NULL);

        const std::string &model = ref->base->model;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    boost::shared_ptr<MWWorld::Action> Container::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        const std::string lockedSound = "LockedChest";
        const std::string trapActivationSound = "Disarm Trap Fail";


        if (ptr.getCellRef().lockLevel>0)
        {
            // TODO check for key
            std::cout << "Locked container" << std::endl;
            MWBase::Environment::get().getSoundManager()->playSound3D (ptr, lockedSound, 1.0, 1.0);
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction);
        }
        else
        {
            std::cout << "Unlocked container" << std::endl;
            if(ptr.getCellRef().trap.empty())
            {
                // Not trapped, Inventory GUI goes here
                //return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction);
                return boost::shared_ptr<MWWorld::Action> (new MWWorld::ActionOpen(ptr));
            }
            else
            {
                // Trap activation goes here
                std::cout << "Activated trap: " << ptr.getCellRef().trap << std::endl;
                MWBase::Environment::get().getSoundManager()->playSound3D (ptr, trapActivationSound, 1.0, 1.0);
                ptr.getCellRef().trap = "";
                return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction);
            }
        }
    }

    std::string Container::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Container> *ref =
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
        MWWorld::LiveCellRef<ESM::Container> *ref =
            ptr.get<ESM::Container>();

        return ref->base->script;
    }

    void Container::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Container);

        registerClass (typeid (ESM::Container).name(), instance);
    }

    bool Container::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Container> *ref =
            ptr.get<ESM::Container>();

        return (ref->base->name != "");
    }

    MWGui::ToolTipInfo Container::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Container> *ref =
            ptr.get<ESM::Container>();

        MWGui::ToolTipInfo info;
        info.caption = ref->base->name;

        const ESMS::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        std::string text;
        if (ref->ref.lockLevel > 0)
            text += "\n" + store.gameSettings.search("sLockLevel")->str + ": " + MWGui::ToolTips::toString(ref->ref.lockLevel);
        if (ref->ref.trap != "")
            text += "\n" + store.gameSettings.search("sTrapped")->str;

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getMiscString(ref->ref.owner, "Owner");
            text += MWGui::ToolTips::getMiscString(ref->base->script, "Script");
        }

        info.text = text;

        return info;
    }

    float Container::getCapacity (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Container> *ref =
            ptr.get<ESM::Container>();

        return ref->base->weight;
    }

    float Container::getEncumbrance (const MWWorld::Ptr& ptr) const
    {
        return getContainerStore (ptr).getWeight();
    }

    void Container::lock (const MWWorld::Ptr& ptr, int lockLevel) const
    {
        if (lockLevel<0)
            lockLevel = 0;

        ptr.getCellRef().lockLevel = lockLevel;
    }

    void Container::unlock (const MWWorld::Ptr& ptr) const
    {
        ptr.getCellRef().lockLevel = 0;
    }

    MWWorld::Ptr
    Container::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Container> *ref =
            ptr.get<ESM::Container>();

        return MWWorld::Ptr(&cell.containers.insert(*ref), &cell);
    }
}
