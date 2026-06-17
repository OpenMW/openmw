#include "container.hpp"

#include <MyGUI_TextIterator.h>
#include <MyGUI_UString.h>

#include <components/esm3/containerstate.hpp>
#include <components/esm3/loadcont.hpp>
#include <components/esm3/loadsoun.hpp>
#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwphysics/physicssystem.hpp"
#include "../mwworld/actionharvest.hpp"
#include "../mwworld/actionopen.hpp"
#include "../mwworld/actiontrap.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/worldmodel.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/animation.hpp"
#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwmechanics/inventory.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "classmodel.hpp"
#include "nameorid.hpp"

namespace MWClass
{
    ContainerCustomData::ContainerCustomData(const ESM::Container& container, MWWorld::CellStore* cell)
    {
        auto& prng = MWBase::Environment::get().getWorld()->getPrng();
        unsigned int seed = Misc::Rng::rollDice(std::numeric_limits<int>::max(), prng);
        // setting ownership not needed, since taking items from a container inherits the
        // container's owner automatically
        mStore.fillNonRandom(container.mInventory, ESM::RefId(), seed);
    }

    ContainerCustomData::ContainerCustomData(const ESM::InventoryState& inventory)
    {
        mStore.readState(inventory);
    }

    ContainerCustomData& ContainerCustomData::asContainerCustomData()
    {
        return *this;
    }
    const ContainerCustomData& ContainerCustomData::asContainerCustomData() const
    {
        return *this;
    }

    Container::Container()
        : MWWorld::RegisteredClass<Container>(ESM::Container::sRecordId)
    {
    }

    void Container::ensureCustomData(const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            MWBase::Environment::get().getWorldModel()->registerPtr(ptr);
            MWWorld::LiveCellRef<ESM::Container>* ref = ptr.get<ESM::Container>();

            // store
            ptr.getRefData().setCustomData(std::make_unique<ContainerCustomData>(*ref->mBase, ptr.getCell()));
            getContainerStore(ptr).setPtr(ptr);

            MWBase::Environment::get().getWorld()->addContainerScripts(ptr, ptr.getCell());
        }
    }

    bool Container::canBeHarvested(const MWWorld::ConstPtr& ptr) const
    {
        if (!Settings::game().mGraphicHerbalism)
            return false;
        const MWRender::Animation* animation = MWBase::Environment::get().getWorld()->getAnimation(ptr);
        if (animation == nullptr)
            return false;

        return animation->canBeHarvested();
    }

    void Container::respawn(const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Container>* ref = ptr.get<ESM::Container>();
        if (ref->mBase->mFlags & ESM::Container::Respawn)
        {
            // Container was not touched, there is no need to modify its content.
            if (ptr.getRefData().getCustomData() == nullptr)
                return;

            MWBase::Environment::get().getWorld()->removeContainerScripts(ptr);
            ptr.getRefData().setCustomData(nullptr);
        }
    }

    void Container::insertObjectRendering(
        const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty())
        {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    void Container::insertObject(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
        MWPhysics::PhysicsSystem& physics) const
    {
        insertObjectPhysics(ptr, model, rotation, physics);
    }

    void Container::insertObjectPhysics(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
        MWPhysics::PhysicsSystem& physics) const
    {
        physics.addObject(ptr, VFS::Path::toNormalized(model), rotation, MWPhysics::CollisionType_World);
    }

    std::string_view Container::getModel(const MWWorld::ConstPtr& ptr) const
    {
        return getClassModel<ESM::Container>(ptr);
    }

    bool Container::useAnim() const
    {
        return true;
    }

    std::unique_ptr<MWWorld::Action> Container::activate(const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor) const
    {
        if (!MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Inventory))
            return std::make_unique<MWWorld::NullAction>();

        if (actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
        {
            const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
            auto& prng = MWBase::Environment::get().getWorld()->getPrng();
            const ESM::Sound* sound = store.get<ESM::Sound>().searchRandom("WolfContainer", prng);

            std::unique_ptr<MWWorld::Action> action = std::make_unique<MWWorld::FailedAction>("#{sWerewolfRefusal}");
            if (sound)
                action->setSound(sound->mId);

            return action;
        }

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWWorld::InventoryStore& invStore = player.getClass().getInventoryStore(player);

        bool isLocked = ptr.getCellRef().isLocked();
        bool isTrapped = !ptr.getCellRef().getTrap().empty();
        bool hasKey = false;
        std::string_view keyName;

        const ESM::RefId& keyId = ptr.getCellRef().getKey();
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
            MWBase::Environment::get().getWindowManager()->messageBox(std::string{ keyName } + " #{sKeyUsed}");
            ptr.getCellRef().unlock();
            // using a key disarms the trap
            if (isTrapped)
            {
                ptr.getCellRef().setTrap(ESM::RefId());
                MWBase::Environment::get().getSoundManager()->playSound3D(
                    ptr, ESM::RefId::stringRefId("Disarm Trap"), 1.0f, 1.0f);
                isTrapped = false;
            }
        }

        if (!isLocked || hasKey)
        {
            if (!isTrapped)
            {
                if (!canBeHarvested(ptr))
                    return std::make_unique<MWWorld::ActionOpen>(ptr);

                if (hasToolTip(ptr))
                    return std::make_unique<MWWorld::ActionHarvest>(ptr);

                return std::make_unique<MWWorld::FailedAction>(std::string_view{}, ptr);
            }
            else
            {
                // Activate trap
                std::unique_ptr<MWWorld::Action> action
                    = std::make_unique<MWWorld::ActionTrap>(ptr.getCellRef().getTrap(), ptr);
                action->setSound(ESM::RefId::stringRefId("Disarm Trap Fail"));
                return action;
            }
        }
        else
        {
            std::unique_ptr<MWWorld::Action> action = std::make_unique<MWWorld::FailedAction>(std::string_view{}, ptr);
            action->setSound(ESM::RefId::stringRefId("LockedChest"));
            return action;
        }
    }

    std::string_view Container::getName(const MWWorld::ConstPtr& ptr) const
    {
        return getNameOrId<ESM::Container>(ptr);
    }

    MWWorld::ContainerStore& Container::getContainerStore(const MWWorld::Ptr& ptr) const
    {
        ensureCustomData(ptr);
        return ptr.getRefData().getCustomData()->asContainerCustomData().mStore;
    }

    ESM::RefId Container::getScript(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Container>* ref = ptr.get<ESM::Container>();

        return ref->mBase->mScript;
    }

    bool Container::hasToolTip(const MWWorld::ConstPtr& ptr) const
    {
        if (const MWWorld::CustomData* data = ptr.getRefData().getCustomData())
        {
            if (!canBeHarvested(ptr))
                return true;
            const MWWorld::ContainerStore& store = data->asContainerCustomData().mStore;
            return !store.isResolved() || store.hasVisibleItems();
        }
        return true;
    }

    MWGui::ToolTipInfo Container::getToolTipInfo(const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Container>* ref = ptr.get<ESM::Container>();

        MWGui::ToolTipInfo info;
        std::string_view name = getName(ptr);
        info.caption = MyGUI::TextIterator::toTagsString(MyGUI::UString(name));

        std::string text;
        int lockLevel = ptr.getCellRef().getLockLevel();
        if (lockLevel)
        {
            if (ptr.getCellRef().isLocked())
                text += "\n#{sLockLevel}: " + MWGui::ToolTips::toString(lockLevel);
            else
                text += "\n#{sUnlocked}";
        }
        if (ptr.getCellRef().getTrap() != ESM::RefId())
            text += "\n#{sTrapped}";

        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
        {
            info.extra += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            info.extra += MWGui::ToolTips::getMiscString(ref->mBase->mScript.getRefIdString(), "Script");
            if (ptr.getCellRef().getRefId() == "stolen_goods")
                info.extra += "\nYou cannot use evidence chests";
        }

        info.text = std::move(text);

        return info;
    }

    float Container::getCapacity(const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Container>* ref = ptr.get<ESM::Container>();

        return ref->mBase->mWeight;
    }

    float Container::getEncumbrance(const MWWorld::Ptr& ptr) const
    {
        return getContainerStore(ptr).getWeight();
    }

    bool Container::canLock(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Container>* ref = ptr.get<ESM::Container>();
        return !(ref->mBase->mFlags & ESM::Container::Organic);
    }

    void Container::modifyBaseInventory(const ESM::RefId& containerId, const ESM::RefId& itemId, int amount) const
    {
        MWMechanics::modifyBaseInventory<ESM::Container>(containerId, itemId, amount);
    }

    MWWorld::Ptr Container::copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const
    {
        const MWWorld::LiveCellRef<ESM::Container>* ref = ptr.get<ESM::Container>();
        MWWorld::Ptr newPtr(cell.insert(ref), &cell);
        if (newPtr.getRefData().getCustomData())
        {
            MWBase::Environment::get().getWorldModel()->registerPtr(newPtr);
            getContainerStore(newPtr).setPtr(newPtr);
        }
        return newPtr;
    }

    void Container::readAdditionalState(const MWWorld::Ptr& ptr, const ESM::ObjectState& state) const
    {
        if (!state.mHasCustomState)
            return;

        const ESM::ContainerState& containerState = state.asContainerState();
        ptr.getRefData().setCustomData(std::make_unique<ContainerCustomData>(containerState.mInventory));

        MWBase::Environment::get().getWorldModel()->registerPtr(ptr);
        getContainerStore(ptr).setPtr(ptr);
    }

    void Container::writeAdditionalState(const MWWorld::ConstPtr& ptr, ESM::ObjectState& state) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            state.mHasCustomState = false;
            return;
        }

        const ContainerCustomData& customData = ptr.getRefData().getCustomData()->asContainerCustomData();
        if (!customData.mStore.isResolved())
        {
            state.mHasCustomState = false;
            return;
        }

        ESM::ContainerState& containerState = state.asContainerState();
        customData.mStore.writeState(containerState.mInventory);
    }
}
