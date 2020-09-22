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
#include "../mwworld/localscripts.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/animation.hpp"
#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/npcstats.hpp"

namespace
{
    void removeScripts(const MWWorld::ContainerStore& store)
    {
        auto& scripts = MWBase::Environment::get().getWorld()->getLocalScripts();
        for(const MWWorld::ConstPtr& ptr : store)
        {
            if(!ptr.getClass().getScript(ptr).empty())
               scripts.remove(ptr);
        }
    }

    void addScripts(MWWorld::ContainerStore& store, MWWorld::CellStore* cell)
    {
        auto& scripts = MWBase::Environment::get().getWorld()->getLocalScripts();
        for(const MWWorld::Ptr& ptr : store)
        {
            const std::string& script = ptr.getClass().getScript(ptr);
            if(!script.empty())
            {
                MWWorld::Ptr item = ptr;
                item.mCell = cell;
                scripts.add(script, item);
            }
        }
    }
}

namespace MWClass
{
    class ResolutionListener
    {
        ContainerCustomData& mCustomData;
        MWWorld::CellStore* mCell;
    public:
        ResolutionListener(ContainerCustomData& customData, MWWorld::CellStore* cell) : mCustomData(customData), mCell(cell) {}

        ~ResolutionListener();
    };

    class ResolvingStoreManager : public MWWorld::ContainerStoreProvider
    {
        ContainerCustomData& mCustomData;
        const ESM::Container& mContainer;
        MWWorld::CellStore* mCell;
        mutable std::shared_ptr<ResolutionListener> mListener;
    public:
        ResolvingStoreManager(ContainerCustomData& customData, const ESM::Container& container, MWWorld::CellStore* cell)
        : mCustomData(customData), mContainer(container), mCell(cell) {}

        virtual MWWorld::ContainerStore& getMutable() override;

        virtual const MWWorld::ContainerStore& getImmutable() const override;
    };

    ContainerCustomData::ContainerCustomData(const ESM::Container& container, MWWorld::CellStore* cell)
    : mUnresolvedStore(std::make_unique<MWWorld::ContainerStore>())
    , mSeed(Misc::Rng::rollDice(std::numeric_limits<int>::max())) {
        // setting ownership not needed, since taking items from a container inherits the
        // container's owner automatically
        mUnresolvedStore->fillNonRandom(container.mInventory, "");
        addScripts(*mUnresolvedStore, cell);
    }

    ContainerCustomData::ContainerCustomData(const ESM::InventoryState& inventory)
    : mResolvedStore(std::make_unique<MWWorld::ContainerStore>()), mSeed()
    {
        mResolvedStore->readState(inventory);
    }

    ContainerCustomData::ContainerCustomData(const ContainerCustomData& other) : mSeed(other.mSeed)
    {
        if(other.mResolvedStore && other.mResolvedStore->isModified())
            mResolvedStore = std::make_unique<MWWorld::ContainerStore>(*other.mResolvedStore);
        else
            mUnresolvedStore = std::make_unique<MWWorld::ContainerStore>(*other.mUnresolvedStore);
    }

    MWWorld::CustomData *ContainerCustomData::clone() const
    {
        return new ContainerCustomData (*this);
    }

    ContainerCustomData& ContainerCustomData::asContainerCustomData()
    {
        return *this;
    }
    const ContainerCustomData& ContainerCustomData::asContainerCustomData() const
    {
        return *this;
    }

    const MWWorld::ContainerStore& ContainerCustomData::getImmutable(std::shared_ptr<ResolutionListener>& listener, MWWorld::CellStore* cell)
    {
        assignListener(listener, cell);
        if(mResolvedStore)
            return *mResolvedStore;
        return *mUnresolvedStore;
    }

    MWWorld::ContainerStore& ContainerCustomData::getMutable(std::shared_ptr<ResolutionListener>& listener, const ESM::Container& container, MWWorld::CellStore* cell)
    {
        assignListener(listener, cell);
        if(!mResolvedStore)
        {
            auto store = std::make_unique<MWWorld::ContainerStore>();
            Misc::Rng generator(mSeed);
            store->fill(container.mInventory, "", generator);
            mResolvedStore = std::move(store);
            removeScripts(*mUnresolvedStore);
            addScripts(*mResolvedStore, cell);
        }
        return *mResolvedStore;
    }

    bool ContainerCustomData::isModified() const
    {
        return mResolvedStore != nullptr;
    }

    void ContainerCustomData::assignListener(std::shared_ptr<ResolutionListener>& listener, MWWorld::CellStore* cell)
    {
        // Only assign if no permanent resolution has taken place
        if(mUnresolvedStore)
        {
            listener = mListener.lock();
            if(!listener)
            {
                listener = std::make_shared<ResolutionListener>(*this, cell);
                mListener = listener;
            }
        }
    }

    MWWorld::ContainerStore& ResolvingStoreManager::getMutable()
    {
        return mCustomData.getMutable(mListener, mContainer, mCell);
    }

    const MWWorld::ContainerStore& ResolvingStoreManager::getImmutable() const
    {
        return mCustomData.getImmutable(mListener, mCell);
    }

    ResolutionListener::~ResolutionListener()
    {
        // A mutable store was requested...
        if(mCustomData.mResolvedStore)
        {
            if(mCustomData.mResolvedStore->isModified())
                mCustomData.mUnresolvedStore.reset(); // ...and modified. Toss the unresolved store
            else
            {
                // ...but not modified. Toss it
                removeScripts(*mCustomData.mResolvedStore);
                mCustomData.mResolvedStore.reset();
                addScripts(*mCustomData.mUnresolvedStore, mCell);
            }
        }
    }

    void Container::ensureCustomData (const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            MWWorld::LiveCellRef<ESM::Container> *ref = ptr.get<ESM::Container>();

            // store
            ptr.getRefData().setCustomData (std::make_unique<ContainerCustomData>(*ref->mBase, ptr.getCell()).release());
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

    MWWorld::StoreManager Container::getStoreManager (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);
        auto& data = ptr.getRefData().getCustomData()->asContainerCustomData();
        if(!data.mUnresolvedStore)
            return data.mResolvedStore.get();
        const ESM::Container* container = ptr.get<ESM::Container>()->mBase;
        return {std::make_unique<ResolvingStoreManager>(data, *container, ptr.getCell())};
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
        {
            if(!canBeHarvested(ptr))
                return true;
            if(data->asContainerCustomData().mResolvedStore)
                return data->asContainerCustomData().mResolvedStore->hasVisibleItems();
            if(data->asContainerCustomData().mUnresolvedStore)
                return data->asContainerCustomData().mUnresolvedStore->hasVisibleItems();
            return false;
        }
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
        const auto customData = ptr.getRefData().getCustomData();
        if(customData && customData->asContainerCustomData().isModified())
            text += "\nmodified";

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
        auto store = getStoreManager(ptr);
        return store.getImmutable().getWeight();
    }

    bool Container::canLock(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Container> *ref = ptr.get<ESM::Container>();
        return !(ref->mBase->mFlags & ESM::Container::Organic);
    }

    void Container::modifyBaseInventory(const std::string& containerId, const std::string& itemId, int amount) const
    {
        MWMechanics::modifyBaseInventory<ESM::Container>(containerId, itemId, amount);
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

        const ESM::ContainerState& containerState = state.asContainerState();
        ptr.getRefData().setCustomData(std::make_unique<ContainerCustomData>(containerState.mInventory).release());
    }

    void Container::writeAdditionalState (const MWWorld::ConstPtr& ptr, ESM::ObjectState& state) const
    {
        if (!ptr.getRefData().getCustomData() || !ptr.getRefData().getCustomData()->asContainerCustomData().isModified())
        {
            state.mHasCustomState = false;
            return;
        }

        const ContainerCustomData& customData = ptr.getRefData().getCustomData()->asContainerCustomData();
        ESM::ContainerState& containerState = state.asContainerState();
        customData.mResolvedStore->writeState (containerState.mInventory);
    }
}
