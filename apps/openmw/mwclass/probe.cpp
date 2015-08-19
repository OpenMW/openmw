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
#include "../mwphysics/physicssystem.hpp"
#include "../mwworld/nullaction.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string Probe::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM::Probe>()->mBase->mId;
    }

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
        return defaultItemActivate(ptr, actor);
    }

    std::string Probe::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Probe> *ref =
            ptr.get<ESM::Probe>();

        return ref->mBase->mScript;
    }

    std::pair<std::vector<int>, bool> Probe::getEquipmentSlots (const MWWorld::Ptr& ptr) const
    {
        std::vector<int> slots_;

        slots_.push_back (int (MWWorld::InventoryStore::Slot_CarriedRight));

        return std::make_pair (slots_, false);
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

        int remainingUses = getItemHealth(ptr);

        text += "\n#{sUses}: " + MWGui::ToolTips::toString(remainingUses);
        text += "\n#{sQuality}: " + MWGui::ToolTips::toString(ref->mBase->mData.mQuality);
        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.mWeight);
        text += MWGui::ToolTips::getValueString(ref->mBase->mData.mValue, "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
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

        return MWWorld::Ptr(&cell.get<ESM::Probe>().insert(*ref), &cell);
    }

    bool Probe::canSell (const MWWorld::Ptr& item, int npcServices) const
    {
        return (npcServices & ESM::NPC::Probes) != 0;
    }

    int Probe::getItemMaxHealth (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Probe> *ref =
            ptr.get<ESM::Probe>();

        return ref->mBase->mData.mUses;
    }

    float Probe::getWeight(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Probe> *ref =
            ptr.get<ESM::Probe>();
        return ref->mBase->mData.mWeight;
    }
}
