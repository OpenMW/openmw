
#include "door.hpp"

#include <components/esm/loaddoor.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/actionteleport.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/physicssystem.hpp"

#include "../mwgui/window_manager.hpp"
#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwsound/soundmanager.hpp"

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

        const std::string &model = ref->base->model;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Door::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Door> *ref =
            ptr.get<ESM::Door>();

        if (ref->ref.teleport && !ref->ref.destCell.empty()) // TODO doors that lead to exteriors
            return ref->ref.destCell;

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Door::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        MWWorld::LiveCellRef<ESM::Door> *ref =
            ptr.get<ESM::Door>();

        const std::string &openSound = ref->base->openSound;
        //const std::string &closeSound = ref->base->closeSound;
        const std::string lockedSound = "LockedDoor";
        const std::string trapActivationSound = "Disarm Trap Fail";

        if (ptr.getCellRef().lockLevel>0)
        {
            // TODO check for key
            // TODO report failure to player (message, sound?). Look up behaviour of original MW.
            std::cout << "Locked!" << std::endl;
            MWBase::Environment::get().getSoundManager()->playSound3D (ptr, lockedSound, 1.0, 1.0);
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction);
        }

        if(!ptr.getCellRef().trap.empty())
        {
            // Trap activation
            std::cout << "Activated trap: " << ptr.getCellRef().trap << std::endl;
            MWBase::Environment::get().getSoundManager()->playSound3D(ptr, trapActivationSound, 1.0, 1.0);
            ptr.getCellRef().trap = "";
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction);
        }

        if (ref->ref.teleport)
        {
            // teleport door
            /// \todo remove this if clause once ActionTeleport can also support other actors
            if (MWBase::Environment::get().getWorld()->getPlayer().getPlayer()==actor)
            {
                // the player is using the door
                // The reason this is not 3D is that it would get interrupted when you teleport
                MWBase::Environment::get().getSoundManager()->playSound(openSound, 1.0, 1.0);
                return boost::shared_ptr<MWWorld::Action> (
                    new MWWorld::ActionTeleport (ref->ref.destCell, ref->ref.doorDest));
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
            MWBase::Environment::get().getSoundManager()->playSound3D (ptr, openSound, 1.0, 1.0);
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction);
        }
    }

    void Door::lock (const MWWorld::Ptr& ptr, int lockLevel) const
    {
        if (lockLevel<0)
            lockLevel = 0;

        ptr.getCellRef().lockLevel = lockLevel;
    }

    void Door::unlock (const MWWorld::Ptr& ptr) const
    {
        ptr.getCellRef().lockLevel = 0;
    }

    std::string Door::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Door> *ref =
            ptr.get<ESM::Door>();

        return ref->base->script;
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

        return (ref->base->name != "");
    }

    MWGui::ToolTipInfo Door::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Door> *ref =
            ptr.get<ESM::Door>();

        MWGui::ToolTipInfo info;
        info.caption = ref->base->name;

        std::string text;

        const ESMS::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        if (ref->ref.teleport)
        {
            std::string dest;
            if (ref->ref.destCell != "")
            {
                // door leads to an interior, use interior name as tooltip
                dest = ref->ref.destCell;
            }
            else
            {
                // door leads to exterior, use cell name (if any), otherwise translated region name
                int x,y;
                MWBase::Environment::get().getWorld()->positionToIndex (ref->ref.doorDest.pos[0], ref->ref.doorDest.pos[1], x, y);
                const ESM::Cell* cell = store.cells.findExt(x,y);
                if (cell->name != "")
                    dest = cell->name;
                else
                {
                    const ESM::Region* region = store.regions.search(cell->region);
                    dest = region->name;
                }
            }
            text += "\n" + store.gameSettings.search("sTo")->str;
            text += "\n"+dest;
        }

        if (ref->ref.lockLevel > 0)
            text += "\n" + store.gameSettings.search("sLockLevel")->str + ": " + MWGui::ToolTips::toString(ref->ref.lockLevel);
        if (ref->ref.trap != "")
            text += "\n" + store.gameSettings.search("sTrapped")->str;

        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
            text += MWGui::ToolTips::getMiscString(ref->base->script, "Script");

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
