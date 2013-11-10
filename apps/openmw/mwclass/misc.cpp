
#include "misc.hpp"

#include <boost/lexical_cast.hpp>

#include <components/esm/loadmisc.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/actionsoulgem.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include <boost/lexical_cast.hpp>

namespace
{
bool isGold (const MWWorld::Ptr& ptr)
{
    return Misc::StringUtils::ciEqual(ptr.getCellRef().mRefID, "gold_001")
                    || Misc::StringUtils::ciEqual(ptr.getCellRef().mRefID, "gold_005")
                    || Misc::StringUtils::ciEqual(ptr.getCellRef().mRefID, "gold_010")
                    || Misc::StringUtils::ciEqual(ptr.getCellRef().mRefID, "gold_025")
                    || Misc::StringUtils::ciEqual(ptr.getCellRef().mRefID, "gold_100");
}
}

namespace MWClass
{
    void Miscellaneous::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        const std::string model = getModel(ptr);
        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    void Miscellaneous::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty())
            physics.addObject(ptr,true);
    }

    std::string Miscellaneous::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Miscellaneous::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();

        return ref->mBase->mName;
    }

    boost::shared_ptr<MWWorld::Action> Miscellaneous::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    std::string Miscellaneous::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();

        return ref->mBase->mScript;
    }

    int Miscellaneous::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();

        int value = (ptr.getCellRef().mGoldValue == 1) ? ref->mBase->mData.mValue : ptr.getCellRef().mGoldValue;

        if (ptr.getCellRef().mSoul != "")
        {
            const ESM::Creature *creature = MWBase::Environment::get().getWorld()->getStore().get<ESM::Creature>().find(ref->mRef.mSoul);
            value *= creature->mData.mSoul;
        }

        return value;
    }

    void Miscellaneous::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Miscellaneous);

        registerClass (typeid (ESM::Miscellaneous).name(), instance);
    }

    std::string Miscellaneous::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        if (isGold(ptr))
            return std::string("Item Gold Up");
        return std::string("Item Misc Up");
    }

    std::string Miscellaneous::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        if (isGold(ptr))
            return std::string("Item Gold Down");
        return std::string("Item Misc Down");
    }

    std::string Miscellaneous::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
          MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();

        return ref->mBase->mIcon;
    }

    bool Miscellaneous::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();

        return (ref->mBase->mName != "");
    }

    MWGui::ToolTipInfo Miscellaneous::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();

        MWGui::ToolTipInfo info;

        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        int count = ptr.getRefData().getCount();

        bool gold = isGold(ptr);

        if (gold && ptr.getCellRef().mGoldValue != 1)
            count = ptr.getCellRef().mGoldValue;
        else if (gold)
            count *= ref->mBase->mData.mValue;

        std::string countString;
        if (!gold)
            countString = MWGui::ToolTips::getCountString(count);
        else // gold displays its count also if it's 1.
            countString = " (" + boost::lexical_cast<std::string>(count) + ")";

        info.caption = ref->mBase->mName + countString;
        info.icon = ref->mBase->mIcon;

        if (ref->mRef.mSoul != "")
        {
            const ESM::Creature *creature = store.get<ESM::Creature>().find(ref->mRef.mSoul);
            info.caption += " (" + creature->mName + ")";
        }

        std::string text;

        if (!gold)
        {
            text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.mWeight);
            text += MWGui::ToolTips::getValueString(getValue(ptr), "#{sValue}");
        }

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getMiscString(ref->mRef.mOwner, "Owner");
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }

        info.text = text;

        return info;
    }

    MWWorld::Ptr
    Miscellaneous::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::Ptr newPtr;

        const MWWorld::ESMStore &store =
            MWBase::Environment::get().getWorld()->getStore();

        if (isGold(ptr)) {
            int goldAmount = ptr.getRefData().getCount();

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
            MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
                newRef.getPtr().get<ESM::Miscellaneous>();
            newPtr = MWWorld::Ptr(&cell.mMiscItems.insert(*ref), &cell);
            newPtr.getCellRef().mGoldValue = goldAmount;
        } else {
            MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
                ptr.get<ESM::Miscellaneous>();
            newPtr = MWWorld::Ptr(&cell.mMiscItems.insert(*ref), &cell);
        }
        return newPtr;
    }

    boost::shared_ptr<MWWorld::Action> Miscellaneous::use (const MWWorld::Ptr& ptr) const
    {
        if (ptr.getCellRef().mSoul == "")
            return boost::shared_ptr<MWWorld::Action>(new MWWorld::NullAction());
        else
            return boost::shared_ptr<MWWorld::Action>(new MWWorld::ActionSoulgem(ptr));
    }

    bool Miscellaneous::canSell (const MWWorld::Ptr& item, int npcServices) const
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            item.get<ESM::Miscellaneous>();

        return !ref->mBase->mData.mIsKey && (npcServices & ESM::NPC::Misc);
    }

    float Miscellaneous::getWeight(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();
        return ref->mBase->mData.mWeight;
    }

}
