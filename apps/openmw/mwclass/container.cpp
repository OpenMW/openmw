
#include "container.hpp"

#include <components/esm/loadcont.hpp>
#include <components/esm/containerstate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/actionopen.hpp"
#include "../mwworld/actiontrap.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
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
    void Container::ensureCustomData (const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            std::auto_ptr<ContainerCustomData> data (new ContainerCustomData);

            MWWorld::LiveCellRef<ESM::Container> *ref =
                ptr.get<ESM::Container>();

            data->mContainerStore.fill(
                ref->mBase->mInventory, ptr.getCellRef().mOwner, ptr.getCellRef().mFaction, MWBase::Environment::get().getWorld()->getStore());

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

    void Container::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        const std::string model = getModel(ptr);
        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    void Container::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty())
            physics.addObject(ptr);
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

        if(get(actor).isNpc() && get(actor).getNpcStats(actor).isWerewolf())
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
        MWWorld::InventoryStore& invStore = MWWorld::Class::get(player).getInventoryStore(player);

        bool needKey = ptr.getCellRef().mLockLevel > 0;
        bool hasKey = false;
        std::string keyName;

        // make key id lowercase
        std::string keyId = ptr.getCellRef().mKey;
        Misc::StringUtils::toLower(keyId);
        for (MWWorld::ContainerStoreIterator it = invStore.begin(); it != invStore.end(); ++it)
        {
            std::string refId = it->getCellRef().mRefID;
            Misc::StringUtils::toLower(refId);
            if (refId == keyId)
            {
                hasKey = true;
                keyName = MWWorld::Class::get(*it).getName(*it);
            }
        }

        if (needKey && hasKey)
        {
            MWBase::Environment::get().getWindowManager ()->messageBox (keyName + " #{sKeyUsed}");
            unlock(ptr);
            // using a key disarms the trap
            ptr.getCellRef().mTrap = "";
        }


        if (!needKey || hasKey)
        {
            if(ptr.getCellRef().mTrap.empty())
            {
                boost::shared_ptr<MWWorld::Action> action (new MWWorld::ActionOpen(ptr));
                return action;
            }
            else
            {
                // Activate trap
                boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionTrap(actor, ptr.getCellRef().mTrap, ptr));
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
        if (ref->mRef.mLockLevel > 0)
            text += "\n#{sLockLevel}: " + MWGui::ToolTips::toString(ref->mRef.mLockLevel);
        else if (ref->mRef.mLockLevel < 0)
            text += "\n#{sUnlocked}";
        if (ref->mRef.mTrap != "")
            text += "\n#{sTrapped}";

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getMiscString(ref->mRef.mOwner, "Owner");
            text += MWGui::ToolTips::getMiscString(ref->mRef.mFaction, "Faction");
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
            ptr.getCellRef().mLockLevel = abs(lockLevel); //Changes lock to locklevel, in positive
        else
            ptr.getCellRef().mLockLevel = abs(ptr.getCellRef().mLockLevel); //No locklevel given, just flip the oriional one
    }

    void Container::unlock (const MWWorld::Ptr& ptr) const
    {
        ptr.getCellRef().mLockLevel = -abs(ptr.getCellRef().mLockLevel); //Makes lockLevel negative
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

        ensureCustomData (ptr);

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
