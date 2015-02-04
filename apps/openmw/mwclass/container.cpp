
#include "container.hpp"

#include <components/esm/loadcont.hpp>
#include <components/esm/containerstate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/actionopen.hpp"
#include "../mwworld/actiontrap.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/actors.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwmechanics/npcstats.hpp"

namespace
{
    struct ContainerCustomData : public MWWorld::CustomData
    {
        MWWorld::ContainerStore mContainerStore;

        virtual MWWorld::CustomData *clone() const;
    };

    MWWorld::CustomData *ContainerCustomData::clone() const
    {
        return new ContainerCustomData (*this);
    }
}

namespace MWClass
{
    std::string Container::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM::Container>()->mBase->mId;
    }

    void Container::ensureCustomData (const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            std::auto_ptr<ContainerCustomData> data (new ContainerCustomData);

            MWWorld::LiveCellRef<ESM::Container> *ref =
                ptr.get<ESM::Container>();

            // setting ownership not needed, since taking items from a container inherits the
            // container's owner automatically
            data->mContainerStore.fill(
                ref->mBase->mInventory, "");

            // store
            ptr.getRefData().setCustomData (data.release());
        }
    }

    void Container::respawn(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Container> *ref =
            ptr.get<ESM::Container>();
        if (ref->mBase->mFlags & ESM::Container::Respawn)
        {
            ptr.getRefData().setCustomData(NULL);
        }
    }

    void Container::restock(const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Container> *ref = ptr.get<ESM::Container>();
        const ESM::InventoryList& list = ref->mBase->mInventory;
        MWWorld::ContainerStore& store = getContainerStore(ptr);

        // setting ownership not needed, since taking items from a container inherits the
        // container's owner automatically
        store.restock(list, ptr, "");
    }

    void Container::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty()) {
            MWRender::Actors& actors = renderingInterface.getActors();
            actors.insertActivator(ptr, model);
        }
    }

    void Container::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
        MWBase::Environment::get().getMechanicsManager()->add(ptr);
    }

    std::string Container::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Container> *ref =
            ptr.get<ESM::Container>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    boost::shared_ptr<MWWorld::Action> Container::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        if (!MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Inventory))
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction ());

        if(actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
        {
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const ESM::Sound *sound = store.get<ESM::Sound>().searchRandom("WolfContainer");

            boost::shared_ptr<MWWorld::Action> action(new MWWorld::FailedAction("#{sWerewolfRefusal}"));
            if(sound) action->setSound(sound->mId);

            return action;
        }

        const std::string lockedSound = "LockedChest";
        const std::string trapActivationSound = "Disarm Trap Fail";

        MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();
        MWWorld::InventoryStore& invStore = player.getClass().getInventoryStore(player);

        bool needKey = ptr.getCellRef().getLockLevel() > 0;
        bool hasKey = false;
        std::string keyName;

        // make key id lowercase
        std::string keyId = ptr.getCellRef().getKey();
        Misc::StringUtils::toLower(keyId);
        for (MWWorld::ContainerStoreIterator it = invStore.begin(); it != invStore.end(); ++it)
        {
            std::string refId = it->getCellRef().getRefId();
            Misc::StringUtils::toLower(refId);
            if (refId == keyId)
            {
                hasKey = true;
                keyName = it->getClass().getName(*it);
            }
        }

        if (needKey && hasKey)
        {
            MWBase::Environment::get().getWindowManager ()->messageBox (keyName + " #{sKeyUsed}");
            unlock(ptr);
            // using a key disarms the trap
            ptr.getCellRef().setTrap("");
        }


        if (!needKey || hasKey)
        {
            if(ptr.getCellRef().getTrap().empty())
            {
                boost::shared_ptr<MWWorld::Action> action (new MWWorld::ActionOpen(ptr));
                return action;
            }
            else
            {
                // Activate trap
                boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionTrap(actor, ptr.getCellRef().getTrap(), ptr));
                action->setSound(trapActivationSound);
                return action;
            }
        }
        else
        {
            boost::shared_ptr<MWWorld::Action> action(new MWWorld::FailedAction);
            action->setSound(lockedSound);
            return action;
        }
    }

    std::string Container::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Container> *ref =
            ptr.get<ESM::Container>();

        return ref->mBase->mName;
    }

    MWWorld::ContainerStore& Container::getContainerStore (const MWWorld::Ptr& ptr)
        const
    {
        ensureCustomData (ptr);

        return dynamic_cast<ContainerCustomData&> (*ptr.getRefData().getCustomData()).mContainerStore;
    }

    std::string Container::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Container> *ref =
            ptr.get<ESM::Container>();

        return ref->mBase->mScript;
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

        return (ref->mBase->mName != "");
    }

    MWGui::ToolTipInfo Container::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Container> *ref =
            ptr.get<ESM::Container>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mName;

        std::string text;
        if (ptr.getCellRef().getLockLevel() > 0)
            text += "\n#{sLockLevel}: " + MWGui::ToolTips::toString(ptr.getCellRef().getLockLevel());
        else if (ptr.getCellRef().getLockLevel() < 0)
            text += "\n#{sUnlocked}";
        if (ptr.getCellRef().getTrap() != "")
            text += "\n#{sTrapped}";

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }

        info.text = text;

        return info;
    }

    float Container::getCapacity (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Container> *ref =
            ptr.get<ESM::Container>();

        return ref->mBase->mWeight;
    }

    float Container::getEncumbrance (const MWWorld::Ptr& ptr) const
    {
        return getContainerStore (ptr).getWeight();
    }

    void Container::lock (const MWWorld::Ptr& ptr, int lockLevel) const
    {
        if(lockLevel!=0)
            ptr.getCellRef().setLockLevel(abs(lockLevel)); //Changes lock to locklevel, in positive
        else
            ptr.getCellRef().setLockLevel(abs(ptr.getCellRef().getLockLevel())); //No locklevel given, just flip the original one
    }

    void Container::unlock (const MWWorld::Ptr& ptr) const
    {
        ptr.getCellRef().setLockLevel(-abs(ptr.getCellRef().getLockLevel())); //Makes lockLevel negative
    }


    MWWorld::Ptr
    Container::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Container> *ref =
            ptr.get<ESM::Container>();

        return MWWorld::Ptr(&cell.get<ESM::Container>().insert(*ref), &cell);
    }

    void Container::readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state)
        const
    {
        const ESM::ContainerState& state2 = dynamic_cast<const ESM::ContainerState&> (state);

        if (!ptr.getRefData().getCustomData())
        {
            // Create a CustomData, but don't fill it from ESM records (not needed)
            std::auto_ptr<ContainerCustomData> data (new ContainerCustomData);
            ptr.getRefData().setCustomData (data.release());
        }

        dynamic_cast<ContainerCustomData&> (*ptr.getRefData().getCustomData()).mContainerStore.
            readState (state2.mInventory);
    }

    void Container::writeAdditionalState (const MWWorld::Ptr& ptr, ESM::ObjectState& state)
        const
    {
        ESM::ContainerState& state2 = dynamic_cast<ESM::ContainerState&> (state);

        ensureCustomData (ptr);

        dynamic_cast<ContainerCustomData&> (*ptr.getRefData().getCustomData()).mContainerStore.
            writeState (state2.mInventory);
    }
}
