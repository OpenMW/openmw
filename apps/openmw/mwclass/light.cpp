#include "light.hpp"

#include <components/esm/loadligh.hpp>
#include <components/esm/objectstate.hpp>
#include <components/settings/settings.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actionequip.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwphysics/physicssystem.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{

    void Light::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();

        // Insert even if model is empty, so that the light is added
        renderingInterface.getObjects().insertModel(ptr, model, true, !(ref->mBase->mData.mFlags & ESM::Light::OffDefault));
    }

    void Light::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWPhysics::PhysicsSystem& physics) const
    {
        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();
        assert (ref->mBase != nullptr);

        // TODO: add option somewhere to enable collision for placeable objects
        if (!model.empty() && (ref->mBase->mData.mFlags & ESM::Light::Carry) == 0)
            physics.addObject(ptr, model);

        if (!ref->mBase->mSound.empty() && !(ref->mBase->mData.mFlags & ESM::Light::OffDefault))
            MWBase::Environment::get().getSoundManager()->playSound3D(ptr, ref->mBase->mSound, 1.0, 1.0,
                                                                      MWSound::Type::Sfx,
                                                                      MWSound::PlayMode::Loop);
    }

    bool Light::useAnim() const
    {
        return true;
    }

    std::string Light::getModel(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Light> *ref = ptr.get<ESM::Light>();

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Light::getName (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Light> *ref = ptr.get<ESM::Light>();

        if (ref->mBase->mModel.empty())
            return std::string();

        const std::string& name = ref->mBase->mName;
        return !name.empty() ? name : ref->mBase->mId;
    }

    std::shared_ptr<MWWorld::Action> Light::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        if(!MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Inventory))
            return std::shared_ptr<MWWorld::Action>(new MWWorld::NullAction());

        MWWorld::LiveCellRef<ESM::Light> *ref = ptr.get<ESM::Light>();
        if(!(ref->mBase->mData.mFlags&ESM::Light::Carry))
            return std::shared_ptr<MWWorld::Action>(new MWWorld::FailedAction());

        return defaultItemActivate(ptr, actor);
    }

    std::string Light::getScript (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Light> *ref = ptr.get<ESM::Light>();

        return ref->mBase->mScript;
    }

    std::pair<std::vector<int>, bool> Light::getEquipmentSlots (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Light> *ref = ptr.get<ESM::Light>();

        std::vector<int> slots_;

        if (ref->mBase->mData.mFlags & ESM::Light::Carry)
            slots_.push_back (int (MWWorld::InventoryStore::Slot_CarriedLeft));

        return std::make_pair (slots_, false);
    }

    int Light::getValue (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Light> *ref = ptr.get<ESM::Light>();

        return ref->mBase->mData.mValue;
    }

    void Light::registerSelf()
    {
        std::shared_ptr<Class> instance (new Light);

        registerClass (typeid (ESM::Light).name(), instance);
    }

    std::string Light::getUpSoundId (const MWWorld::ConstPtr& ptr) const
    {
        return std::string("Item Misc Up");
    }

    std::string Light::getDownSoundId (const MWWorld::ConstPtr& ptr) const
    {
        return std::string("Item Misc Down");
    }


    std::string Light::getInventoryIcon (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Light> *ref = ptr.get<ESM::Light>();

        return ref->mBase->mIcon;
    }

    bool Light::hasToolTip (const MWWorld::ConstPtr& ptr) const
    {
        return showsInInventory(ptr);
    }

    MWGui::ToolTipInfo Light::getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Light> *ref = ptr.get<ESM::Light>();

        MWGui::ToolTipInfo info;
        info.caption = MyGUI::TextIterator::toTagsString(getName(ptr)) + MWGui::ToolTips::getCountString(count);
        info.icon = ref->mBase->mIcon;

        std::string text;

        // Don't show duration for infinite light sources.
        if (Settings::Manager::getBool("show effect duration","Game") && ptr.getClass().getRemainingUsageTime(ptr) != -1)
            text += MWGui::ToolTips::getDurationString(ptr.getClass().getRemainingUsageTime(ptr), "\n#{sDuration}");

        text += MWGui::ToolTips::getWeightString(ref->mBase->mData.mWeight, "#{sWeight}");
        text += MWGui::ToolTips::getValueString(ref->mBase->mData.mValue, "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }

        info.text = text;

        return info;
    }

    bool Light::showsInInventory (const MWWorld::ConstPtr& ptr) const
    {
        const ESM::Light* light = ptr.get<ESM::Light>()->mBase;

        if (!(light->mData.mFlags & ESM::Light::Carry))
            return false;

        return Class::showsInInventory(ptr);
    }

    std::shared_ptr<MWWorld::Action> Light::use (const MWWorld::Ptr& ptr, bool force) const
    {
        std::shared_ptr<MWWorld::Action> action(new MWWorld::ActionEquip(ptr, force));

        action->setSound(getUpSoundId(ptr));

        return action;
    }

    void Light::setRemainingUsageTime (const MWWorld::Ptr& ptr, float duration) const
    {
        ptr.getCellRef().setChargeFloat(duration);
    }

    float Light::getRemainingUsageTime (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Light> *ref = ptr.get<ESM::Light>();
        if (ptr.getCellRef().getCharge() == -1)
            return static_cast<float>(ref->mBase->mData.mTime);
        else
            return ptr.getCellRef().getChargeFloat();
    }

    MWWorld::Ptr Light::copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const
    {
        const MWWorld::LiveCellRef<ESM::Light> *ref = ptr.get<ESM::Light>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    bool Light::canSell (const MWWorld::ConstPtr& item, int npcServices) const
    {
        return (npcServices & ESM::NPC::Lights) != 0;
    }

    float Light::getWeight(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Light> *ref = ptr.get<ESM::Light>();
        return ref->mBase->mData.mWeight;
    }

    std::pair<int, std::string> Light::canBeEquipped(const MWWorld::ConstPtr &ptr, const MWWorld::Ptr &npc) const
    {
        const MWWorld::LiveCellRef<ESM::Light> *ref = ptr.get<ESM::Light>();
        if (!(ref->mBase->mData.mFlags & ESM::Light::Carry))
            return std::make_pair(0,"");

        return std::make_pair(1,"");
    }

    std::string Light::getSound(const MWWorld::ConstPtr& ptr) const
    {
      return ptr.get<ESM::Light>()->mBase->mSound;
    }
}
