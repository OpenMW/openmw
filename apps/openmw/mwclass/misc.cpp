
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

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include <boost/lexical_cast.hpp>

namespace MWClass
{
    void Miscellaneous::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        const std::string model = getModel(ptr);
        if (!model.empty()) {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, model);
        }
    }

    void Miscellaneous::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty()) {
            physics.insertObjectPhysics(ptr, model);
        }
    }

    std::string Miscellaneous::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();
        assert(ref->base != NULL);

        const std::string &model = ref->base->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Miscellaneous::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();

        return ref->base->mName;
    }

    boost::shared_ptr<MWWorld::Action> Miscellaneous::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionTake (ptr));

        action->setSound(getUpSoundId(ptr));

        return action;
    }

    std::string Miscellaneous::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();

        return ref->base->mScript;
    }

    int Miscellaneous::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();

        return ref->base->mData.mValue;
    }

    void Miscellaneous::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Miscellaneous);

        registerClass (typeid (ESM::Miscellaneous).name(), instance);
    }

    std::string Miscellaneous::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();

        if (ref->base->mName == MWBase::Environment::get().getWorld()->getStore().gameSettings.find("sGold")->getString())
        {
            return std::string("Item Gold Up");
        }
        return std::string("Item Misc Up");
    }

    std::string Miscellaneous::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();

        if (ref->base->mName == MWBase::Environment::get().getWorld()->getStore().gameSettings.find("sGold")->getString())
        {
            return std::string("Item Gold Down");
        }
        return std::string("Item Misc Down");
    }

    std::string Miscellaneous::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
          MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();

        return ref->base->mIcon;
    }

    bool Miscellaneous::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();

        return (ref->base->mName != "");
    }

    MWGui::ToolTipInfo Miscellaneous::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();

        MWGui::ToolTipInfo info;

        const ESMS::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        int count = ptr.getRefData().getCount();

        bool isGold = (ref->base->mName == store.gameSettings.find("sGold")->getString());
        if (isGold && count == 1)
            count = ref->base->mData.mValue;

        std::string countString;
        if (!isGold)
            countString = MWGui::ToolTips::getCountString(count);
        else // gold displays its count also if it's 1.
            countString = " (" + boost::lexical_cast<std::string>(count) + ")";

        info.caption = ref->base->mName + countString;
        info.icon = ref->base->mIcon;

        if (ref->ref.mSoul != "")
        {
            const ESM::Creature *creature = store.creatures.search(ref->ref.mSoul);
            info.caption += " (" + creature->mName + ")";
        }

        std::string text;

        if (!isGold)
        {
            text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->base->mData.mWeight);
            text += MWGui::ToolTips::getValueString(ref->base->mData.mValue, "#{sValue}");
        }

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getMiscString(ref->ref.mOwner, "Owner");
            text += MWGui::ToolTips::getMiscString(ref->base->mScript, "Script");
        }

        info.text = text;

        return info;
    }

    MWWorld::Ptr
    Miscellaneous::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::Ptr newPtr;

        const ESMS::ESMStore &store =
            MWBase::Environment::get().getWorld()->getStore();

        if (MWWorld::Class::get(ptr).getName(ptr) == store.gameSettings.find("sGold")->getString()) {
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
            newPtr = MWWorld::Ptr(&cell.miscItems.insert(*ref), &cell);
        } else {
            MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
                ptr.get<ESM::Miscellaneous>();
            newPtr = MWWorld::Ptr(&cell.miscItems.insert(*ref), &cell);
        }
        return newPtr;
    }
}
