#include "pickpocketitemmodel.hpp"

#include <components/misc/rng.hpp>
#include <components/esm/loadskil.hpp>

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/pickpocket.hpp"

#include "../mwworld/class.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

namespace MWGui
{

    PickpocketItemModel::PickpocketItemModel(const MWWorld::Ptr& actor, ItemModel *sourceModel, bool hideItems)
        : mActor(actor)
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        mSourceModel = sourceModel;
        int chance = player.getClass().getSkill(player, ESM::Skill::Sneak);

        mSourceModel->update();

        // build list of items that player is unable to find when attempts to pickpocket.
        if (hideItems)
        {
            for (size_t i = 0; i<mSourceModel->getItemCount(); ++i)
            {
                if (Misc::Rng::roll0to99() > chance)
                    mHiddenItems.push_back(mSourceModel->getItem(i));
            }
        }
    }

    bool PickpocketItemModel::allowedToUseItems() const
    {
        return false;
    }

    ItemStack PickpocketItemModel::getItem (ModelIndex index)
    {
        if (index < 0)
            throw std::runtime_error("Invalid index supplied");
        if (mItems.size() <= static_cast<size_t>(index))
            throw std::runtime_error("Item index out of range");
        return mItems[index];
    }

    size_t PickpocketItemModel::getItemCount()
    {
        return mItems.size();
    }

    void PickpocketItemModel::update()
    {
        mSourceModel->update();
        mItems.clear();
        for (size_t i = 0; i<mSourceModel->getItemCount(); ++i)
        {
            const ItemStack& item = mSourceModel->getItem(i);

            // Bound items may not be stolen
            if (item.mFlags & ItemStack::Flag_Bound)
                continue;

            if (std::find(mHiddenItems.begin(), mHiddenItems.end(), item) == mHiddenItems.end()
                    && item.mType != ItemStack::Type_Equipped)
                mItems.push_back(item);
        }
    }

    void PickpocketItemModel::removeItem (const ItemStack &item, size_t count)
    {
        ProxyItemModel::removeItem(item, count);
        /// \todo check if player is detected
    }

    bool PickpocketItemModel::allowedToInsertItems() const
    {
        // don't allow "reverse pickpocket" (yet)
        return false;
    }

    bool PickpocketItemModel::onTakeItem(const MWWorld::Ptr &item, int count) const
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWMechanics::Pickpocket pickpocket(player, mActor);
        if (pickpocket.pick(item, count))
        {
            MWBase::Environment::get().getMechanicsManager()->commitCrime(
                        player, mActor, MWBase::MechanicsManager::OT_Pickpocket, 0, true);
            MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Container);
            return false;
        }
        else
            player.getClass().skillUsageSucceeded(player, ESM::Skill::Sneak, 1);

        return true;
    }
}
