#include "lockpick.hpp"

#include <components/esm/loadlock.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actionequip.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwphysics/physicssystem.hpp"
#include "../mwworld/nullaction.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{

    void Lockpick::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    void Lockpick::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWPhysics::PhysicsSystem& physics) const
    {
        // TODO: add option somewhere to enable collision for placeable objects
    }

    std::string Lockpick::getModel(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Lockpick> *ref = ptr.get<ESM::Lockpick>();

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Lockpick::getName (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Lockpick> *ref = ptr.get<ESM::Lockpick>();
        const std::string& name = ref->mBase->mName;

        return !name.empty() ? name : ref->mBase->mId;
    }

    std::shared_ptr<MWWorld::Action> Lockpick::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    std::string Lockpick::getScript (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Lockpick> *ref = ptr.get<ESM::Lockpick>();

        return ref->mBase->mScript;
    }

    std::pair<std::vector<int>, bool> Lockpick::getEquipmentSlots (const MWWorld::ConstPtr& ptr) const
    {
        std::vector<int> slots_;

        slots_.push_back (int (MWWorld::InventoryStore::Slot_CarriedRight));

        return std::make_pair (slots_, false);
    }

    int Lockpick::getValue (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Lockpick> *ref = ptr.get<ESM::Lockpick>();

        return ref->mBase->mData.mValue;
    }

    void Lockpick::registerSelf()
    {
        std::shared_ptr<Class> instance (new Lockpick);

        registerClass (typeid (ESM::Lockpick).name(), instance);
    }

    std::string Lockpick::getUpSoundId (const MWWorld::ConstPtr& ptr) const
    {
        return std::string("Item Lockpick Up");
    }

    std::string Lockpick::getDownSoundId (const MWWorld::ConstPtr& ptr) const
    {
        return std::string("Item Lockpick Down");
    }

    std::string Lockpick::getInventoryIcon (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Lockpick> *ref = ptr.get<ESM::Lockpick>();

        return ref->mBase->mIcon;
    }

    MWGui::ToolTipInfo Lockpick::getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Lockpick> *ref = ptr.get<ESM::Lockpick>();

        MWGui::ToolTipInfo info;
        info.caption = MyGUI::TextIterator::toTagsString(getName(ptr)) + MWGui::ToolTips::getCountString(count);
        info.icon = ref->mBase->mIcon;

        std::string text;

        int remainingUses = getItemHealth(ptr);

        text += "\n#{sUses}: " + MWGui::ToolTips::toString(remainingUses);
        text += "\n#{sQuality}: " + MWGui::ToolTips::toString(ref->mBase->mData.mQuality);
        text += MWGui::ToolTips::getWeightString(ref->mBase->mData.mWeight, "#{sWeight}");
        text += MWGui::ToolTips::getValueString(ref->mBase->mData.mValue, "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }

        info.text = text;

        return info;
    }

    std::shared_ptr<MWWorld::Action> Lockpick::use (const MWWorld::Ptr& ptr, bool force) const
    {
        std::shared_ptr<MWWorld::Action> action(new MWWorld::ActionEquip(ptr, force));

        action->setSound(getUpSoundId(ptr));

        return action;
    }

    MWWorld::Ptr Lockpick::copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const
    {
        const MWWorld::LiveCellRef<ESM::Lockpick> *ref = ptr.get<ESM::Lockpick>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    std::pair<int, std::string> Lockpick::canBeEquipped(const MWWorld::ConstPtr &ptr, const MWWorld::Ptr &npc) const
    {
        // Do not allow equip tools from inventory during attack
        if (MWBase::Environment::get().getMechanicsManager()->isAttackingOrSpell(npc)
            && MWBase::Environment::get().getWindowManager()->isGuiMode())
            return std::make_pair(0, "#{sCantEquipWeapWarning}");

        return std::make_pair(1, "");
    }

    bool Lockpick::canSell (const MWWorld::ConstPtr& item, int npcServices) const
    {
        return (npcServices & ESM::NPC::Picks) != 0;
    }

    int Lockpick::getItemMaxHealth (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Lockpick> *ref = ptr.get<ESM::Lockpick>();

        return ref->mBase->mData.mUses;
    }

    float Lockpick::getWeight(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Lockpick> *ref = ptr.get<ESM::Lockpick>();
        return ref->mBase->mData.mWeight;
    }
}
