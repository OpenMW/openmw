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
            const MWWorld::Ptr *mNewItemPtr;
            int mEnchantType;

            bool mSelfEnchanting;

            ESM::EffectList mEffectList;
            ESM::Enchantment mEnchantment;

            std::string mNewItemName;
            std::string mObjectType;
            std::string mOldItemId;
        public:
            Enchanting();
            void setEnchanter(MWWorld::Ptr enchanter);
            void setSelfEnchanting(bool selfEnchanting);
            void setOldItem(MWWorld::Ptr oldItem);
            void setNewItemName(std::string s);
            void setEffect(ESM::EffectList effectList);
            void setSoulGem(MWWorld::Ptr soulGem);
            int create();
            void nextEnchantType();
            int getEnchantType();
            int getEnchantCost();
            int getMaxEnchantValue();
            int getGemCharge();
            float getEnchantChance();
            bool soulEmpty();
            bool itemEmpty();
    };
}
#endif
