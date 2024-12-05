#include "apparatus.hpp"

#include <MyGUI_TextIterator.h>
#include <MyGUI_UString.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include <components/esm3/loadappa.hpp>
#include <components/esm3/loadnpc.hpp>

#include "../mwworld/actionalchemy.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/ptr.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

#include "classmodel.hpp"
#include "nameorid.hpp"

namespace MWClass
{
    Apparatus::Apparatus()
        : MWWorld::RegisteredClass<Apparatus>(ESM::Apparatus::sRecordId)
    {
    }

    void Apparatus::insertObjectRendering(
        const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty())
        {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    std::string_view Apparatus::getModel(const MWWorld::ConstPtr& ptr) const
    {
        return getClassModel<ESM::Apparatus>(ptr);
    }

    std::string_view Apparatus::getName(const MWWorld::ConstPtr& ptr) const
    {
        return getNameOrId<ESM::Apparatus>(ptr);
    }

    std::unique_ptr<MWWorld::Action> Apparatus::activate(const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    ESM::RefId Apparatus::getScript(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Apparatus>* ref = ptr.get<ESM::Apparatus>();

        return ref->mBase->mScript;
    }

    int Apparatus::getValue(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Apparatus>* ref = ptr.get<ESM::Apparatus>();

        return ref->mBase->mData.mValue;
    }

    const ESM::RefId& Apparatus::getUpSoundId(const MWWorld::ConstPtr& ptr) const
    {
        static const auto sound = ESM::RefId::stringRefId("Item Apparatus Up");
        return sound;
    }

    const ESM::RefId& Apparatus::getDownSoundId(const MWWorld::ConstPtr& ptr) const
    {
        static const auto sound = ESM::RefId::stringRefId("Item Apparatus Down");
        return sound;
    }

    const std::string& Apparatus::getInventoryIcon(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Apparatus>* ref = ptr.get<ESM::Apparatus>();

        return ref->mBase->mIcon;
    }

    MWGui::ToolTipInfo Apparatus::getToolTipInfo(const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Apparatus>* ref = ptr.get<ESM::Apparatus>();

        MWGui::ToolTipInfo info;
        std::string_view name = getName(ptr);
        info.caption = MyGUI::TextIterator::toTagsString(MyGUI::UString(name)) + MWGui::ToolTips::getCountString(count);
        info.icon = ref->mBase->mIcon;

        std::string text;
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

    std::unique_ptr<MWWorld::Action> Apparatus::use(const MWWorld::Ptr& ptr, bool force) const
    {
        return std::make_unique<MWWorld::ActionAlchemy>(force);
    }

    MWWorld::Ptr Apparatus::copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const
    {
        const MWWorld::LiveCellRef<ESM::Apparatus>* ref = ptr.get<ESM::Apparatus>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    bool Apparatus::canSell(const MWWorld::ConstPtr& item, int npcServices) const
    {
        return (npcServices & ESM::NPC::Apparatus) != 0;
    }

    float Apparatus::getWeight(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Apparatus>* ref = ptr.get<ESM::Apparatus>();
        return ref->mBase->mData.mWeight;
    }
}
