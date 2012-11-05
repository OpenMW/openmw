
#include "lockpick.hpp"

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
    void Lockpick::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        const std::string model = getModel(ptr);
        if (!model.empty()) {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, model);
        }
    }

    void Lockpick::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty()) {
            physics.insertObjectPhysics(ptr, model);
        }
    }

    std::string Lockpick::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Tool> *ref =
            ptr.get<ESM::Tool>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Lockpick::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Tool> *ref =
            ptr.get<ESM::Tool>();

        return ref->mBase->mName;
    }

    boost::shared_ptr<MWWorld::Action> Lockpick::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionTake (ptr));

        action->setSound(getUpSoundId(ptr));

        return action;
    }

    std::string Lockpick::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Tool> *ref =
            ptr.get<ESM::Tool>();

        return ref->mBase->mScript;
    }

    std::pair<std::vector<int>, bool> Lockpick::getEquipmentSlots (const MWWorld::Ptr& ptr) const
    {
        std::vector<int> slots;

        slots.push_back (int (MWWorld::InventoryStore::Slot_CarriedRight));

        return std::make_pair (slots, false);
    }

    int Lockpick::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Tool> *ref =
            ptr.get<ESM::Tool>();

        return ref->mBase->mData.mValue;
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
          MWWorld::LiveCellRef<ESM::Tool> *ref =
            ptr.get<ESM::Tool>();

        return ref->mBase->mIcon;
    }

    bool Lockpick::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Tool> *ref =
            ptr.get<ESM::Tool>();

        return (ref->mBase->mName != "");
    }

    MWGui::ToolTipInfo Lockpick::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Tool> *ref =
            ptr.get<ESM::Tool>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        info.icon = ref->mBase->mIcon;

        std::string text;

        /// \todo store remaining uses somewhere

        text += "\n#{sUses}: " + MWGui::ToolTips::toString(ref->mBase->mData.mUses);
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

    boost::shared_ptr<MWWorld::Action> Lockpick::use (const MWWorld::Ptr& ptr) const
    {
        boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionEquip(ptr));

        action->setSound(getUpSoundId(ptr));

        return action;
    }

    MWWorld::Ptr
    Lockpick::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Tool> *ref =
            ptr.get<ESM::Tool>();

        return MWWorld::Ptr(&cell.mLockpicks.insert(*ref), &cell);
    }
}
