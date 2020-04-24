#include "container.hpp"

#include <components/esm/loadcont.hpp>
#include <components/esm/containerstate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/actionharvest.hpp"
#include "../mwworld/actionopen.hpp"
#include "../mwworld/actiontrap.hpp"
#include "../mwphysics/physicssystem.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/animation.hpp"
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
        virtual const ContainerCustomData& asContainerCustomData() const
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

            MWBase::Environment::get().getWorld()->addContainerScripts(ptr, ptr.getCell());
        }
    }

    bool canBeHarvested(const MWWorld::ConstPtr& ptr)
    {
        const MWRender::Animation* animation = MWBase::Environment::get().getWorld()->getAnimation(ptr);
        if (animation == nullptr)
            return false;

        return animation->canBeHarvested();
    }

    void Container::respawn(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Container> *ref =
            ptr.get<ESM::Container>();
        if (ref->mBase->mFlags & ESM::Container::Respawn)
        {
            // Container was not touched, there is no need to modify its content.
            if (ptr.getRefData().getCustomData() == nullptr)
                return;

            MWBase::Environment::get().getWorld()->removeContainerScripts(ptr);
            ptr.getRefData().setCustomData(nullptr);
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
        MWWorld::InventoryStore& invStore = player.getClass().getInventoryStore(player);

        bool isLocked = ptr.getCellRef().getLockLevel() > 0;
        bool isTrapped = !ptr.getCellRef().getTrap().empty();
        bool hasKey = false;
        std::string keyName;

        const std::string keyId = ptr.getCellRef().getKey();
        if (!keyId.empty())
        {
            MWWorld::Ptr keyPtr = invStore.search(keyId);
            if (!keyPtr.isEmpty())
            {
                hasKey = true;
                keyName = keyPtr.getClass().getName(keyPtr);
            }
        }

        if (isLocked && hasKey)
        {
            MWBase::Environment::get().getWindowManager ()->messageBox (keyName + " #{sKeyUsed}");
            ptr.getCellRef().unlock();
            // using a key disarms the trap
            if(isTrapped)
            {
                ptr.getCellRef().setTrap("");
                MWBase::Environment::get().getSoundManager()->playSound3D(ptr, "Disarm Trap", 1.0f, 1.0f);
                isTrapped = false;
            }
        }


        if (!isLocked || hasKey)
        {
            if(!isTrapped)
            {
                if (canBeHarvested(ptr))
                {
                    std::shared_ptr<MWWorld::Action> action (new MWWorld::ActionHarvest(ptr));
                    return action;
                }

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
        const std::string& name = ref->mBase->mName;

        return !name.empty() ? name : ref->mBase->mId;
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
        if (const MWWorld::CustomData* data = ptr.getRefData().getCustomData())
            return !canBeHarvested(ptr) || data->asContainerCustomData().mContainerStore.hasVisibleItems();

        return true;
    }

    MWGui::ToolTipInfo Container::getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Container> *ref = ptr.get<ESM::Container>();

        MWGui::ToolTipInfo info;
        info.caption = MyGUI::TextIterator::toTagsString(getName(ptr));

        std::string text;
        int lockLevel = ptr.getCellRef().getLockLevel();
        if (lockLevel > 0 && lockLevel != ESM::UnbreakableLock)
            text += "\n#{sLockLevel}: " + MWGui::ToolTips::toString(lockLevel);
        else if (lockLevel < 0)
            text += "\n#{sUnlocked}";
        if (ptr.getCellRef().getTrap() != "")
            text += "\n#{sTrapped}";

        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
        {   text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
            if (Misc::StringUtils::ciEqual(ptr.getCellRef().getRefId(), "stolen_goods"))
                text += "\nYou can not use evidence chests";
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

    bool Container::canLock(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Container> *ref = ptr.get<ESM::Container>();
        return !(ref->mBase->mFlags & ESM::Container::Organic);
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

        if (!ptr.getRefData().getCustomData())
        {
            // Create a CustomData, but don't fill it from ESM records (not needed)
            std::unique_ptr<ContainerCustomData> data (new ContainerCustomData);
            ptr.getRefData().setCustomData (data.release());
        }

        ContainerCustomData& customData = ptr.getRefData().getCustomData()->asContainerCustomData();
        const ESM::ContainerState& containerState = state.asContainerState();
        customData.mContainerStore.readState (containerState.mInventory);
    }

    void Container::writeAdditionalState (const MWWorld::ConstPtr& ptr, ESM::ObjectState& state) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            state.mHasCustomState = false;
            return;
        }

        const ContainerCustomData& customData = ptr.getRefData().getCustomData()->asContainerCustomData();
        ESM::ContainerState& containerState = state.asContainerState();
        customData.mContainerStore.writeState (containerState.mInventory);
    }
}
