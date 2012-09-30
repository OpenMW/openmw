
#include "probe.hpp"

#include <components/esm/loadlocks.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/actionequip.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/physicssystem.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    void Probe::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        const std::string model = getModel(ptr);
        if (!model.empty()) {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, model);
        }
    }

    void Probe::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty()) {
            physics.insertObjectPhysics(ptr, model);
        }
    }

    std::string Probe::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Probe> *ref =
            ptr.get<ESM::Probe>();
        assert(ref->base != NULL);

        const std::string &model = ref->base->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Probe::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Probe> *ref =
            ptr.get<ESM::Probe>();

        return ref->base->mName;
    }
    boost::shared_ptr<MWWorld::Action> Probe::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionTake (ptr));

        action->setSound(getUpSoundId(ptr));

        return action;
    }

    std::string Probe::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Probe> *ref =
            ptr.get<ESM::Probe>();

        return ref->base->mScript;
    }

    std::pair<std::vector<int>, bool> Probe::getEquipmentSlots (const MWWorld::Ptr& ptr) const
    {
        std::vector<int> slots;

        slots.push_back (int (MWWorld::InventoryStore::Slot_CarriedRight));

        return std::make_pair (slots, false);
    }

    int Probe::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Probe> *ref =
            ptr.get<ESM::Probe>();

        return ref->base->mData.mValue;
    }

    void Probe::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Probe);

        registerClass (typeid (ESM::Probe).name(), instance);
    }

    std::string Probe::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        return std::string("Item Probe Up");
    }

    std::string Probe::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        return std::string("Item Probe Down");
    }

    std::string Probe::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
          MWWorld::LiveCellRef<ESM::Probe> *ref =
            ptr.get<ESM::Probe>();

        return ref->base->mIcon;
    }

    bool Probe::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Probe> *ref =
            ptr.get<ESM::Probe>();

        return (ref->base->mName != "");
    }

    MWGui::ToolTipInfo Probe::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Probe> *ref =
            ptr.get<ESM::Probe>();

        MWGui::ToolTipInfo info;
        info.caption = ref->base->mName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        info.icon = ref->base->mIcon;

        std::string text;

        /// \todo store remaining uses somewhere

        text += "\n#{sUses}: " + MWGui::ToolTips::toString(ref->base->mData.mUses);
        text += "\n#{sQuality}: " + MWGui::ToolTips::toString(ref->base->mData.mQuality);
        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->base->mData.mWeight);
        text += MWGui::ToolTips::getValueString(ref->base->mData.mValue, "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getMiscString(ref->ref.mOwner, "Owner");
            text += MWGui::ToolTips::getMiscString(ref->base->mScript, "Script");
        }

        info.text = text;

        return info;
    }

    boost::shared_ptr<MWWorld::Action> Probe::use (const MWWorld::Ptr& ptr) const
    {
        boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionEquip(ptr));

        action->setSound(getUpSoundId(ptr));

        return action;
    }

    MWWorld::Ptr
    Probe::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Probe> *ref =
            ptr.get<ESM::Probe>();

        return MWWorld::Ptr(&cell.probes.insert(*ref), &cell);
    }
}
