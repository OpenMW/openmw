#include "enchanting.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "creaturestats.hpp"
#include "npcstats.hpp"
#include <boost/algorithm/string.hpp>

namespace MWMechanics
{
    Enchanting::Enchanting():
    mEnchantType(0)
    {}

    void Enchanting::setOldItem(MWWorld::Ptr oldItem)
    {
        mOldItemPtr=oldItem;
        if(!itemEmpty())
        {
            mObjectType = mOldItemPtr.getTypeName();
            mOldItemId = mOldItemPtr.getCellRef().mRefID;
            mOldItemCount = mOldItemPtr.getRefData().getCount();
        }
        else
        {
            mObjectType="";
            mOldItemId="";
        }
    }

    void Enchanting::setNewItemName(const std::string& s)
    {
        mNewItemName=s;
    }

    void Enchanting::setEffect(ESM::EffectList effectList)
    {
        mEffectList=effectList;
    }

    int Enchanting::getEnchantType() const
    {
        return mEnchantType;
    }

    void Enchanting::setSoulGem(MWWorld::Ptr soulGem)
    {
        mSoulGemPtr=soulGem;
    }

    bool Enchanting::create()
    {
        ESM::Enchantment enchantment;
        enchantment.mData.mCharge = getGemCharge();

        //Exception for Azura Star, it's not destroyed after enchanting
        if(boost::iequals(mSoulGemPtr.get<ESM::Miscellaneous>()->mBase->mId, "Misc_SoulGem_Azura"))
            mSoulGemPtr.getCellRef().mSoul="";
        else
            mSoulGemPtr.getRefData().setCount (mSoulGemPtr.getRefData().getCount()-1);

        if(mSelfEnchanting)
        {
            if(getEnchantChance()<std::rand()/static_cast<double> (RAND_MAX)*100)
                return false;

            MWWorld::Class::get (mEnchanter).skillUsageSucceeded (mEnchanter, ESM::Skill::Enchant, 1);
        }

        if(mEnchantType==3)
        {
            enchantment.mData.mCharge=0;
        }
        enchantment.mData.mType = mEnchantType;
        enchantment.mData.mCost = getEnchantCost();
        enchantment.mEffects = mEffectList;

        const ESM::Enchantment *enchantmentPtr = MWBase::Environment::get().getWorld()->createRecord (enchantment);

        MWWorld::Class::get(mOldItemPtr).applyEnchantment(mOldItemPtr, enchantmentPtr->mId, getGemCharge(), mNewItemName);

        mOldItemPtr.getRefData().setCount(1);

        MWWorld::ManualRef ref (MWBase::Environment::get().getWorld()->getStore(), mOldItemId);
        ref.getPtr().getRefData().setCount (mOldItemCount-1);

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        MWWorld::Class::get (player).getContainerStore (player).add (ref.getPtr());
        if(!mSelfEnchanting)
            payForEnchantment();

        return true;
    }
    
    void Enchanting::nextEnchantType()
    {
        mEnchantType++;
        if (itemEmpty())
        {
            mEnchantType = 0;
            return;
        }
        if ((mObjectType == typeid(ESM::Armor).name())||(mObjectType == typeid(ESM::Clothing).name()))
        {
            int soulConstAmount = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find ("iSoulAmountForConstantEffect")->getInt();
            switch(mEnchantType)
            {
                case 1:
                    mEnchantType = 2;
                case 3:
                    if(getGemCharge()<soulConstAmount)
                        mEnchantType=2;
                case 4:
                    mEnchantType = 2;
            }
        }
        else if(mObjectType == typeid(ESM::Weapon).name())
        {
            switch(mEnchantType)
            {
                case 3:
                    mEnchantType = 1;
            }
        }
        else if(mObjectType == typeid(ESM::Book).name())
        {
            mEnchantType=0;
        }
    }

    int Enchanting::getEnchantCost() const
    {
        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        float cost = 0;
        std::vector<ESM::ENAMstruct> mEffects = mEffectList.mList;
        int i=mEffects.size();
        if(i<=0)
            return 0;

        /*
        Formula from http://www.uesp.net/wiki/Morrowind:Enchant
        */
        for (std::vector<ESM::ENAMstruct>::const_iterator it = mEffects.begin(); it != mEffects.end(); ++it)
        {
            const ESM::MagicEffect* effect = store.get<ESM::MagicEffect>().find(it->mEffectID);

            float cost1 = ((it->mMagnMin + it->mMagnMax)*it->mDuration*effect->mData.mBaseCost*0.025);

            float cost2 = (std::max(1, it->mArea)*0.125*effect->mData.mBaseCost);

            if(mEnchantType==3)
            {
                int constDurationMultipler = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find ("fEnchantmentConstantDurationMult")->getFloat();
                cost1 *= constDurationMultipler;
                cost2 *= 2;
            }
            if(effect->mData.mFlags & ESM::MagicEffect::CastTarget)
                cost1 *= 1.5;

            float fullcost = cost1+cost2;
            fullcost*= i;
            i--;

            cost+=fullcost;
        }
        return cost;
    }

    int Enchanting::getEnchantPrice() const
    {
        if(mEnchanter.isEmpty())
            return 0;

        float priceMultipler = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find ("fEnchantmentValueMult")->getFloat();
        int price = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mEnchanter, (getEnchantCost() * priceMultipler), true);
        return price;
    }

    int Enchanting::getGemCharge() const
    {
        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        if(soulEmpty())
            return 0;
        if(mSoulGemPtr.getCellRef().mSoul=="")
            return 0;
        const ESM::Creature* soul = store.get<ESM::Creature>().find(mSoulGemPtr.getCellRef().mSoul);
        return soul->mData.mSoul;
    }

    int Enchanting::getMaxEnchantValue() const
    {
        if (itemEmpty())
            return 0;
        return MWWorld::Class::get(mOldItemPtr).getEnchantmentPoints(mOldItemPtr);
    }
    bool Enchanting::soulEmpty() const
    {
        if (mSoulGemPtr.isEmpty())
            return true;
        return false;
    }

    bool Enchanting::itemEmpty() const
    {
        if(mOldItemPtr.isEmpty())
            return true;
        return false;
    }

    void Enchanting::setSelfEnchanting(bool selfEnchanting)
    {
        mSelfEnchanting = selfEnchanting;
    }

    void Enchanting::setEnchanter(MWWorld::Ptr enchanter)
    {
        mEnchanter = enchanter;
    }

    float Enchanting::getEnchantChance() const
    {
        /*
        Formula from http://www.uesp.net/wiki/Morrowind:Enchant
        */
        const CreatureStats& creatureStats = MWWorld::Class::get (mEnchanter).getCreatureStats (mEnchanter);
        const NpcStats& npcStats = MWWorld::Class::get (mEnchanter).getNpcStats (mEnchanter);

        float chance1 = (npcStats.getSkill (ESM::Skill::Enchant).getModified() + 
        (0.25 * creatureStats.getAttribute (ESM::Attribute::Intelligence).getModified())
        + (0.125 * creatureStats.getAttribute (ESM::Attribute::Luck).getModified()));

        float chance2 = 2.5 * getEnchantCost();
        if(mEnchantType==3)
        {
            float constantChance = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find ("fEnchantmentConstantChanceMult")->getFloat();
            chance2 /= constantChance;
        }
        return (chance1-chance2);
    }

    void Enchanting::payForEnchantment() const
    {
        MWWorld::Ptr gold;

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        MWWorld::ContainerStore& store = MWWorld::Class::get(player).getContainerStore(player);

        for (MWWorld::ContainerStoreIterator it = store.begin();
                it != store.end(); ++it)
        {
            if (Misc::StringUtils::ciEqual(it->getCellRef().mRefID, "gold_001"))
            {
                gold = *it;
            }
        }

        gold.getRefData().setCount(gold.getRefData().getCount() - getEnchantPrice());
    }
}
