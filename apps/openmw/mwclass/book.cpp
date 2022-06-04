#include "book.hpp"

#include <MyGUI_TextIterator.h>

#include <components/esm3/loadbook.hpp>

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
    Book::Book()
        : MWWorld::RegisteredClass<Book>(ESM::Book::sRecordId)
    {
    }

    void Book::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    std::string Book::getModel(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Book> *ref = ptr.get<ESM::Book>();

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Book::getName (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Book> *ref = ptr.get<ESM::Book>();
        const std::string& name = ref->mBase->mName;

        return !name.empty() ? name : ref->mBase->mId;
    }

    std::unique_ptr<MWWorld::Action> Book::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        if(actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
        {
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            auto& prng = MWBase::Environment::get().getWorld()->getPrng();
            const ESM::Sound *sound = store.get<ESM::Sound>().searchRandom("WolfItem", prng);

            std::unique_ptr<MWWorld::Action> action = std::make_unique<MWWorld::FailedAction>("#{sWerewolfRefusal}");
            if(sound) action->setSound(sound->mId);

            return action;
        }

        return std::make_unique<MWWorld::ActionRead>(ptr);
    }

    std::string Book::getScript (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Book> *ref = ptr.get<ESM::Book>();

        return ref->mBase->mScript;
    }

    int Book::getValue (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Book> *ref = ptr.get<ESM::Book>();

        return ref->mBase->mData.mValue;
    }

    std::string Book::getUpSoundId (const MWWorld::ConstPtr& ptr) const
    {
        return std::string("Item Book Up");
    }

    std::string Book::getDownSoundId (const MWWorld::ConstPtr& ptr) const
    {
        return std::string("Item Book Down");
    }

    std::string Book::getInventoryIcon (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Book> *ref = ptr.get<ESM::Book>();

        return ref->mBase->mIcon;
    }

    MWGui::ToolTipInfo Book::getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Book> *ref = ptr.get<ESM::Book>();

        MWGui::ToolTipInfo info;
        info.caption = MyGUI::TextIterator::toTagsString(getName(ptr)) + MWGui::ToolTips::getCountString(count);
        info.icon = ref->mBase->mIcon;

        std::string text;

        text += MWGui::ToolTips::getWeightString(ref->mBase->mData.mWeight, "#{sWeight}");
        text += MWGui::ToolTips::getValueString(ref->mBase->mData.mValue, "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }

        info.enchant = ref->mBase->mEnchant;

        info.text = text;

        return info;
    }

    std::string Book::getEnchantment (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Book> *ref = ptr.get<ESM::Book>();

        return ref->mBase->mEnchant;
    }

    std::string Book::applyEnchantment(const MWWorld::ConstPtr &ptr, const std::string& enchId, int enchCharge, const std::string& newName) const
    {
        const MWWorld::LiveCellRef<ESM::Book> *ref = ptr.get<ESM::Book>();

        ESM::Book newItem = *ref->mBase;
        newItem.mId.clear();
        newItem.mName=newName;
        newItem.mData.mIsScroll = 1;
        newItem.mData.mEnchant=enchCharge;
        newItem.mEnchant=enchId;
        const ESM::Book *record = MWBase::Environment::get().getWorld()->createRecord (newItem);
        return record->mId;
    }

    std::unique_ptr<MWWorld::Action> Book::use (const MWWorld::Ptr& ptr, bool force) const
    {
        return std::make_unique<MWWorld::ActionRead>(ptr);
    }

    MWWorld::Ptr Book::copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const
    {
        const MWWorld::LiveCellRef<ESM::Book> *ref = ptr.get<ESM::Book>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    int Book::getEnchantmentPoints (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Book> *ref = ptr.get<ESM::Book>();

        return ref->mBase->mData.mEnchant;
    }

    bool Book::canSell (const MWWorld::ConstPtr& item, int npcServices) const
    {
        return (npcServices & ESM::NPC::Books)
                || ((npcServices & ESM::NPC::MagicItems) && !getEnchantment(item).empty());
    }

    float Book::getWeight(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Book> *ref = ptr.get<ESM::Book>();
        return ref->mBase->mData.mWeight;
    }
}
