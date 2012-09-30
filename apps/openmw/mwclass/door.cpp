
#include "door.hpp"

#include <components/esm/loaddoor.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/nullaction.hpp"
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
        if(!model.empty()) {
            physics.insertObjectPhysics(ptr, model);
        }
    }

    std::string Door::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Door> *ref =
            ptr.get<ESM::Door>();
        assert(ref->base != NULL);

        const std::string &model = ref->base->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Door::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Door> *ref =
            ptr.get<ESM::Door>();

        if (ref->ref.mTeleport && !ref->ref.mDestCell.empty()) // TODO doors that lead to exteriors
            return ref->ref.mDestCell;

        return ref->base->mName;
    }

    boost::shared_ptr<MWWorld::Action> Door::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        MWWorld::LiveCellRef<ESM::Door> *ref =
            ptr.get<ESM::Door>();

        const std::string &openSound = ref->base->mOpenSound;
        //const std::string &closeSound = ref->base->closeSound;
        const std::string lockedSound = "LockedDoor";
        const std::string trapActivationSound = "Disarm Trap Fail";

        MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayer().getPlayer();
        MWWorld::InventoryStore& invStore = MWWorld::Class::get(player).getInventoryStore(player);

        bool needKey = ptr.getCellRef().mLockLevel>0;
        bool hasKey = false;
        std::string keyName;
        for (MWWorld::ContainerStoreIterator it = invStore.begin(); it != invStore.end(); ++it)
        {
            if (it->getCellRef ().mRefID == ptr.getCellRef().mKey)
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

                boost::shared_ptr<MWWorld::Action> action(new MWWorld::NullAction);

                action->setSound(trapActivationSound);
                ptr.getCellRef().mTrap = "";

                return action;
            }

            if (ref->ref.mTeleport)
            {
                // teleport door
                /// \todo remove this if clause once ActionTeleport can also support other actors
                if (MWBase::Environment::get().getWorld()->getPlayer().getPlayer()==actor)
                {
                    boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionTeleport (ref->ref.mDestCell, ref->ref.mDoorDest));

                    action->setSound(openSound);

                    return action;
                }
                else
                {
                    // another NPC or a creature is using the door
                    return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction);
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
            boost::shared_ptr<MWWorld::Action> action(new MWWorld::NullAction);
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

        return ref->base->mScript;
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

        return (ref->base->mName != "");
    }

    MWGui::ToolTipInfo Door::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Door> *ref =
            ptr.get<ESM::Door>();

        MWGui::ToolTipInfo info;
        info.caption = ref->base->mName;

        std::string text;

        const ESMS::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        if (ref->ref.mTeleport)
        {
            std::string dest;
            if (ref->ref.mDestCell != "")
            {
                // door leads to an interior, use interior name as tooltip
                dest = ref->ref.mDestCell;
            }
            else
            {
                // door leads to exterior, use cell name (if any), otherwise translated region name
                int x,y;
                MWBase::Environment::get().getWorld()->positionToIndex (ref->ref.mDoorDest.pos[0], ref->ref.mDoorDest.pos[1], x, y);
                const ESM::Cell* cell = store.cells.findExt(x,y);
                if (cell->mName != "")
                    dest = cell->mName;
                else
                {
                    const ESM::Region* region = store.regions.search(cell->mRegion);
                    dest = region->mName;
                }
            }
            text += "\n#{sTo}";
            text += "\n"+dest;
        }

        if (ref->ref.mLockLevel > 0)
            text += "\n#{sLockLevel}: " + MWGui::ToolTips::toString(ref->ref.mLockLevel);
        if (ref->ref.mTrap != "")
            text += "\n#{sTrapped}";

        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
            text += MWGui::ToolTips::getMiscString(ref->base->mScript, "Script");

        info.text = text;

        return info;
    }

    MWWorld::Ptr
    Door::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Door> *ref =
            ptr.get<ESM::Door>();

        return MWWorld::Ptr(&cell.doors.insert(*ref), &cell);
    }
}
