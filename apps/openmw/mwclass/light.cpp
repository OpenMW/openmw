#include "light.hpp"

#include <MyGUI_TextIterator.h>
#include <MyGUI_UString.h>

#include <components/esm3/loadligh.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/esm3/objectstate.hpp>
#include <components/esm4/loadligh.hpp>
#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwphysics/physicssystem.hpp"
#include "../mwworld/actionequip.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/ptr.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "classmodel.hpp"
#include "nameorid.hpp"

namespace MWClass
{
    Light::Light()
        : MWWorld::RegisteredClass<Light>(ESM::Light::sRecordId)
    {
    }

    void Light::insertObjectRendering(
        const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM::Light>* ref = ptr.get<ESM::Light>();

        // Insert even if model is empty, so that the light is added
        renderingInterface.getObjects().insertModel(ptr, model, !(ref->mBase->mData.mFlags & ESM::Light::OffDefault));
    }

    void Light::insertObject(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
        MWPhysics::PhysicsSystem& physics) const
    {
        MWWorld::LiveCellRef<ESM::Light>* ref = ptr.get<ESM::Light>();
        assert(ref->mBase != nullptr);

        insertObjectPhysics(ptr, model, rotation, physics);

        if (!ref->mBase->mSound.empty() && !(ref->mBase->mData.mFlags & ESM::Light::OffDefault))
            MWBase::Environment::get().getSoundManager()->playSound3D(
                ptr, ref->mBase->mSound, 1.0, 1.0, MWSound::Type::Sfx, MWSound::PlayMode::Loop);
    }

    void Light::insertObjectPhysics(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
        MWPhysics::PhysicsSystem& physics) const
    {
        // TODO: add option somewhere to enable collision for placeable objects
        if ((ptr.get<ESM::Light>()->mBase->mData.mFlags & ESM::Light::Carry) == 0)
            physics.addObject(ptr, VFS::Path::toNormalized(model), rotation, MWPhysics::CollisionType_World);
    }

    bool Light::useAnim() const
    {
        return true;
    }

    std::string_view Light::getModel(const MWWorld::ConstPtr& ptr) const
    {
        return getClassModel<ESM::Light>(ptr);
    }

    std::string_view Light::getName(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Light>* ref = ptr.get<ESM::Light>();

        if (ref->mBase->mModel.empty())
            return {};
        return getNameOrId<ESM::Light>(ptr);
    }

    bool Light::isItem(const MWWorld::ConstPtr& ptr) const
    {
        return ptr.get<ESM::Light>()->mBase->mData.mFlags & ESM::Light::Carry;
    }

    std::unique_ptr<MWWorld::Action> Light::activate(const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor) const
    {
        if (!MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Inventory))
            return std::make_unique<MWWorld::NullAction>();

        MWWorld::LiveCellRef<ESM::Light>* ref = ptr.get<ESM::Light>();
        if (!(ref->mBase->mData.mFlags & ESM::Light::Carry))
            return std::make_unique<MWWorld::FailedAction>();

        return defaultItemActivate(ptr, actor);
    }

    ESM::RefId Light::getScript(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Light>* ref = ptr.get<ESM::Light>();

        return ref->mBase->mScript;
    }

    std::pair<std::vector<int>, bool> Light::getEquipmentSlots(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Light>* ref = ptr.get<ESM::Light>();

        std::vector<int> slots;

        if (ref->mBase->mData.mFlags & ESM::Light::Carry)
            slots.push_back(int(MWWorld::InventoryStore::Slot_CarriedLeft));

        return std::make_pair(slots, false);
    }

    int Light::getValue(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Light>* ref = ptr.get<ESM::Light>();

        return ref->mBase->mData.mValue;
    }

    const ESM::RefId& Light::getUpSoundId(const MWWorld::ConstPtr& ptr) const
    {
        static const auto sound = ESM::RefId::stringRefId("Item Misc Up");
        return sound;
    }

    const ESM::RefId& Light::getDownSoundId(const MWWorld::ConstPtr& ptr) const
    {
        static const auto sound = ESM::RefId::stringRefId("Item Misc Down");
        return sound;
    }

    const std::string& Light::getInventoryIcon(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Light>* ref = ptr.get<ESM::Light>();

        return ref->mBase->mIcon;
    }

    bool Light::hasToolTip(const MWWorld::ConstPtr& ptr) const
    {
        return showsInInventory(ptr);
    }

    MWGui::ToolTipInfo Light::getToolTipInfo(const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Light>* ref = ptr.get<ESM::Light>();

        MWGui::ToolTipInfo info;
        std::string_view name = getName(ptr);
        info.caption = MyGUI::TextIterator::toTagsString(MyGUI::UString(name)) + MWGui::ToolTips::getCountString(count);
        info.icon = ref->mBase->mIcon;

        std::string text;

        // Don't show duration for infinite light sources.
        if (Settings::game().mShowEffectDuration && ptr.getClass().getRemainingUsageTime(ptr) != -1)
            text += MWGui::ToolTips::getDurationString(ptr.getClass().getRemainingUsageTime(ptr), "\n#{sDuration}");

        text += MWGui::ToolTips::getWeightString(ref->mBase->mData.mWeight, "#{sWeight}");
        text += MWGui::ToolTips::getValueString(ref->mBase->mData.mValue, "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
        {
            info.extra += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            info.extra += MWGui::ToolTips::getMiscString(ref->mBase->mScript.getRefIdString(), "Script");
        }

        info.text = std::move(text);

        return info;
    }

    bool Light::showsInInventory(const MWWorld::ConstPtr& ptr) const
    {
        const ESM::Light* light = ptr.get<ESM::Light>()->mBase;

        if (!(light->mData.mFlags & ESM::Light::Carry))
            return false;

        return Class::showsInInventory(ptr);
    }

    std::unique_ptr<MWWorld::Action> Light::use(const MWWorld::Ptr& ptr, bool force) const
    {
        std::unique_ptr<MWWorld::Action> action = std::make_unique<MWWorld::ActionEquip>(ptr, force);

        action->setSound(getUpSoundId(ptr));

        return action;
    }

    void Light::setRemainingUsageTime(const MWWorld::Ptr& ptr, float duration) const
    {
        ptr.getCellRef().setChargeFloat(duration);
    }

    float Light::getRemainingUsageTime(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Light>* ref = ptr.get<ESM::Light>();
        if (ptr.getCellRef().getCharge() == -1)
            return static_cast<float>(ref->mBase->mData.mTime);
        else
            return ptr.getCellRef().getChargeFloat();
    }

    MWWorld::Ptr Light::copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const
    {
        const MWWorld::LiveCellRef<ESM::Light>* ref = ptr.get<ESM::Light>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    bool Light::canSell(const MWWorld::ConstPtr& item, int npcServices) const
    {
        return (npcServices & ESM::NPC::Lights) != 0;
    }

    float Light::getWeight(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Light>* ref = ptr.get<ESM::Light>();
        return ref->mBase->mData.mWeight;
    }

    std::pair<int, std::string_view> Light::canBeEquipped(const MWWorld::ConstPtr& ptr, const MWWorld::Ptr& npc) const
    {
        const MWWorld::LiveCellRef<ESM::Light>* ref = ptr.get<ESM::Light>();
        if (!(ref->mBase->mData.mFlags & ESM::Light::Carry))
            return { 0, {} };

        return { 1, {} };
    }

    ESM::RefId Light::getSound(const MWWorld::ConstPtr& ptr) const
    {
        return ptr.get<ESM::Light>()->mBase->mSound;
    }

}
