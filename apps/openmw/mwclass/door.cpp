
#include "door.hpp"

#include <components/esm/loaddoor.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/actionteleport.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    void Door::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        const std::string model = getModel(ptr);
        if (!model.empty()) {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, model);
        }
    }

    void Door::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty())
            physics.addObject(ptr);
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

        if (ref->mRef.mTeleport && !ref->mRef.mDestCell.empty()) // TODO doors that lead to exteriors
            return ref->mRef.mDestCell;

        return ref->mBase->mName;
    }

    boost::shared_ptr<MWWorld::Action> Door::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        MWWorld::LiveCellRef<ESM::Door> *ref =
            ptr.get<ESM::Door>();

        const std::string &openSound = ref->mBase->mOpenSound;
        //const std::string &closeSound = ref->mBase->closeSound;
        const std::string lockedSound = "LockedDoor";
        const std::string trapActivationSound = "Disarm Trap Fail";

        MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayer().getPlayer();
        MWWorld::InventoryStore& invStore = MWWorld::Class::get(player).getInventoryStore(player);

        bool needKey = ptr.getCellRef().mLockLevel>0;
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
            MWBase::Environment::get().getWindowManager ()->messageBox (keyName + " #{sKeyUsed}", std::vector<std::string>());
            ptr.getCellRef().mLockLevel = 0;
            // using a key disarms the trap
            ptr.getCellRef().mTrap = "";
        }

        if (!needKey || hasKey)
        {
            if(!ptr.getCellRef().mTrap.empty())
            {
                // Trap activation
                std::cout << "Activated trap: " << ptr.getCellRef().mTrap << std::endl;

                boost::shared_ptr<MWWorld::Action> action(new MWWorld::FailedAction);
                action->setSound(trapActivationSound);
                ptr.getCellRef().mTrap = "";

                return action;
            }

            if (ref->mRef.mTeleport)
            {
                // teleport door
                /// \todo remove this if clause once ActionTeleport can also support other actors
                if (MWBase::Environment::get().getWorld()->getPlayer().getPlayer()==actor)
                {
                    boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionTeleport (ref->mRef.mDestCell, ref->mRef.mDoorDest));

                    action->setSound(openSound);

                    return action;
                }
                else
                {
                    // another NPC or a creature is using the door
                    return boost::shared_ptr<MWWorld::Action> (new MWWorld::FailedAction);
                }
            }
            else
            {
                // animated door
                // TODO return action for rotating the door

                // This is a little pointless, but helps with testing
                boost::shared_ptr<MWWorld::Action> action(new MWWorld::NullAction);

                action->setSound(openSound);

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
        if (lockLevel<0)
            lockLevel = 0;

        ptr.getCellRef().mLockLevel = lockLevel;
    }

    void Door::unlock (const MWWorld::Ptr& ptr) const
    {
        ptr.getCellRef().mLockLevel = 0;
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

        if (ref->mRef.mTeleport)
        {
            text += "\n#{sTo}";
            text += "\n" + getDestination(*ref);
        }

        if (ref->mRef.mLockLevel > 0)
            text += "\n#{sLockLevel}: " + MWGui::ToolTips::toString(ref->mRef.mLockLevel);
        if (ref->mRef.mTrap != "")
            text += "\n#{sTrapped}";

        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");

        info.text = text;

        return info;
    }

    std::string Door::getDestination (const MWWorld::LiveCellRef<ESM::Door>& door)
    {
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        std::string dest;
        if (door.mRef.mDestCell != "")
        {
            // door leads to an interior, use interior name as tooltip
            dest = door.mRef.mDestCell;
        }
        else
        {
            // door leads to exterior, use cell name (if any), otherwise translated region name
            int x,y;
            MWBase::Environment::get().getWorld()->positionToIndex (door.mRef.mDoorDest.pos[0], door.mRef.mDoorDest.pos[1], x, y);
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

        return MWWorld::Ptr(&cell.mDoors.insert(*ref), &cell);
    }
}
