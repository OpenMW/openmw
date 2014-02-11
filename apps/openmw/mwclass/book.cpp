#include "book.hpp"

#include <components/esm/loadbook.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actionread.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/physicssystem.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwmechanics/npcstats.hpp"

namespace MWClass
{
    void Book::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        const std::string model = getModel(ptr);
        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    void Book::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty())
            physics.addObject(ptr,true);
    }

    std::string Book::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Book> *ref =
            ptr.get<ESM::Book>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Book::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Book> *ref =
            ptr.get<ESM::Book>();

        return ref->mBase->mName;
    }

    boost::shared_ptr<MWWorld::Action> Book::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        if(get(actor).isNpc() && get(actor).getNpcStats(actor).isWerewolf())
        {
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const ESM::Sound *sound = store.get<ESM::Sound>().searchRandom("WolfItem");

            boost::shared_ptr<MWWorld::Action> action(new MWWorld::FailedAction("#{sWerewolfRefusal}"));
            if(sound) action->setSound(sound->mId);

            return action;
        }

        return boost::shared_ptr<MWWorld::Action>(new MWWorld::ActionRead(ptr));
    }

    std::string Book::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Book> *ref =
            ptr.get<ESM::Book>();

        return ref->mBase->mScript;
    }

    int Book::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Book> *ref =
            ptr.get<ESM::Book>();

        return ref->mBase->mData.mValue;
    }

    void Book::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Book);

        registerClass (typeid (ESM::Book).name(), instance);
    }

    std::string Book::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        return std::string("Item Book Up");
    }

    std::string Book::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        return std::string("Item Book Down");
    }

    std::string Book::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
          MWWorld::LiveCellRef<ESM::Book> *ref =
            ptr.get<ESM::Book>();

        return ref->mBase->mIcon;
    }

    bool Book::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Book> *ref =
            ptr.get<ESM::Book>();

        return (ref->mBase->mName != "");
    }

    MWGui::ToolTipInfo Book::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Book> *ref =
            ptr.get<ESM::Book>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        info.icon = ref->mBase->mIcon;

        std::string text;

        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.mWeight);
        text += MWGui::ToolTips::getValueString(getValue(ptr), "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getMiscString(ref->mRef.mOwner, "Owner");
            text += MWGui::ToolTips::getMiscString(ref->mRef.mFaction, "Faction");
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }

        info.enchant = ref->mBase->mEnchant;

        info.text = text;

        return info;
    }

    std::string Book::getEnchantment (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Book> *ref =
            ptr.get<ESM::Book>();

        return ref->mBase->mEnchant;
    }

    void Book::applyEnchantment(const MWWorld::Ptr &ptr, const std::string& enchId, int enchCharge, const std::string& newName) const
    {
        MWWorld::LiveCellRef<ESM::Book> *ref =
            ptr.get<ESM::Book>();

        ESM::Book newItem = *ref->mBase;
        newItem.mId="";
        newItem.mName=newName;
        newItem.mData.mIsScroll = 1;
        newItem.mData.mEnchant=enchCharge;
        newItem.mEnchant=enchId;
        const ESM::Book *record = MWBase::Environment::get().getWorld()->createRecord (newItem);
        ref->mBase = record;
        ref->mRef.mRefID = record->mId;
    }

    boost::shared_ptr<MWWorld::Action> Book::use (const MWWorld::Ptr& ptr) const
    {
        return boost::shared_ptr<MWWorld::Action>(new MWWorld::ActionRead(ptr));
    }

    MWWorld::Ptr
    Book::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Book> *ref =
            ptr.get<ESM::Book>();

        return MWWorld::Ptr(&cell.mBooks.insert(*ref), &cell);
    }

    int Book::getEnchantmentPoints (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Book> *ref =
                ptr.get<ESM::Book>();

        return ref->mBase->mData.mEnchant;
    }

    bool Book::canSell (const MWWorld::Ptr& item, int npcServices) const
    {
        return npcServices & ESM::NPC::Books;
    }

    float Book::getWeight(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Book> *ref =
            ptr.get<ESM::Book>();
        return ref->mBase->mData.mWeight;
    }
}
