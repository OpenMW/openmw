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

            ESM::EffectList mEffectList;
            ESM::Enchantment mEnchantment;

            std::string mNewItemName;
            std::string mObjectType;
            std::string mOldItemId;
        public:
            Enchanting(MWWorld::Ptr enchanter);
            void setOldItem(MWWorld::Ptr oldItem);
            void setNewItemName(std::string s);
            void setEffect(ESM::EffectList effectList);
            void setSoulGem(MWWorld::Ptr soulGem);
            void create();
            void nextEnchantType();
            int getEnchantType();
            int getEnchantCost();
            int getMaxEnchantValue();
            int getGemCharge();
            bool soulEmpty();
            bool itemEmpty();
    };
}
#endif
