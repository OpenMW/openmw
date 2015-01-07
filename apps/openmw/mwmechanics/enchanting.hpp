#ifndef GAME_MWMECHANICS_ENCHANTING_H
#define GAME_MWMECHANICS_ENCHANTING_H
#include <string>
#include "../mwworld/ptr.hpp"
#include <components/esm/effectlist.hpp>
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
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

        public:
            Enchanting();
            void setEnchanter(MWWorld::Ptr enchanter);
            void setSelfEnchanting(bool selfEnchanting);
            void setOldItem(MWWorld::Ptr oldItem);
            MWWorld::Ptr getOldItem() { return mOldItemPtr; }
            MWWorld::Ptr getGem() { return mSoulGemPtr; }
            void setNewItemName(const std::string& s);
            void setEffect(ESM::EffectList effectList);
            void setSoulGem(MWWorld::Ptr soulGem);
            bool create(); //Return true if created, false if failed.
            void nextCastStyle(); //Set enchant type to next possible type (for mOldItemPtr object)
            int getCastStyle() const;
            int getEnchantPoints() const;
            int getBaseCastCost() const; // To be saved in the enchantment's record
            int getEffectiveCastCost() const; // Effective cost taking player Enchant skill into account, used for preview purposes in the UI
            int getEnchantPrice() const;
            int getMaxEnchantValue() const;
            int getGemCharge() const;
            float getEnchantChance() const;
            bool soulEmpty() const; //Return true if empty
            bool itemEmpty() const; //Return true if empty
            void payForEnchantment() const;
    };
}
#endif
