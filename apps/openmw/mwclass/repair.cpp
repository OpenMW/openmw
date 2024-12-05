#include "repair.hpp"

#include <MyGUI_TextIterator.h>
#include <MyGUI_UString.h>

#include <components/esm3/loadnpc.hpp>
#include <components/esm3/loadrepa.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/actionrepair.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/ptr.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "classmodel.hpp"
#include "nameorid.hpp"

namespace MWClass
{
    Repair::Repair()
        : MWWorld::RegisteredClass<Repair>(ESM::Repair::sRecordId)
    {
    }

    void Repair::insertObjectRendering(
        const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty())
        {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    std::string_view Repair::getModel(const MWWorld::ConstPtr& ptr) const
    {
        return getClassModel<ESM::Repair>(ptr);
    }

    std::string_view Repair::getName(const MWWorld::ConstPtr& ptr) const
    {
        return getNameOrId<ESM::Repair>(ptr);
    }

    std::unique_ptr<MWWorld::Action> Repair::activate(const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    ESM::RefId Repair::getScript(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Repair>* ref = ptr.get<ESM::Repair>();

        return ref->mBase->mScript;
    }

    int Repair::getValue(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Repair>* ref = ptr.get<ESM::Repair>();

        return ref->mBase->mData.mValue;
    }

    const ESM::RefId& Repair::getUpSoundId(const MWWorld::ConstPtr& ptr) const
    {
        static auto val = ESM::RefId::stringRefId("Item Repair Up");
        return val;
    }

    const ESM::RefId& Repair::getDownSoundId(const MWWorld::ConstPtr& ptr) const
    {
        static auto val = ESM::RefId::stringRefId("Item Repair Down");
        return val;
    }

    const std::string& Repair::getInventoryIcon(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Repair>* ref = ptr.get<ESM::Repair>();

        return ref->mBase->mIcon;
    }

    bool Repair::hasItemHealth(const MWWorld::ConstPtr& ptr) const
    {
        return true;
    }

    int Repair::getItemMaxHealth(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Repair>* ref = ptr.get<ESM::Repair>();

        return ref->mBase->mData.mUses;
    }

    MWGui::ToolTipInfo Repair::getToolTipInfo(const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Repair>* ref = ptr.get<ESM::Repair>();

        MWGui::ToolTipInfo info;
        std::string_view name = getName(ptr);
        info.caption = MyGUI::TextIterator::toTagsString(MyGUI::UString(name)) + MWGui::ToolTips::getCountString(count);
        info.icon = ref->mBase->mIcon;

        std::string text;

        int remainingUses = getItemHealth(ptr);

        text += "\n#{sUses}: " + MWGui::ToolTips::toString(remainingUses);
        text += "\n#{sQuality}: " + MWGui::ToolTips::toString(ref->mBase->mData.mQuality);
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

    MWWorld::Ptr Repair::copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const
    {
        const MWWorld::LiveCellRef<ESM::Repair>* ref = ptr.get<ESM::Repair>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    std::unique_ptr<MWWorld::Action> Repair::use(const MWWorld::Ptr& ptr, bool force) const
    {
        return std::make_unique<MWWorld::ActionRepair>(ptr, force);
    }

    bool Repair::canSell(const MWWorld::ConstPtr& item, int npcServices) const
    {
        return (npcServices & ESM::NPC::RepairItem) != 0;
    }

    float Repair::getWeight(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Repair>* ref = ptr.get<ESM::Repair>();
        return ref->mBase->mData.mWeight;
    }
}
