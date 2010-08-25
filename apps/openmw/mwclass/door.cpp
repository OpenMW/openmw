
#include "door.hpp"

#include <components/esm/loaddoor.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwrender/playerpos.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/actionteleport.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"

#include "../mwrender/cellimp.hpp"

namespace MWClass
{
    void Door::insertObj (const MWWorld::Ptr& ptr, MWRender::CellRenderImp& cellRender,
        MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Door, MWWorld::RefData> *ref =
            ptr.get<ESM::Door>();

        assert (ref->base != NULL);
        const std::string &model = ref->base->model;
        if (!model.empty())
        {
            MWRender::Rendering rendering (cellRender, ref->ref);
            cellRender.insertMesh ("meshes\\" + model);
            ref->mData.setHandle (rendering.end (ref->mData.isEnabled()));
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

        if (ref->ref.teleport)
        {
            // teleport door
            if (environment.mWorld->getPlayerPos().getPlayer()==actor)
            {
                // the player is using the door
                return boost::shared_ptr<MWWorld::Action> (
                    new MWWorld::ActionTeleportPlayer (ref->ref.destCell, ref->ref.doorDest));
            }
            else
            {
                // another NPC or a create is using the door
                // TODO return action for teleporting other NPC/creature
                return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction);
            }
        }
        else
        {
            // animated door
            // TODO return action for rotating the door
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction);
        }
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
