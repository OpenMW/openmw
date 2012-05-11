
#include "activator.hpp"

#include <components/esm/loadacti.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwrender/objects.hpp"
#include "../mwbase/environment.hpp"
#include "../mwgui/window_manager.hpp"

namespace MWClass
{
   void Activator::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        ESMS::LiveCellRef<ESM::Activator, MWWorld::RefData> *ref =
            ptr.get<ESM::Activator>();

        assert (ref->base != NULL);
        const std::string &model = ref->base->model;

        if (!model.empty())
        {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, "meshes\\" + model);
        }
    }

    void Activator::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        ESMS::LiveCellRef<ESM::Activator, MWWorld::RefData> *ref =
            ptr.get<ESM::Activator>();


        const std::string &model = ref->base->model;
        assert (ref->base != NULL);
        if(!model.empty()){
            physics.insertObjectPhysics(ptr, "meshes\\" + model);
        }

    }

    std::string Activator::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Activator, MWWorld::RefData> *ref =
            ptr.get<ESM::Activator>();

        return ref->base->name;
    }

    std::string Activator::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Activator, MWWorld::RefData> *ref =
            ptr.get<ESM::Activator>();

        return ref->base->script;
    }

    void Activator::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Activator);

        registerClass (typeid (ESM::Activator).name(), instance);
    }

    bool Activator::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Activator, MWWorld::RefData> *ref =
            ptr.get<ESM::Activator>();

        return (ref->base->name != "");
    }

    MWGui::ToolTipInfo Activator::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Activator, MWWorld::RefData> *ref =
            ptr.get<ESM::Activator>();

        MWGui::ToolTipInfo info;
        info.caption = ref->base->name;

        std::string text;
        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
            text += MWGui::ToolTips::getMiscString(ref->base->script, "Script");
        info.text = text;

        return info;
    }
}
