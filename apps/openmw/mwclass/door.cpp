
#include "door.hpp"

#include <components/esm/loaddoor.hpp>
#include <components/esm/doorstate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/actionteleport.hpp"
#include "../mwworld/actiondoor.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/actiontrap.hpp"
#include "../mwworld/customdata.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/actors.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace
{
    struct DoorCustomData : public MWWorld::CustomData
    {
        int mDoorState; // 0 = nothing, 1 = opening, 2 = closing

        virtual MWWorld::CustomData *clone() const;
    };

    MWWorld::CustomData *DoorCustomData::clone() const
    {
        return new DoorCustomData (*this);
    }
}

namespace MWClass
{
    std::string Door::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM::Door>()->mBase->mId;
    }

    void Door::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty()) {
            MWRender::Actors& actors = renderingInterface.getActors();
            actors.insertActivator(ptr, model);
        }
    }

    void Door::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);

        // Resume the door's opening/closing animation if it wasn't finished
        if (ptr.getRefData().getCustomData())
        {
            const DoorCustomData& customData = dynamic_cast<const DoorCustomData&>(*ptr.getRefData().getCustomData());
            if (customData.mDoorState > 0)
            {
                MWBase::Environment::get().getWorld()->activateDoor(ptr, customData.mDoorState);
            }
        }

        MWBase::Environment::get().getMechanicsManager()->add(ptr);
    }

    std::string Door::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Door> *ref =
            ptr.get<ESM::Door>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Door::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Door> *ref =
            ptr.get<ESM::Door>();

        return ref->mBase->mName;
    }

    boost::shared_ptr<MWWorld::Action> Door::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        MWWorld::LiveCellRef<ESM::Door> *ref = ptr.get<ESM::Door>();

        const std::string &openSound = ref->mBase->mOpenSound;
        const std::string &closeSound = ref->mBase->mCloseSound;
        const std::string lockedSound = "LockedDoor";
        const std::string trapActivationSound = "Disarm Trap Fail";

        MWWorld::ContainerStore &invStore = actor.getClass().getContainerStore(actor);

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
            if(actor == MWBase::Environment::get().getWorld()->getPlayerPtr())
                MWBase::Environment::get().getWindowManager()->messageBox(keyName + " #{sKeyUsed}");
            unlock(ptr); //Call the function here. because that makes sense.
            // using a key disarms the trap
            ptr.getCellRef().setTrap("");
        }

        if (!needKey || hasKey)
        {
            if(!ptr.getCellRef().getTrap().empty())
            {
                // Trap activation
                boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionTrap(actor, ptr.getCellRef().getTrap(), ptr));
                action->setSound(trapActivationSound);
                return action;
            }

            if (ptr.getCellRef().getTeleport())
            {
                boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionTeleport (ptr.getCellRef().getDestCell(), ptr.getCellRef().getDoorDest()));

                action->setSound(openSound);

                return action;
            }
            else
            {
                // animated door
                boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionDoor(ptr));
                int doorstate = getDoorState(ptr);
                bool opening = true;
                if (doorstate == 1)
                    opening = false;
                if (doorstate == 0 && ptr.getRefData().getLocalRotation().rot[2] != 0)
                    opening = false;

                if (opening)
                {
                    MWBase::Environment::get().getSoundManager()->fadeOutSound3D(ptr,
                            closeSound, 0.5f);
                    float offset = ptr.getRefData().getLocalRotation().rot[2]/ 3.14159265f * 2.0f;
                    action->setSoundOffset(offset);
                    action->setSound(openSound);
                }
                else
                {
                    MWBase::Environment::get().getSoundManager()->fadeOutSound3D(ptr,
                                                openSound, 0.5f);
                    float offset = 1.0f - ptr.getRefData().getLocalRotation().rot[2]/ 3.14159265f * 2.0f;
                    //most if not all door have closing bang somewhere in the middle of the sound,
                    //so we divide offset by two
                    action->setSoundOffset(offset * 0.5f);
                    action->setSound(closeSound);
                }

                return action;
            }
        }
        else
        {
            // locked, and we can't open.
            boost::shared_ptr<MWWorld::Action> action(new MWWorld::FailedAction);
            action->setSound(lockedSound);
            return action;
        }
    }

    void Door::lock (const MWWorld::Ptr& ptr, int lockLevel) const
    {
        if(lockLevel!=0)
            ptr.getCellRef().setLockLevel(abs(lockLevel)); //Changes lock to locklevel, in positive
        else
            ptr.getCellRef().setLockLevel(abs(ptr.getCellRef().getLockLevel())); //No locklevel given, just flip the origional one
    }

    void Door::unlock (const MWWorld::Ptr& ptr) const
    {
        ptr.getCellRef().setLockLevel(-abs(ptr.getCellRef().getLockLevel())); //Makes lockLevel negative
    }

    std::string Door::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Door> *ref =
            ptr.get<ESM::Door>();

        return ref->mBase->mScript;
    }

    void Door::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Door);

        registerClass (typeid (ESM::Door).name(), instance);
    }

    bool Door::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Door> *ref =
            ptr.get<ESM::Door>();

        return (ref->mBase->mName != "");
    }

    MWGui::ToolTipInfo Door::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Door> *ref =
            ptr.get<ESM::Door>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mName;

        std::string text;

        if (ptr.getCellRef().getTeleport())
        {
            text += "\n#{sTo}";
            text += "\n" + getDestination(*ref);
        }

        if (ptr.getCellRef().getLockLevel() > 0)
            text += "\n#{sLockLevel}: " + MWGui::ToolTips::toString(ptr.getCellRef().getLockLevel());
        else if (ptr.getCellRef().getLockLevel() < 0)
            text += "\n#{sUnlocked}";
        if (ptr.getCellRef().getTrap() != "")
            text += "\n#{sTrapped}";

        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
        {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }
        info.text = text;

        return info;
    }

    std::string Door::getDestination (const MWWorld::LiveCellRef<ESM::Door>& door)
    {
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        std::string dest;
        if (door.mRef.getDestCell() != "")
        {
            // door leads to an interior, use interior name as tooltip
            dest = door.mRef.getDestCell();
        }
        else
        {
            // door leads to exterior, use cell name (if any), otherwise translated region name
            int x,y;
            MWBase::Environment::get().getWorld()->positionToIndex (door.mRef.getDoorDest().pos[0], door.mRef.getDoorDest().pos[1], x, y);
            const ESM::Cell* cell = store.get<ESM::Cell>().find(x,y);
            if (cell->mName != "")
                dest = cell->mName;
            else
            {
                const ESM::Region* region =
                    store.get<ESM::Region>().find(cell->mRegion);

                //name as is, not a token
                return region->mName;
            }
        }

        return "#{sCell=" + dest + "}";
    }

    MWWorld::Ptr
    Door::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Door> *ref =
            ptr.get<ESM::Door>();

        return MWWorld::Ptr(&cell.get<ESM::Door>().insert(*ref), &cell);
    }

    void Door::ensureCustomData(const MWWorld::Ptr &ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            std::auto_ptr<DoorCustomData> data(new DoorCustomData);

            data->mDoorState = 0;
            ptr.getRefData().setCustomData(data.release());
        }
    }

    int Door::getDoorState (const MWWorld::Ptr &ptr) const
    {
        ensureCustomData(ptr);
        const DoorCustomData& customData = dynamic_cast<const DoorCustomData&>(*ptr.getRefData().getCustomData());
        return customData.mDoorState;
    }

    void Door::setDoorState (const MWWorld::Ptr &ptr, int state) const
    {
        if (ptr.getCellRef().getTeleport())
            throw std::runtime_error("load doors can't be moved");

        ensureCustomData(ptr);
        DoorCustomData& customData = dynamic_cast<DoorCustomData&>(*ptr.getRefData().getCustomData());
        customData.mDoorState = state;
    }

    void Door::readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state) const
    {
        ensureCustomData(ptr);
        DoorCustomData& customData = dynamic_cast<DoorCustomData&>(*ptr.getRefData().getCustomData());

        const ESM::DoorState& state2 = dynamic_cast<const ESM::DoorState&>(state);
        customData.mDoorState = state2.mDoorState;
    }

    void Door::writeAdditionalState (const MWWorld::Ptr& ptr, ESM::ObjectState& state) const
    {
        ensureCustomData(ptr);
        const DoorCustomData& customData = dynamic_cast<const DoorCustomData&>(*ptr.getRefData().getCustomData());

        ESM::DoorState& state2 = dynamic_cast<ESM::DoorState&>(state);
        state2.mDoorState = customData.mDoorState;
    }

}
