
#include "apparatus.hpp"

#include <components/esm/loadappa.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/actionalchemy.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/physicssystem.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/window_manager.hpp"
#include "../mwgui/tooltips.hpp"

#include "../mwsound/soundmanager.hpp"

namespace MWClass
{
    void Apparatus::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        const std::string model = getModel(ptr);
        if (!model.empty()) {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, model);
        }
    }

    void Apparatus::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty()) {
            physics.insertObjectPhysics(ptr, model);
        }
    }
    
    std::string Apparatus::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Apparatus> *ref =
            ptr.get<ESM::Apparatus>();
        assert(ref->base != NULL);

        const std::string &model = ref->base->model;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Apparatus::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Apparatus> *ref =
            ptr.get<ESM::Apparatus>();

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Apparatus::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        MWBase::Environment::get().getSoundManager()->playSound3D (ptr, getUpSoundId(ptr), 1.0, 1.0, MWSound::Play_NoTrack);

        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionTake (ptr));
    }

    std::string Apparatus::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Apparatus> *ref =
            ptr.get<ESM::Apparatus>();

        return ref->base->script;
    }

    int Apparatus::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Apparatus> *ref =
            ptr.get<ESM::Apparatus>();

        return ref->base->data.value;
    }

    void Apparatus::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Apparatus);

        registerClass (typeid (ESM::Apparatus).name(), instance);
    }

    std::string Apparatus::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        return std::string("Item Apparatus Up");
    }

    std::string Apparatus::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        return std::string("Item Apparatus Down");
    }

    std::string Apparatus::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
          MWWorld::LiveCellRef<ESM::Apparatus> *ref =
            ptr.get<ESM::Apparatus>();

        return ref->base->icon;
    }

    bool Apparatus::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Apparatus> *ref =
            ptr.get<ESM::Apparatus>();

        return (ref->base->name != "");
    }

    MWGui::ToolTipInfo Apparatus::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Apparatus> *ref =
            ptr.get<ESM::Apparatus>();

        MWGui::ToolTipInfo info;
        info.caption = ref->base->name + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        info.icon = ref->base->icon;

        const ESMS::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        std::string text;
        text += "\n" + store.gameSettings.search("sQuality")->str + ": " + MWGui::ToolTips::toString(ref->base->data.quality);
        text += "\n" + store.gameSettings.search("sWeight")->str + ": " + MWGui::ToolTips::toString(ref->base->data.weight);
        text += MWGui::ToolTips::getValueString(ref->base->data.value, store.gameSettings.search("sValue")->str);

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getMiscString(ref->ref.owner, "Owner");
            text += MWGui::ToolTips::getMiscString(ref->base->script, "Script");
        }
        info.text = text;

        return info;
    }


    boost::shared_ptr<MWWorld::Action> Apparatus::use (const MWWorld::Ptr& ptr) const
    {
        return boost::shared_ptr<MWWorld::Action>(new MWWorld::ActionAlchemy());
    }

    MWWorld::Ptr
    Apparatus::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Apparatus> *ref =
            ptr.get<ESM::Apparatus>();

        return MWWorld::Ptr(&cell.appas.insert(*ref), &cell);
    }
}
