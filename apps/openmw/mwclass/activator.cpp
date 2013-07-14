
#include "activator.hpp"

#include <components/esm/loadacti.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld//cellstore.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"

#include "../mwrender/actors.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

namespace MWClass
{
   void Activator::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        const std::string model = getModel(ptr);
        if (!model.empty()) {
            MWRender::Actors& actors = renderingInterface.getActors();
            actors.insertActivator(ptr);
        }
    }

    void Activator::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty())
            physics.addObject(ptr);
        MWBase::Environment::get().getMechanicsManager()->add(ptr);
    }

    std::string Activator::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Activator> *ref =
            ptr.get<ESM::Activator>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Activator::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Activator> *ref =
            ptr.get<ESM::Activator>();

        return ref->mBase->mName;
    }

    std::string Activator::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Activator> *ref =
            ptr.get<ESM::Activator>();

        return ref->mBase->mScript;
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

        return (ref->mBase->mName != "");
    }

    MWGui::ToolTipInfo Activator::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Activator> *ref =
            ptr.get<ESM::Activator>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());

        std::string text;
        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        info.text = text;

        return info;
    }
    
    MWWorld::Ptr
    Activator::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Activator> *ref =
            ptr.get<ESM::Activator>();

        return MWWorld::Ptr(&cell.mActivators.insert(*ref), &cell);
    }
}
