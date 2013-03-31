
#include "light.hpp"

#include <components/esm/loadligh.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/actionequip.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/physicssystem.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    void Light::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();
        assert (ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;

        MWRender::Objects& objects = renderingInterface.getObjects();
        objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);

        if (!model.empty())
            objects.insertMesh(ptr, "meshes\\" + model, true);
        else
            objects.insertLight(ptr);
    }

    void Light::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();
        assert (ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;

        if(!model.empty())
            physics.addObject(ptr,ref->mBase->mData.mFlags & ESM::Light::Carry);

        if (!ref->mBase->mSound.empty())
            MWBase::Environment::get().getSoundManager()->playSound3D(ptr, ref->mBase->mSound, 1.0, 1.0, MWBase::SoundManager::Play_Loop);
    }

    std::string Light::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();
        assert (ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Light::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();

        if (ref->mBase->mModel.empty())
            return "";

        return ref->mBase->mName;
    }

    boost::shared_ptr<MWWorld::Action> Light::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        if (!MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Inventory))
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction ());

        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();

        if (!(ref->mBase->mData.mFlags & ESM::Light::Carry))
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::FailedAction);

        boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionTake (ptr));

        action->setSound(getUpSoundId(ptr));

        return action;
    }

    std::string Light::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();

        return ref->mBase->mScript;
    }

    std::pair<std::vector<int>, bool> Light::getEquipmentSlots (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();

        std::vector<int> slots;

        if (ref->mBase->mData.mFlags & ESM::Light::Carry)
            slots.push_back (int (MWWorld::InventoryStore::Slot_CarriedLeft));

        return std::make_pair (slots, false);
    }

    int Light::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();

        return ref->mBase->mData.mValue;
    }

    void Light::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Light);

        registerClass (typeid (ESM::Light).name(), instance);
    }

    std::string Light::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        return std::string("Item Misc Up");
    }

    std::string Light::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        return std::string("Item Misc Down");
    }


    std::string Light::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
          MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();

        return ref->mBase->mIcon;
    }

    bool Light::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();

        return (ref->mBase->mName != "");
    }

    MWGui::ToolTipInfo Light::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        info.icon = ref->mBase->mIcon;

        std::string text;

        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.mWeight);
        text += MWGui::ToolTips::getValueString(ref->mBase->mData.mValue, "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getMiscString(ref->mRef.mOwner, "Owner");
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }

        info.text = text;

        return info;
    }

    boost::shared_ptr<MWWorld::Action> Light::use (const MWWorld::Ptr& ptr) const
    {
        boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionEquip(ptr));

        action->setSound(getUpSoundId(ptr));

        return action;
    }

    MWWorld::Ptr
    Light::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();

        return MWWorld::Ptr(&cell.mLights.insert(*ref), &cell);
    }
}
