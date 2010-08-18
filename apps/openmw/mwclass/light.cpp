
#include "light.hpp"

#include <components/esm/loadligh.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/environment.hpp"

#include "../mwrender/cellimp.hpp"

#include "../mwsound/soundmanager.hpp"

#include "containerutil.hpp"

namespace MWClass
{
    void Light::insertObj (const MWWorld::Ptr& ptr, MWRender::CellRenderImp& cellRender,
        MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Light, MWWorld::RefData> *ref =
            ptr.get<ESM::Light>();

        assert (ref->base != NULL);
        const std::string &model = ref->base->model;
        if (!model.empty())
        {
            cellRender.insertBegin (ref->ref);

            cellRender.insertMesh ("meshes\\" + model);

            // Extract the color and convert to floating point
            const int color = ref->base->data.color;
            const float r = ((color >>  0) & 0xFF) / 255.0f;
            const float g = ((color >>  8) & 0xFF) / 255.0f;
            const float b = ((color >> 16) & 0xFF) / 255.0f;
            const float radius = float (ref->base->data.radius);
            cellRender.insertLight (r, g, b, radius);

            ref->mData.setHandle (cellRender.insertEnd (ref->mData.isEnabled()));
        }
    }

    void Light::enable (const MWWorld::Ptr& ptr, MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Light, MWWorld::RefData> *ref =
            ptr.get<ESM::Light>();

        if (!ref->base->sound.empty())
        {
            environment.mSoundManager->playSound3D (ptr, ref->base->sound, 1.0, 1.0, true);
        }
    }

    std::string Light::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Light, MWWorld::RefData> *ref =
            ptr.get<ESM::Light>();

        if (ref->base->model.empty())
            return "";

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Light::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Light, MWWorld::RefData> *ref =
            ptr.get<ESM::Light>();

        if (!(ref->base->data.flags & ESM::Light::Carry))
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction);

        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionTake (ptr));
    }

    void Light::insertIntoContainer (const MWWorld::Ptr& ptr,
        MWWorld::ContainerStore<MWWorld::RefData>& containerStore) const
    {
        insertIntoContainerStore (ptr, containerStore.lights);
    }

    std::string Light::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Light, MWWorld::RefData> *ref =
            ptr.get<ESM::Light>();

        return ref->base->script;
    }

    void Light::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Light);

        registerClass (typeid (ESM::Light).name(), instance);
    }
}
