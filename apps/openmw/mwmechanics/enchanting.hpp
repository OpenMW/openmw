#ifndef GAME_MWMECHANICS_ENCHANTING_H
#define GAME_MWMECHANICS_ENCHANTING_H

#include <string>

#include <components/esm/effectlist.hpp>
#include <components/esm/loadench.hpp>

#include "../mwworld/ptr.hpp"

namespace MWMechanics
{
    class Enchanting
    {
            MWWorld::Ptr mOldItemPtr;
            MWWorld::Ptr mSoulGemPtr;
            MWWorld::Ptr mEnchanter;

            int mCastStyle;

            bool mSelfEnchanting;

            ESM::EffectList mEffectList;

            std::string mNewItemName;
            std::string mObjectType;
            int mWeaponType;

            const ESM::Enchantment* getRecord(const ESM::Enchantment& newEnchantment) const;

        public:
            Enchanting();
            void setEnchanter(const MWWorld::Ptr& enchanter);
            void setSelfEnchanting(bool selfEnchanting);
            void setOldItem(const MWWorld::Ptr& oldItem);
            MWWorld::Ptr getOldItem() { return mOldItemPtr; }
            MWWorld::Ptr getGem() { return mSoulGemPtr; }
            void setNewItemName(const std::string& s);
            void setEffect(const ESM::EffectList& effectList);
            void setSoulGem(const MWWorld::Ptr& soulGem);
            bool create(); //Return true if created, false if failed.
            void nextCastStyle(); //Set enchant type to next possible type (for mOldItemPtr object)
            int getCastStyle() const;
            float getEnchantPoints(bool precise = true) const;
            int getBaseCastCost() const; // To be saved in the enchantment's record
            int getEffectiveCastCost() const; // Effective cost taking player Enchant skill into account, used for preview purposes in the UI
            int getEnchantPrice() const;
            int getMaxEnchantValue() const;
            int getGemCharge() const;
            int getEnchantChance() const;
            int getEnchantItemsCount() const;
            float getTypeMultiplier() const;
            bool soulEmpty() const; //Return true if empty
            bool itemEmpty() const; //Return true if empty
            void payForEnchantment() const;
    };
}
#endif
