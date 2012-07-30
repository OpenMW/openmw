
#include "misc.hpp"

#include <boost/lexical_cast.hpp>

#include <components/esm/loadmisc.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/manualref.hpp"

#include "../mwgui/window_manager.hpp"
#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwsound/soundmanager.hpp"

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

        const std::string &model = ref->base->model;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Miscellaneous::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Miscellaneous::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        MWBase::Environment::get().getSoundManager()->playSound3D (ptr, getUpSoundId(ptr), 1.0, 1.0, MWSound::Play_NoTrack);

        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionTake (ptr));
    }

    std::string Miscellaneous::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();

        return ref->base->script;
    }

    int Miscellaneous::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();

        return ref->base->data.value;
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

        if (ref->base->name == MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sGold")->str)
        {
            return std::string("Item Gold Up");
        }
        return std::string("Item Misc Up");
    }

    std::string Miscellaneous::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();

        if (ref->base->name == MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sGold")->str)
        {
            return std::string("Item Gold Down");
        }
        return std::string("Item Misc Down");
    }

    std::string Miscellaneous::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
          MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();

        return ref->base->icon;
    }

    bool Miscellaneous::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();

        return (ref->base->name != "");
    }

    MWGui::ToolTipInfo Miscellaneous::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *ref =
            ptr.get<ESM::Miscellaneous>();

        MWGui::ToolTipInfo info;

        const ESMS::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        int count = ptr.getRefData().getCount();

        bool isGold = (ref->base->name == store.gameSettings.search("sGold")->str);
        if (isGold && count == 1)
            count = ref->base->data.value;

        std::string countString;
        if (!isGold)
            countString = MWGui::ToolTips::getCountString(count);
        else // gold displays its count also if it's 1.
            countString = " (" + boost::lexical_cast<std::string>(count) + ")";

        info.caption = ref->base->name + countString;
        info.icon = ref->base->icon;

        if (ref->ref.soul != "")
        {
            const ESM::Creature *creature = store.creatures.search(ref->ref.soul);
            info.caption += " (" + creature->name + ")";
        }

        std::string text;

        if (!isGold)
        {
            text += "\n" + store.gameSettings.search("sWeight")->str + ": " + MWGui::ToolTips::toString(ref->base->data.weight);
            text += MWGui::ToolTips::getValueString(ref->base->data.value, store.gameSettings.search("sValue")->str);
        }

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getMiscString(ref->ref.owner, "Owner");
            text += MWGui::ToolTips::getMiscString(ref->base->script, "Script");
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

        if (MWWorld::Class::get(ptr).getName(ptr) == store.gameSettings.search("sGold")->str) {
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
