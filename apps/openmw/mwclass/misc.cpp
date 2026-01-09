#include "misc.hpp"

#include <MyGUI_TextIterator.h>
#include <MyGUI_UString.h>

#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadmisc.hpp>
#include <components/esm3/loadnpc.hpp>

#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/actionsoulgem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/worldmodel.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "classmodel.hpp"
#include "nameorid.hpp"

namespace MWClass
{
    Miscellaneous::Miscellaneous()
        : MWWorld::RegisteredClass<Miscellaneous>(ESM::Miscellaneous::sRecordId)
    {
    }

    bool Miscellaneous::isGold(const MWWorld::ConstPtr& ptr) const
    {
        return ptr.getCellRef().getRefId() == "gold_001" || ptr.getCellRef().getRefId() == "gold_005"
            || ptr.getCellRef().getRefId() == "gold_010" || ptr.getCellRef().getRefId() == "gold_025"
            || ptr.getCellRef().getRefId() == "gold_100";
    }

    void Miscellaneous::insertObjectRendering(
        const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty())
        {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    std::string_view Miscellaneous::getModel(const MWWorld::ConstPtr& ptr) const
    {
        return getClassModel<ESM::Miscellaneous>(ptr);
    }

    std::string_view Miscellaneous::getName(const MWWorld::ConstPtr& ptr) const
    {
        return getNameOrId<ESM::Miscellaneous>(ptr);
    }

    std::unique_ptr<MWWorld::Action> Miscellaneous::activate(const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    ESM::RefId Miscellaneous::getScript(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Miscellaneous>* ref = ptr.get<ESM::Miscellaneous>();

        return ref->mBase->mScript;
    }

    int Miscellaneous::getValue(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Miscellaneous>* ref = ptr.get<ESM::Miscellaneous>();

        int value = ref->mBase->mData.mValue;
        if (isGold(ptr) && ptr.getCellRef().getCount() != 1)
            value = 1;

        if (!ptr.getCellRef().getSoul().empty())
        {
            const ESM::Creature* creature
                = MWBase::Environment::get().getESMStore()->get<ESM::Creature>().search(ref->mRef.getSoul());
            if (creature)
            {
                int soul = creature->mData.mSoul;
                if (Settings::game().mRebalanceSoulGemValues)
                {
                    // use the 'soul gem value rebalance' formula from the Morrowind Code Patch
                    double soulValue = 0.0001 * std::pow(soul, 3) + 2 * soul;

                    // for Azura's star add the unfilled value
                    if (ptr.getCellRef().getRefId() == "Misc_SoulGem_Azura")
                        value += static_cast<int>(soulValue);
                    else
                        value = static_cast<int>(soulValue);
                }
                else
                    value *= soul;
            }
        }

        return value;
    }

    const ESM::RefId& Miscellaneous::getUpSoundId(const MWWorld::ConstPtr& ptr) const
    {
        static const ESM::RefId soundGold = ESM::RefId::stringRefId("Item Gold Up");
        static const ESM::RefId soundMisc = ESM::RefId::stringRefId("Item Misc Up");
        if (isGold(ptr))
            return soundGold;

        return soundMisc;
    }

    const ESM::RefId& Miscellaneous::getDownSoundId(const MWWorld::ConstPtr& ptr) const
    {
        static const ESM::RefId soundGold = ESM::RefId::stringRefId("Item Gold Down");
        static const ESM::RefId soundMisc = ESM::RefId::stringRefId("Item Misc Down");
        if (isGold(ptr))
            return soundGold;
        return soundMisc;
    }

    const std::string& Miscellaneous::getInventoryIcon(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Miscellaneous>* ref = ptr.get<ESM::Miscellaneous>();

        return ref->mBase->mIcon;
    }

    MWGui::ToolTipInfo Miscellaneous::getToolTipInfo(const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Miscellaneous>* ref = ptr.get<ESM::Miscellaneous>();

        MWGui::ToolTipInfo info;

        bool gold = isGold(ptr);
        if (gold)
            count *= getValue(ptr);

        std::string countString;
        if (!gold)
            countString = MWGui::ToolTips::getCountString(count);
        else // gold displays its count also if it's 1.
            countString = " (" + std::to_string(count) + ")";

        std::string_view name = getName(ptr);
        info.caption = MyGUI::TextIterator::toTagsString(MyGUI::UString(name)) + MWGui::ToolTips::getCountString(count)
            + MWGui::ToolTips::getSoulString(ptr.getCellRef());
        info.icon = ref->mBase->mIcon;

        std::string text;

        text += MWGui::ToolTips::getWeightString(ref->mBase->mData.mWeight, "#{sWeight}");
        if (!gold && !(ref->mBase->mData.mFlags & ESM::Miscellaneous::Key))
            text += MWGui::ToolTips::getValueString(getValue(ptr), "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
        {
            info.extra += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            info.extra += MWGui::ToolTips::getMiscString(ref->mBase->mScript.getRefIdString(), "Script");
        }

        info.text = std::move(text);

        return info;
    }

    static MWWorld::Ptr createGold(MWWorld::CellStore& cell, int goldAmount)
    {
        std::string_view base = "gold_001";
        if (goldAmount >= 100)
            base = "gold_100";
        else if (goldAmount >= 25)
            base = "gold_025";
        else if (goldAmount >= 10)
            base = "gold_010";
        else if (goldAmount >= 5)
            base = "gold_005";

        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
        MWWorld::ManualRef newRef(store, ESM::RefId::stringRefId(base));
        const MWWorld::LiveCellRef<ESM::Miscellaneous>* ref = newRef.getPtr().get<ESM::Miscellaneous>();

        MWWorld::Ptr ptr(cell.insert(ref), &cell);
        ptr.getCellRef().setCount(goldAmount);
        return ptr;
    }

    MWWorld::Ptr Miscellaneous::copyToCell(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell, int count) const
    {
        MWWorld::Ptr newPtr;
        if (isGold(ptr))
        {
            newPtr = createGold(cell, getValue(ptr) * count);
            newPtr.getRefData() = ptr.getRefData();
        }
        else
        {
            const MWWorld::LiveCellRef<ESM::Miscellaneous>* ref = ptr.get<ESM::Miscellaneous>();
            newPtr = MWWorld::Ptr(cell.insert(ref), &cell);
            newPtr.getCellRef().setCount(count);
        }
        newPtr.getCellRef().unsetRefNum();
        newPtr.getRefData().setLuaScripts(nullptr);
        MWBase::Environment::get().getWorldModel()->registerPtr(newPtr);
        return newPtr;
    }

    MWWorld::Ptr Miscellaneous::moveToCell(const MWWorld::Ptr& ptr, MWWorld::CellStore& cell) const
    {
        MWWorld::Ptr newPtr;
        if (isGold(ptr))
        {
            newPtr = createGold(cell, getValue(ptr) * ptr.getCellRef().getCount());
            newPtr.getRefData() = ptr.getRefData();
            newPtr.getCellRef().setRefNum(ptr.getCellRef().getRefNum());
        }
        else
        {
            const MWWorld::LiveCellRef<ESM::Miscellaneous>* ref = ptr.get<ESM::Miscellaneous>();
            newPtr = MWWorld::Ptr(cell.insert(ref), &cell);
        }
        ptr.getRefData().setLuaScripts(nullptr);
        MWBase::Environment::get().getWorldModel()->registerPtr(newPtr);
        return newPtr;
    }

    std::unique_ptr<MWWorld::Action> Miscellaneous::use(const MWWorld::Ptr& ptr, bool force) const
    {
        if (isSoulGem(ptr))
            return std::make_unique<MWWorld::ActionSoulgem>(ptr);

        return std::make_unique<MWWorld::NullAction>();
    }

    bool Miscellaneous::canSell(const MWWorld::ConstPtr& item, int npcServices) const
    {
        const MWWorld::LiveCellRef<ESM::Miscellaneous>* ref = item.get<ESM::Miscellaneous>();

        return !(ref->mBase->mData.mFlags & ESM::Miscellaneous::Key) && (npcServices & ESM::NPC::Misc) && !isGold(item);
    }

    float Miscellaneous::getWeight(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Miscellaneous>* ref = ptr.get<ESM::Miscellaneous>();
        return ref->mBase->mData.mWeight;
    }

    bool Miscellaneous::isKey(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Miscellaneous>* ref = ptr.get<ESM::Miscellaneous>();
        return ref->mBase->mData.mFlags & ESM::Miscellaneous::Key;
    }

    bool Miscellaneous::isSoulGem(const MWWorld::ConstPtr& ptr) const
    {
        return ptr.getCellRef().getRefId().startsWith("misc_soulgem");
    }

}
