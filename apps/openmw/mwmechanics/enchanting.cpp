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
    mCastStyle(ESM::CS_CastOnce)
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

    int Enchanting::getCastStyle() const
    {
        return mCastStyle;
    }

    void Enchanting::setSoulGem(MWWorld::Ptr soulGem)
    {
        mSoulGemPtr=soulGem;
    }

    bool Enchanting::create()
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        ESM::Enchantment enchantment;
        enchantment.mData.mCharge = getGemCharge();

        mSoulGemPtr.getRefData().setCount (mSoulGemPtr.getRefData().getCount()-1);

        //Exception for Azura Star, new one will be added after enchanting
        if(boost::iequals(mSoulGemPtr.get<ESM::Miscellaneous>()->mBase->mId, "Misc_SoulGem_Azura"))
        {
            MWWorld::ManualRef azura (MWBase::Environment::get().getWorld()->getStore(), "Misc_SoulGem_Azura");
            MWWorld::Class::get (player).getContainerStore (player).add (azura.getPtr());
        }

        if(mSelfEnchanting)
        {
            if(getEnchantChance()<std::rand()/static_cast<double> (RAND_MAX)*100)
                return false;

            MWWorld::Class::get (mEnchanter).skillUsageSucceeded (mEnchanter, ESM::Skill::Enchant, 1);
        }

        if(mCastStyle==ESM::CS_ConstantEffect)
        {
            enchantment.mData.mCharge=0;
        }
        enchantment.mData.mType = mCastStyle;
        enchantment.mData.mCost = getEnchantCost();
        enchantment.mEffects = mEffectList;

        const ESM::Enchantment *enchantmentPtr = MWBase::Environment::get().getWorld()->createRecord (enchantment);

        MWWorld::Class::get(mOldItemPtr).applyEnchantment(mOldItemPtr, enchantmentPtr->mId, getGemCharge(), mNewItemName);

        mOldItemPtr.getRefData().setCount(1);

        MWWorld::ManualRef ref (MWBase::Environment::get().getWorld()->getStore(), mOldItemId);
        ref.getPtr().getRefData().setCount (mOldItemCount-1);

        MWWorld::Class::get (player).getContainerStore (player).add (ref.getPtr());
        if(!mSelfEnchanting)
            payForEnchantment();

        return true;
    }
    
    void Enchanting::nextCastStyle()
    {
        if (itemEmpty())
        {
            mCastStyle = ESM::CS_WhenUsed;
            return;
        }

        const bool powerfulSoul = getGemCharge() >= \
        		MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find ("iSoulAmountForConstantEffect")->getInt();
        if ((mObjectType == typeid(ESM::Armor).name()) || (mObjectType == typeid(ESM::Clothing).name()))
        { // Armor or Clothing
            switch(mCastStyle)
            {
                case ESM::CS_WhenUsed:
                    if (powerfulSoul)
                        mCastStyle = ESM::CS_ConstantEffect;
                    return;
                default: // takes care of Constant effect too
                    mCastStyle = ESM::CS_WhenUsed;
                    return;
            }
        }
        else if(mObjectType == typeid(ESM::Weapon).name())
        { // Weapon
            switch(mCastStyle)
            {
                case ESM::CS_WhenStrikes:
                	mCastStyle = ESM::CS_WhenUsed;
                	return;
                case ESM::CS_WhenUsed:
                	if (powerfulSoul)
                		mCastStyle = ESM::CS_ConstantEffect;
                	else
                		mCastStyle = ESM::CS_WhenStrikes;
                	return;
                default: // takes care of Constant effect too
                	mCastStyle = ESM::CS_WhenStrikes;
                	return;
            }
        }
        else if(mObjectType == typeid(ESM::Book).name())
        { // Scroll or Book
            mCastStyle = ESM::CS_CastOnce;
            return;
        }

        // Fail case
        mCastStyle = ESM::CS_CastOnce;
    }

	/*
	 * Vanilla enchant cost formula:
	 *
	 * 	Touch/Self:          (min + max) * baseCost * 0.025 * duration + area * baseCost * 0.025
	 * 	Target:       1.5 * ((min + max) * baseCost * 0.025 * duration + area * baseCost * 0.025)
	 * 	Constant eff:        (min + max) * baseCost * 2.5              + area * baseCost * 0.025
	 *
	 *	For multiple effects - cost of each effect is multiplied by number of effects that follows +1.
	 *
	 * 	Note: Minimal value inside formula for 'min' and 'max' is 1. So in vanilla:
	 * 		  (0 + 0) == (1 + 0) == (1 + 1) => 2 or (2 + 0) == (1 + 2) => 3
	 *
	 *  Formula on UESPWiki is not entirely correct.
	 */
    float Enchanting::getEnchantCost() const
    {
    	if (mEffectList.mList.empty())
    		// No effects added, cost = 0
    		return 0;

        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        std::vector<ESM::ENAMstruct> mEffects = mEffectList.mList;

        float enchantmentCost = 0;
        int effectsLeftCnt = mEffects.size();
        float baseCost, magnitudeCost, areaCost;
        int magMin, magMax, area;
        for (std::vector<ESM::ENAMstruct>::const_iterator it = mEffects.begin(); it != mEffects.end(); ++it)
        {
            baseCost = (store.get<ESM::MagicEffect>().find(it->mEffectID))->mData.mBaseCost;
            // To reflect vanilla behavior
            magMin = (it->mMagnMin == 0) ? 1 : it->mMagnMin;
            magMax = (it->mMagnMax == 0) ? 1 : it->mMagnMax;
            area = (it->mArea == 0) ? 1 : it->mArea;

            if (mCastStyle == ESM::CS_ConstantEffect)
            {
            	magnitudeCost = (magMin + magMax) * baseCost * 2.5;
            }
            else
            {
            	magnitudeCost = (magMin + magMax) * it->mDuration * baseCost * 0.025;
            	if(it->mRange == ESM::RT_Target)
            		magnitudeCost *= 1.5;
            }

            areaCost = area * 0.025 * baseCost;
            if (it->mRange == ESM::RT_Target)
            	areaCost *= 1.5;

            enchantmentCost += (magnitudeCost + areaCost) * effectsLeftCnt;
            --effectsLeftCnt;
        }

        return enchantmentCost;
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

    float Enchanting::getMaxEnchantValue() const
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
        if(mCastStyle==ESM::CS_ConstantEffect)
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
