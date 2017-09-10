#include "container.hpp"

#include <components/esm/loadcont.hpp>
#include <components/esm/containerstate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/actionopen.hpp"
#include "../mwworld/actiontrap.hpp"
#include "../mwphysics/physicssystem.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwmechanics/npcstats.hpp"

namespace MWClass
{
    class ContainerCustomData : public MWWorld::CustomData
    {
    public:
        MWWorld::ContainerStore mContainerStore;

        virtual MWWorld::CustomData *clone() const;

        virtual ContainerCustomData& asContainerCustomData()
        {
            return *this;
        }
    };

    MWWorld::CustomData *ContainerCustomData::clone() const
    {
        return new ContainerCustomData (*this);
    }

    void Container::ensureCustomData (const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            std::unique_ptr<ContainerCustomData> data (new ContainerCustomData);

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
            MWBase::Environment::get().getWorld()->removeContainerScripts(ptr);
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
            renderingInterface.getObjects().insertModel(ptr, model, true);
        }
    }

    void Container::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWPhysics::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string Container::getModel(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Container> *ref = ptr.get<ESM::Container>();

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    bool Container::useAnim() const
    {
        return true;
    }

    std::shared_ptr<MWWorld::Action> Container::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        if (!MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Inventory))
            return std::shared_ptr<MWWorld::Action> (new MWWorld::NullAction ());

        if(actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
        {
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const ESM::Sound *sound = store.get<ESM::Sound>().searchRandom("WolfContainer");

            std::shared_ptr<MWWorld::Action> action(new MWWorld::FailedAction("#{sWerewolfRefusal}"));
            if(sound) action->setSound(sound->mId);

            return action;
        }

        const std::string lockedSound = "LockedChest";
        const std::string trapActivationSound = "Disarm Trap Fail";

        MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();
        const MWWorld::InventoryStore& invStore = player.getClass().getInventoryStore(player);

        bool isLocked = ptr.getCellRef().getLockLevel() > 0;
        bool isTrapped = !ptr.getCellRef().getTrap().empty();
        bool hasKey = false;
        std::string keyName;

        // make key id lowercase
        std::string keyId = ptr.getCellRef().getKey();
        Misc::StringUtils::lowerCaseInPlace(keyId);
        for (MWWorld::ConstContainerStoreIterator it = invStore.cbegin(); it != invStore.cend(); ++it)
        {
            std::string refId = it->getCellRef().getRefId();
            Misc::StringUtils::lowerCaseInPlace(refId);
            if (refId == keyId)
            {
                hasKey = true;
                keyName = it->getClass().getName(*it);
            }
        }

        if ((isLocked || isTrapped) && hasKey)
        {
            MWBase::Environment::get().getWindowManager ()->messageBox (keyName + " #{sKeyUsed}");
            if(isLocked)
                unlock(ptr);
            // using a key disarms the trap
            if(isTrapped)
            {
                ptr.getCellRef().setTrap("");
                MWBase::Environment::get().getSoundManager()->playSound3D(ptr,
                    "Disarm Trap", 1.0f, 1.0f, MWBase::SoundManager::Play_TypeSfx,
                    MWBase::SoundManager::Play_Normal);
                isTrapped = false;
            }
        }


        if (!isLocked || hasKey)
        {
            if(!isTrapped)
            {
                std::shared_ptr<MWWorld::Action> action (new MWWorld::ActionOpen(ptr));
                return action;
            }
            else
            {
                // Activate trap
                std::shared_ptr<MWWorld::Action> action(new MWWorld::ActionTrap(ptr.getCellRef().getTrap(), ptr));
                action->setSound(trapActivationSound);
                return action;
            }
        }
        else
        {
            std::shared_ptr<MWWorld::Action> action(new MWWorld::FailedAction(std::string(), ptr));
            action->setSound(lockedSound);
            return action;
        }
    }

    std::string Container::getName (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Container> *ref = ptr.get<ESM::Container>();

        return ref->mBase->mName;
    }

    MWWorld::ContainerStore& Container::getContainerStore (const MWWorld::Ptr& ptr)
        const
    {
        ensureCustomData (ptr);

        return ptr.getRefData().getCustomData()->asContainerCustomData().mContainerStore;
    }

    std::string Container::getScript (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Container> *ref = ptr.get<ESM::Container>();

        return ref->mBase->mScript;
    }

    void Container::registerSelf()
    {
        std::shared_ptr<Class> instance (new Container);

        registerClass (typeid (ESM::Container).name(), instance);
    }

    bool Container::hasToolTip (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Container> *ref = ptr.get<ESM::Container>();

        return (ref->mBase->mName != "");
    }

    MWGui::ToolTipInfo Container::getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Container> *ref = ptr.get<ESM::Container>();

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

    bool Container::canLock(const MWWorld::ConstPtr &ptr) const
    {
        return true;
    }

    MWWorld::Ptr Container::copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const
    {
        const MWWorld::LiveCellRef<ESM::Container> *ref = ptr.get<ESM::Container>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    void Container::readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state) const
    {
        if (!state.mHasCustomState)
            return;
        const ESM::ContainerState& state2 = dynamic_cast<const ESM::ContainerState&> (state);

        if (!ptr.getRefData().getCustomData())
        {
            // Create a CustomData, but don't fill it from ESM records (not needed)
            std::unique_ptr<ContainerCustomData> data (new ContainerCustomData);
            ptr.getRefData().setCustomData (data.release());
        }

        dynamic_cast<ContainerCustomData&> (*ptr.getRefData().getCustomData()).mContainerStore.
            readState (state2.mInventory);
    }

    void Container::writeAdditionalState (const MWWorld::ConstPtr& ptr, ESM::ObjectState& state) const
    {
        ESM::ContainerState& state2 = dynamic_cast<ESM::ContainerState&> (state);

        if (!ptr.getRefData().getCustomData())
        {
            state.mHasCustomState = false;
            return;
        }

        dynamic_cast<const ContainerCustomData&> (*ptr.getRefData().getCustomData()).mContainerStore.
            writeState (state2.mInventory);
    }
}
