
#include "lockpick.hpp"

#include <components/esm/loadlocks.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwbase/environment.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/world.hpp"
#include "../mwgui/window_manager.hpp"
#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"

#include "../mwsound/soundmanager.hpp"

namespace MWClass
{
    void Lockpick::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        ESMS::LiveCellRef<ESM::Tool, MWWorld::RefData> *ref =
            ptr.get<ESM::Tool>();

        assert (ref->base != NULL);
        const std::string &model = ref->base->model;

        if (!model.empty())
        {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, "meshes\\" + model);
        }
    }

    void Lockpick::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        ESMS::LiveCellRef<ESM::Tool, MWWorld::RefData> *ref =
            ptr.get<ESM::Tool>();


        const std::string &model = ref->base->model;
        assert (ref->base != NULL);
        if(!model.empty()){
            physics.insertObjectPhysics(ptr, "meshes\\" + model);
        }

    }


    std::string Lockpick::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Tool, MWWorld::RefData> *ref =
            ptr.get<ESM::Tool>();

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Lockpick::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        MWBase::Environment::get().getSoundManager()->playSound3D (ptr, getUpSoundId(ptr), 1.0, 1.0, MWSound::Play_NoTrack);

        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionTake (ptr));
    }

    std::string Lockpick::getScript (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Tool, MWWorld::RefData> *ref =
            ptr.get<ESM::Tool>();

        return ref->base->script;
    }

    std::pair<std::vector<int>, bool> Lockpick::getEquipmentSlots (const MWWorld::Ptr& ptr) const
    {
        std::vector<int> slots;

        slots.push_back (int (MWWorld::InventoryStore::Slot_CarriedRight));

        return std::make_pair (slots, false);
    }

    int Lockpick::getValue (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Tool, MWWorld::RefData> *ref =
            ptr.get<ESM::Tool>();

        return ref->base->data.value;
    }

    void Lockpick::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Lockpick);

        registerClass (typeid (ESM::Tool).name(), instance);
    }

    std::string Lockpick::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        return std::string("Item Lockpick Up");
    }

    std::string Lockpick::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        return std::string("Item Lockpick Down");
    }

    std::string Lockpick::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
          ESMS::LiveCellRef<ESM::Tool, MWWorld::RefData> *ref =
            ptr.get<ESM::Tool>();

        return ref->base->icon;
    }

    bool Lockpick::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Tool, MWWorld::RefData> *ref =
            ptr.get<ESM::Tool>();

        return (ref->base->name != "");
    }

    MWGui::ToolTipInfo Lockpick::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Tool, MWWorld::RefData> *ref =
            ptr.get<ESM::Tool>();

        MWGui::ToolTipInfo info;
        info.caption = ref->base->name + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        info.icon = ref->base->icon;

        const ESMS::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        std::string text;

        /// \todo store remaining uses somewhere

        text += "\n" + store.gameSettings.search("sUses")->str + ": " + MWGui::ToolTips::toString(ref->base->data.uses);
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
}
