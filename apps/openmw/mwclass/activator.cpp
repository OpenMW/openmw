
#include "activator.hpp"

#include <components/esm/loadacti.hpp>

#include "../mwbase/environment.hpp"

#include "../mwworld//cellstore.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/window_manager.hpp"
#include "../mwgui/tooltips.hpp"

namespace MWClass
{
   void Activator::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        const std::string model = getModel(ptr);
        if (!model.empty()) {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, model);
        }
    }

    void Activator::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty()) {
            physics.insertObjectPhysics(ptr, model);
        }
    }
    
    std::string Activator::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Activator> *ref =
            ptr.get<ESM::Activator>();
        assert(ref->base != NULL);

        const std::string &model = ref->base->model;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Activator::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Activator> *ref =
            ptr.get<ESM::Activator>();

        return ref->base->name;
    }

    std::string Activator::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Activator> *ref =
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
        MWWorld::LiveCellRef<ESM::Activator> *ref =
            ptr.get<ESM::Activator>();

        return (ref->base->name != "");
    }

    MWGui::ToolTipInfo Activator::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Activator> *ref =
            ptr.get<ESM::Activator>();

        MWGui::ToolTipInfo info;
        info.caption = ref->base->name + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());

        std::string text;
        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
            text += MWGui::ToolTips::getMiscString(ref->base->script, "Script");
        info.text = text;

        return info;
    }

    MWWorld::Ptr
    Activator::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Activator> *ref =
            ptr.get<ESM::Activator>();

        return MWWorld::Ptr(&cell.activators.insert(*ref), &cell);
    }
}

