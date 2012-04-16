
#include "apparatus.hpp"

#include <components/esm/loadappa.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"

#include "../mwrender/objects.hpp"

#include "../mwgui/window_manager.hpp"
#include "../mwgui/tooltips.hpp"

#include "../mwsound/soundmanager.hpp"

namespace MWClass
{
   void Apparatus::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        ESMS::LiveCellRef<ESM::Apparatus, MWWorld::RefData> *ref =
            ptr.get<ESM::Apparatus>();

        assert (ref->base != NULL);
        const std::string &model = ref->base->model;

        if (!model.empty())
        {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, "meshes\\" + model);
        }
    }

    void Apparatus::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics, MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Apparatus, MWWorld::RefData> *ref =
            ptr.get<ESM::Apparatus>();


        const std::string &model = ref->base->model;
        assert (ref->base != NULL);
        if(!model.empty()){
            physics.insertObjectPhysics(ptr, "meshes\\" + model);
        }

    }

    std::string Apparatus::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Apparatus, MWWorld::RefData> *ref =
            ptr.get<ESM::Apparatus>();

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Apparatus::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const
    {
        environment.mSoundManager->playSound3D (ptr, getUpSoundId(ptr), 1.0, 1.0, MWSound::Play_NoTrack);

        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionTake (ptr));
    }

    std::string Apparatus::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Apparatus, MWWorld::RefData> *ref =
            ptr.get<ESM::Apparatus>();

        return ref->base->script;
    }

    int Apparatus::getValue (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Apparatus, MWWorld::RefData> *ref =
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

    bool Apparatus::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Apparatus, MWWorld::RefData> *ref =
            ptr.get<ESM::Apparatus>();

        return (ref->base->name != "");
    }

    MWGui::ToolTipInfo Apparatus::getToolTipInfo (const MWWorld::Ptr& ptr, MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Apparatus, MWWorld::RefData> *ref =
            ptr.get<ESM::Apparatus>();

        MWGui::ToolTipInfo info;
        info.caption = ref->base->name;
        info.icon = ref->base->icon;

        std::string text;
        text += "\n" + environment.mWorld->getStore().gameSettings.search("sQuality")->str + ": " + MWGui::ToolTips::toString(ref->base->data.quality);
        text += "\n" + environment.mWorld->getStore().gameSettings.search("sWeight")->str + ": " + MWGui::ToolTips::toString(ref->base->data.weight);
        text += MWGui::ToolTips::getValueString(ref->base->data.value, environment.mWorld->getStore().gameSettings.search("sValue")->str);

        if (environment.mWindowManager->getFullHelp()) {
            text += MWGui::ToolTips::getMiscString(ref->ref.owner, "Owner");
            text += MWGui::ToolTips::getMiscString(ref->base->script, "Script");
        }
        info.text = text;

        return info;
    }
}
