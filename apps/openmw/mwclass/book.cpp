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
#include "../mwworld/esmstore.hpp"
#include "../mwphysics/physicssystem.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwmechanics/npcstats.hpp"

namespace MWClass
{
    std::string Book::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM::Book>()->mBase->mId;
    }

    void Book::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    void Book::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWPhysics::PhysicsSystem& physics) const
    {
        // TODO: add option somewhere to enable collision for placeable objects
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
        if(actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
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
        text += MWGui::ToolTips::getValueString(ref->mBase->mData.mValue, "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
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

    std::string Book::applyEnchantment(const MWWorld::Ptr &ptr, const std::string& enchId, int enchCharge, const std::string& newName) const
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
        return record->mId;
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

        return MWWorld::Ptr(&cell.get<ESM::Book>().insert(*ref), &cell);
    }

    int Book::getEnchantmentPoints (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Book> *ref =
                ptr.get<ESM::Book>();

        return ref->mBase->mData.mEnchant;
    }

    bool Book::canSell (const MWWorld::Ptr& item, int npcServices) const
    {
        return (npcServices & ESM::NPC::Books)
                || ((npcServices & ESM::NPC::MagicItems) && !getEnchantment(item).empty());
    }

    float Book::getWeight(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Book> *ref =
            ptr.get<ESM::Book>();
        return ref->mBase->mData.mWeight;
    }
}
