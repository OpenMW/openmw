
#include "misc.hpp"

#include <components/esm/loadmisc.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/environment.hpp"

#include "../mwrender/objects.hpp"

#include "../mwsound/soundmanager.hpp"

namespace MWClass
{
    void Miscellaneous::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        ESMS::LiveCellRef<ESM::Miscellaneous, MWWorld::RefData> *ref =
            ptr.get<ESM::Miscellaneous>();

        assert (ref->base != NULL);
        const std::string &model = ref->base->model;

        if (!model.empty())
        {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, "meshes\\" + model);
        }
    }

    void Miscellaneous::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics, MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Miscellaneous, MWWorld::RefData> *ref =
            ptr.get<ESM::Miscellaneous>();


        const std::string &model = ref->base->model;
        assert (ref->base != NULL);
        if(!model.empty()){
            physics.insertObjectPhysics(ptr, "meshes\\" + model);
        }

    }

    std::string Miscellaneous::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Miscellaneous, MWWorld::RefData> *ref =
            ptr.get<ESM::Miscellaneous>();

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Miscellaneous::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const
    {
        environment.mSoundManager->playSound3D (ptr, getUpSoundId(ptr, environment), 1.0, 1.0, MWSound::Play_NoTrack);

        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionTake (ptr));
    }

    std::string Miscellaneous::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Miscellaneous, MWWorld::RefData> *ref =
            ptr.get<ESM::Miscellaneous>();

        return ref->base->script;
    }

    void Miscellaneous::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Miscellaneous);

        registerClass (typeid (ESM::Miscellaneous).name(), instance);
    }

    std::string Miscellaneous::getUpSoundId (const MWWorld::Ptr& ptr, const MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Miscellaneous, MWWorld::RefData> *ref =
            ptr.get<ESM::Miscellaneous>();

        if (ref->base->name =="Gold")
        {
            return std::string("Item Gold Up");
        }
        return std::string("Item Misc Up");
    }

    std::string Miscellaneous::getDownSoundId (const MWWorld::Ptr& ptr, const MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Miscellaneous, MWWorld::RefData> *ref =
            ptr.get<ESM::Miscellaneous>();

        if (ref->base->name =="Gold")
        {
            return std::string("Item Gold Down");
        }
        return std::string("Item Misc Down");
    }
}
