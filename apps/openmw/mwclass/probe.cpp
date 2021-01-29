#include "probe.hpp"

#include <components/esm/loadprob.hpp>

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

    void Probe::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    void Probe::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWPhysics::PhysicsSystem& physics) const
    {
        // TODO: add option somewhere to enable collision for placeable objects
    }

    std::string Probe::getModel(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Probe> *ref = ptr.get<ESM::Probe>();

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Probe::getName (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Probe> *ref = ptr.get<ESM::Probe>();
        const std::string& name = ref->mBase->mName;

        return !name.empty() ? name : ref->mBase->mId;
    }
    std::shared_ptr<MWWorld::Action> Probe::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    std::string Probe::getScript (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Probe> *ref =
            ptr.get<ESM::Probe>();

        return ref->mBase->mScript;
    }

    std::pair<std::vector<int>, bool> Probe::getEquipmentSlots (const MWWorld::ConstPtr& ptr) const
    {
        std::vector<int> slots_;

        slots_.push_back (int (MWWorld::InventoryStore::Slot_CarriedRight));

        return std::make_pair (slots_, false);
    }

    int Probe::getValue (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Probe> *ref = ptr.get<ESM::Probe>();

        return ref->mBase->mData.mValue;
    }

    void Probe::registerSelf()
    {
        std::shared_ptr<Class> instance (new Probe);

        registerClass (typeid (ESM::Probe).name(), instance);
    }

    std::string Probe::getUpSoundId (const MWWorld::ConstPtr& ptr) const
    {
        return std::string("Item Probe Up");
    }

    std::string Probe::getDownSoundId (const MWWorld::ConstPtr& ptr) const
    {
        return std::string("Item Probe Down");
    }

    std::string Probe::getInventoryIcon (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Probe> *ref = ptr.get<ESM::Probe>();

        return ref->mBase->mIcon;
    }

    MWGui::ToolTipInfo Probe::getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Probe> *ref = ptr.get<ESM::Probe>();

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

    std::shared_ptr<MWWorld::Action> Probe::use (const MWWorld::Ptr& ptr, bool force) const
    {
        std::shared_ptr<MWWorld::Action> action(new MWWorld::ActionEquip(ptr, force));

        action->setSound(getUpSoundId(ptr));

        return action;
    }

    MWWorld::Ptr Probe::copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const
    {
        const MWWorld::LiveCellRef<ESM::Probe> *ref = ptr.get<ESM::Probe>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    std::pair<int, std::string> Probe::canBeEquipped(const MWWorld::ConstPtr &ptr, const MWWorld::Ptr &npc) const
    {
        // Do not allow equip tools from inventory during attack
        if (MWBase::Environment::get().getMechanicsManager()->isAttackingOrSpell(npc)
            && MWBase::Environment::get().getWindowManager()->isGuiMode())
            return std::make_pair(0, "#{sCantEquipWeapWarning}");

        return std::make_pair(1, "");
    }

    bool Probe::canSell (const MWWorld::ConstPtr& item, int npcServices) const
    {
        return (npcServices & ESM::NPC::Probes) != 0;
    }

    int Probe::getItemMaxHealth (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Probe> *ref = ptr.get<ESM::Probe>();

        return ref->mBase->mData.mUses;
    }

    float Probe::getWeight(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Probe> *ref = ptr.get<ESM::Probe>();
        return ref->mBase->mData.mWeight;
    }
}
