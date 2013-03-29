
#include "probe.hpp"

#include <components/esm/loadprob.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/actionequip.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/nullaction.hpp"

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
        if(!model.empty())
            physics.addObject(ptr,true);
    }

    std::string Probe::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Probe> *ref =
            ptr.get<ESM::Probe>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Probe::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Probe> *ref =
            ptr.get<ESM::Probe>();

        return ref->mBase->mName;
    }
    boost::shared_ptr<MWWorld::Action> Probe::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        if (!MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Inventory))
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction ());

        boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionTake (ptr));

        action->setSound(getUpSoundId(ptr));

        return action;
    }

    std::string Probe::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Probe> *ref =
            ptr.get<ESM::Probe>();

        return ref->mBase->mScript;
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

        return ref->mBase->mData.mValue;
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

        return ref->mBase->mIcon;
    }

    bool Probe::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Probe> *ref =
            ptr.get<ESM::Probe>();

        return (ref->mBase->mName != "");
    }

    MWGui::ToolTipInfo Probe::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Probe> *ref =
            ptr.get<ESM::Probe>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        info.icon = ref->mBase->mIcon;

        std::string text;

        int remainingUses = (ptr.getCellRef().mCharge != -1) ? ptr.getCellRef().mCharge : ref->mBase->mData.mUses;

        text += "\n#{sUses}: " + MWGui::ToolTips::toString(remainingUses);
        text += "\n#{sQuality}: " + MWGui::ToolTips::toString(ref->mBase->mData.mQuality);
        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.mWeight);
        text += MWGui::ToolTips::getValueString(ref->mBase->mData.mValue, "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getMiscString(ref->mRef.mOwner, "Owner");
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
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

        return MWWorld::Ptr(&cell.mProbes.insert(*ref), &cell);
    }
}
