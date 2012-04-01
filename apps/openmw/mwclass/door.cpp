
#include "door.hpp"

#include <components/esm/loaddoor.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/player.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/actionteleport.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"

#include "../mwrender/objects.hpp"

#include "../mwsound/soundmanager.hpp"

namespace MWClass
{
    void Door::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
         ESMS::LiveCellRef<ESM::Door, MWWorld::RefData> *ref =
            ptr.get<ESM::Door>();

        assert (ref->base != NULL);
        const std::string &model = ref->base->model;

        if (!model.empty())
        {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, "meshes\\" + model);
        }
    }

    void Door::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics, MWWorld::Environment& environment) const
    {
         ESMS::LiveCellRef<ESM::Door, MWWorld::RefData> *ref =
            ptr.get<ESM::Door>();

        const std::string &model = ref->base->model;
        assert (ref->base != NULL);
        if(!model.empty()){
            physics.insertObjectPhysics(ptr, "meshes\\" + model);
        }
    }

    std::string Door::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Door, MWWorld::RefData> *ref =
            ptr.get<ESM::Door>();

        if (ref->ref.teleport && !ref->ref.destCell.empty()) // TODO doors that lead to exteriors
            return ref->ref.destCell;

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Door::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Door, MWWorld::RefData> *ref =
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
            environment.mSoundManager->playSound3D (ptr, lockedSound, 1.0, 1.0);
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction);
        }

        if(!ptr.getCellRef().trap.empty())
        {
            // Trap activation
            std::cout << "Activated trap: " << ptr.getCellRef().trap << std::endl;
            environment.mSoundManager->playSound3D(ptr, trapActivationSound, 1.0, 1.0);
            ptr.getCellRef().trap = "";
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction);
        }

        if (ref->ref.teleport)
        {
            // teleport door
            if (environment.mWorld->getPlayer().getPlayer()==actor)
            {
                // the player is using the door
                // The reason this is not 3D is that it would get interrupted when you teleport
                environment.mSoundManager->playSound(openSound, 1.0, 1.0);
                return boost::shared_ptr<MWWorld::Action> (
                    new MWWorld::ActionTeleportPlayer (ref->ref.destCell, ref->ref.doorDest));
            }
            else
            {
                // another NPC or a creature is using the door
                // TODO return action for teleporting other NPC/creature
                return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction);
            }
        }
        else
        {
            // animated door
            // TODO return action for rotating the door

            // This is a little pointless, but helps with testing
            environment.mSoundManager->playSound3D (ptr, openSound, 1.0, 1.0);
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
        ESMS::LiveCellRef<ESM::Door, MWWorld::RefData> *ref =
            ptr.get<ESM::Door>();

        return ref->base->script;
    }

    void Door::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Door);

        registerClass (typeid (ESM::Door).name(), instance);
    }
}
