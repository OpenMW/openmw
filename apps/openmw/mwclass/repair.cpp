
#include "repair.hpp"

#include <components/esm/loadrepa.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/actionrepair.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    void Repair::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        const std::string model = getModel(ptr);
        if (!model.empty()) {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, model);
        }
    }

    void Repair::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty())
            physics.addObject(ptr,true);
    }

    std::string Repair::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Repair> *ref =
            ptr.get<ESM::Repair>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Repair::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Repair> *ref =
            ptr.get<ESM::Repair>();

        return ref->mBase->mName;
    }

    boost::shared_ptr<MWWorld::Action> Repair::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        if (!MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Inventory))
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction ());

        boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionTake (ptr));

        action->setSound(getUpSoundId(ptr));

        return action;
    }

    std::string Repair::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Repair> *ref =
            ptr.get<ESM::Repair>();

        return ref->mBase->mScript;
    }

    int Repair::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Repair> *ref =
            ptr.get<ESM::Repair>();

        return ref->mBase->mData.mValue;
    }

    void Repair::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Repair);

        registerClass (typeid (ESM::Repair).name(), instance);
    }

    std::string Repair::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        return std::string("Item Repair Up");
    }

    std::string Repair::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        return std::string("Item Repair Down");
    }

    std::string Repair::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
          MWWorld::LiveCellRef<ESM::Repair> *ref =
            ptr.get<ESM::Repair>();

        return ref->mBase->mIcon;
    }

    bool Repair::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Repair> *ref =
            ptr.get<ESM::Repair>();

        return (ref->mBase->mName != "");
    }

    bool Repair::hasItemHealth (const MWWorld::Ptr& ptr) const
    {
        return true;
    }

    int Repair::getItemMaxHealth (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Repair> *ref =
            ptr.get<ESM::Repair>();

        return ref->mBase->mData.mUses;
    }

    MWGui::ToolTipInfo Repair::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Repair> *ref =
            ptr.get<ESM::Repair>();

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

    MWWorld::Ptr
    Repair::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Repair> *ref =
            ptr.get<ESM::Repair>();

        return MWWorld::Ptr(&cell.mRepairs.insert(*ref), &cell);
    }

    boost::shared_ptr<MWWorld::Action> Repair::use (const MWWorld::Ptr& ptr) const
    {
        return boost::shared_ptr<MWWorld::Action>(new MWWorld::ActionRepair(ptr));
    }
}
