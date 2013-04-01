#include "enchanting.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
namespace MWMechanics
{
    Enchanting::Enchanting(MWWorld::Ptr enchanter):
    mEnchanter(enchanter),
    mEnchantType(0)
    {}

    void Enchanting::setOldItem(MWWorld::Ptr oldItem)
    {
        mOldItemPtr=oldItem;
        if(!itemEmpty())
        {
            mObjectType = mOldItemPtr.getTypeName();
            mOldItemId = mOldItemPtr.getCellRef().mRefID;
        }
        else
        {
            mObjectType="";
            mOldItemId="";
        }
    }

    void Enchanting::setNewItemName(std::string s)
    {
        mNewItemName=s;
    }

    void Enchanting::setEffect(ESM::EffectList effectList)
    {
        mEffectList=effectList;
    }

    int Enchanting::getEnchantType()
    {
        return mEnchantType;
    }

    void Enchanting::setSoulGem(MWWorld::Ptr soulGem)
    {
        mSoulGemPtr=soulGem;
    }

    void Enchanting::create()
    {
        mOldItemPtr.getRefData().setCount(mOldItemPtr.getRefData().getCount()-1);
        mSoulGemPtr.getRefData().setCount(mSoulGemPtr.getRefData().getCount()-1);

        mEnchantment.mData.mCharge = getGemCharge();
        if(mEnchantType==3)
        {
            mEnchantment.mData.mCharge=0;
        }
        mEnchantment.mData.mType = mEnchantType;
        mEnchantment.mData.mCost = getEnchantCost();
        mEnchantment.mEffects = mEffectList;
        const ESM::Enchantment *enchantment = MWBase::Environment::get().getWorld()->createRecord (mEnchantment);

        std::string newobjId = MWWorld::Class::get(mOldItemPtr).applyEnchantment(mOldItemPtr, enchantment->mId, getGemCharge(), mNewItemName);

        MWWorld::ManualRef ref (MWBase::Environment::get().getWorld()->getStore(), newobjId);
        MWWorld::Ptr newobjPtr = ref.getPtr();
        MWWorld::Ptr result = mOldItemPtr;
        result.mPtr = newobjPtr.mPtr;
        MWWorld::Class::get (mEnchanter).getContainerStore (mEnchanter).add (result);
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
            switch(mEnchantType)
            {
                case 1:
                    mEnchantType = 2;
                case 3:
                    if(getGemCharge()<400)
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

    int Enchanting::getEnchantCost()
    {
        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        float cost = 0;
        std::vector<ESM::ENAMstruct> mEffects = mEffectList.mList;
        int i=mEffects.size();
        for (std::vector<ESM::ENAMstruct>::const_iterator it = mEffects.begin(); it != mEffects.end(); ++it)
        {
            const ESM::MagicEffect* effect = store.get<ESM::MagicEffect>().find(it->mEffectID);

            float cost1 = ((it->mMagnMin + it->mMagnMax)*it->mDuration*effect->mData.mBaseCost*0.025);

            float cost2 = (std::max(1, it->mArea)*0.125*effect->mData.mBaseCost);

            if(mEnchantType==3)
            {
                cost1 *= 100;
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
    int Enchanting::getGemCharge()
    {
        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        if(soulEmpty())
            return 0;
        if(mSoulGemPtr.getCellRef().mSoul=="")
            return 0;
        const ESM::Creature* soul = store.get<ESM::Creature>().find(mSoulGemPtr.getCellRef().mSoul);
        return soul->mData.mSoul;
    }

    int Enchanting::getMaxEnchantValue()
    {
        if (itemEmpty())
            return 0;
        return MWWorld::Class::get(mOldItemPtr).getEnchantmentPoints(mOldItemPtr);
    }
    bool Enchanting::soulEmpty()
    {
        if (mSoulGemPtr.isEmpty())
            return true;
        return false;
    }

    bool Enchanting::itemEmpty()
    {
        if(mOldItemPtr.isEmpty())
            return true;
        return false;
    }
}
