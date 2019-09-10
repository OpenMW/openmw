#include "misc.hpp"

#include <components/esm/loadmisc.hpp>
#include <components/settings/settings.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwphysics/physicssystem.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/actionsoulgem.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    bool Miscellaneous::isGold (const MWWorld::ConstPtr& ptr) const
    {
        return Misc::StringUtils::ciEqual(ptr.getCellRef().getRefId(), "gold_001")
                        || Misc::StringUtils::ciEqual(ptr.getCellRef().getRefId(), "gold_005")
                        || Misc::StringUtils::ciEqual(ptr.getCellRef().getRefId(), "gold_010")
                        || Misc::StringUtils::ciEqual(ptr.getCellRef().getRefId(), "gold_025")
                        || Misc::StringUtils::ciEqual(ptr.getCellRef().getRefId(), "gold_100");
    }

    void Miscellaneous::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    void Miscellaneous::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWPhysics::PhysicsSystem& physics) const
    {
        // TODO: add option somewhere to enable collision for placeable objects
    }

    std::string Miscellaneous::getModel(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Miscellaneous> *ref = ptr.get<ESM::Miscellaneous>();

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Miscellaneous::getName (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Miscellaneous> *ref = ptr.get<ESM::Miscellaneous>();
        const std::string& name = ref->mBase->mName;

        return !name.empty() ? name : ref->mBase->mId;
    }

    std::shared_ptr<MWWorld::Action> Miscellaneous::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    std::string Miscellaneous::getScript (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Miscellaneous> *ref = ptr.get<ESM::Miscellaneous>();

        return ref->mBase->mScript;
    }

    int Miscellaneous::getValue (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Miscellaneous> *ref = ptr.get<ESM::Miscellaneous>();

        int value = ref->mBase->mData.mValue;
        if (ptr.getCellRef().getGoldValue() > 1 && ptr.getRefData().getCount() == 1)
            value = ptr.getCellRef().getGoldValue();

        if (ptr.getCellRef().getSoul() != "")
        {
            const ESM::Creature *creature = MWBase::Environment::get().getWorld()->getStore().get<ESM::Creature>().search(ref->mRef.getSoul());
            if (creature)
            {
                int soul = creature->mData.mSoul;
                if (Settings::Manager::getBool("rebalance soul gem values", "Game"))
                {
                    // use the 'soul gem value rebalance' formula from the Morrowind Code Patch 
                    float soulValue = 0.0001 * pow(soul, 3) + 2 * soul;
                    
                    // for Azura's star add the unfilled value
                    if (Misc::StringUtils::ciEqual(ptr.getCellRef().getRefId(), "Misc_SoulGem_Azura"))
                        value += soulValue;
                    else
                        value = soulValue;
                }
                else
                    value *= soul;
            }
        }

        return value;
    }

    void Miscellaneous::registerSelf()
    {
        std::shared_ptr<Class> instance (new Miscellaneous);

        registerClass (typeid (ESM::Miscellaneous).name(), instance);
    }

    std::string Miscellaneous::getUpSoundId (const MWWorld::ConstPtr& ptr) const
    {
        if (isGold(ptr))
            return std::string("Item Gold Up");
        return std::string("Item Misc Up");
    }

    std::string Miscellaneous::getDownSoundId (const MWWorld::ConstPtr& ptr) const
    {
        if (isGold(ptr))
            return std::string("Item Gold Down");
        return std::string("Item Misc Down");
    }

    std::string Miscellaneous::getInventoryIcon (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Miscellaneous> *ref = ptr.get<ESM::Miscellaneous>();

        return ref->mBase->mIcon;
    }

    MWGui::ToolTipInfo Miscellaneous::getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Miscellaneous> *ref = ptr.get<ESM::Miscellaneous>();

        MWGui::ToolTipInfo info;

        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        bool gold = isGold(ptr);
        if (gold)
            count *= getValue(ptr);

        std::string countString;
        if (!gold)
            countString = MWGui::ToolTips::getCountString(count);
        else // gold displays its count also if it's 1.
            countString = " (" + std::to_string(count) + ")";

        info.caption = MyGUI::TextIterator::toTagsString(getName(ptr)) + countString;
        info.icon = ref->mBase->mIcon;

        if (ref->mRef.getSoul() != "")
        {
            const ESM::Creature *creature = store.get<ESM::Creature>().search(ref->mRef.getSoul());
            if (creature && !creature->mName.empty())
                info.caption += " (" + creature->mName + ")";
            else if (creature)
                info.caption += " (" + creature->mId + ")";
        }

        std::string text;

        text += MWGui::ToolTips::getWeightString(ref->mBase->mData.mWeight, "#{sWeight}");
        if (!gold && !ref->mBase->mData.mIsKey)
            text += MWGui::ToolTips::getValueString(getValue(ptr), "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }

        info.text = text;

        return info;
    }

    MWWorld::Ptr Miscellaneous::copyToCell(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell, int count) const
    {
        MWWorld::Ptr newPtr;

        const MWWorld::ESMStore &store =
            MWBase::Environment::get().getWorld()->getStore();

        if (isGold(ptr)) {
            int goldAmount = getValue(ptr) * count;

            std::string base = "Gold_001";
            if (goldAmount >= 100)
                base = "Gold_100";
            else if (goldAmount >= 25)
                base = "Gold_025";
            else if (goldAmount >= 10)
                base = "Gold_010";
            else if (goldAmount >= 5)
                base = "Gold_005";

            // Really, I have no idea why moving ref out of conditional
            // scope causes list::push_back throwing std::bad_alloc
            MWWorld::ManualRef newRef(store, base);
            const MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
                newRef.getPtr().get<ESM::Miscellaneous>();

            newPtr = MWWorld::Ptr(cell.insert(ref), &cell);
            newPtr.getCellRef().setGoldValue(goldAmount);
            newPtr.getRefData().setCount(1);
        } else {
            const MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
                ptr.get<ESM::Miscellaneous>();
            newPtr = MWWorld::Ptr(cell.insert(ref), &cell);
            newPtr.getRefData().setCount(count);
        }
        newPtr.getCellRef().unsetRefNum();

        return newPtr;
    }

    std::shared_ptr<MWWorld::Action> Miscellaneous::use (const MWWorld::Ptr& ptr, bool force) const
    {
        if (ptr.getCellRef().getSoul().empty() || !MWBase::Environment::get().getWorld()->getStore().get<ESM::Creature>().search(ptr.getCellRef().getSoul()))
            return std::shared_ptr<MWWorld::Action>(new MWWorld::NullAction());
        else
            return std::shared_ptr<MWWorld::Action>(new MWWorld::ActionSoulgem(ptr));
    }

    bool Miscellaneous::canSell (const MWWorld::ConstPtr& item, int npcServices) const
    {
        const MWWorld::LiveCellRef<ESM::Miscellaneous> *ref = item.get<ESM::Miscellaneous>();

        return !ref->mBase->mData.mIsKey && (npcServices & ESM::NPC::Misc) && !isGold(item);
    }

    float Miscellaneous::getWeight(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Miscellaneous> *ref = ptr.get<ESM::Miscellaneous>();
        return ref->mBase->mData.mWeight;
    }

    bool Miscellaneous::isKey(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Miscellaneous> *ref = ptr.get<ESM::Miscellaneous>();
        return ref->mBase->mData.mIsKey != 0;
    }

}
