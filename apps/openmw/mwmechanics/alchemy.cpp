
#include "alchemy.hpp"

#include <cassert>
#include <cstdlib>

#include <algorithm>
#include <stdexcept>

#include <components/esm/loadskil.hpp>
#include <components/esm/loadappa.hpp>
#include <components/esm/loadgmst.hpp>
#include <components/esm/loadmgef.hpp>

#include <components/esm_store/store.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/manualref.hpp"

#include "magiceffects.hpp"
#include "creaturestats.hpp"
#include "npcstats.hpp"

std::set<MWMechanics::EffectKey> MWMechanics::Alchemy::listEffects() const
{
    std::set<EffectKey> effects;
    
    for (TIngredientsIterator iter (mIngredients.begin()); iter!=mIngredients.end(); ++iter)
    {
        if (!iter->isEmpty())
        {
            const MWWorld::LiveCellRef<ESM::Ingredient> *ingredient = iter->get<ESM::Ingredient>();
            
            for (int i=0; i<4; ++i)
                if (ingredient->base->mData.mEffectID[i]!=-1)
                    effects.insert (EffectKey (
                        ingredient->base->mData.mEffectID[i], ingredient->base->mData.mSkills[i]!=-1 ?
                        ingredient->base->mData.mSkills[i] : ingredient->base->mData.mAttributes[i]));
        }
    }
    
    return effects;
}

void MWMechanics::Alchemy::filterEffects (std::set<EffectKey>& effects) const
{
    std::set<EffectKey>::iterator iter = effects.begin();
    
    while (iter!=effects.end())
    {
        bool remove = false;
        
        const EffectKey& key = *iter;
        
        { // dodge pointless g++ warning
            for (TIngredientsIterator iter (mIngredients.begin()); iter!=mIngredients.end(); ++iter)
            {
                bool found = false;

                if (iter->isEmpty())
                    continue;

                const MWWorld::LiveCellRef<ESM::Ingredient> *ingredient = iter->get<ESM::Ingredient>();       

                for (int i=0; i<4; ++i)
                    if (key.mId==ingredient->base->mData.mEffectID[i] &&
                        (key.mArg==ingredient->base->mData.mSkills[i] ||
                        key.mArg==ingredient->base->mData.mAttributes[i]))
                    {
                        found = true;
                        break;
                    }
            
                if (!found)
                {
                    remove = true;
                    break;
                }
            }
        }       
        
        if (remove)
            effects.erase (iter++);
        else
            ++iter;
    }
}

void MWMechanics::Alchemy::applyTools (int flags, float& value) const
{
    bool magnitude = !(flags & ESM::MagicEffect::NoMagnitude);
    bool duration = !(flags & ESM::MagicEffect::NoDuration);
    bool negative = flags & (ESM::MagicEffect::Negative | ESM::MagicEffect::Harmful);

    int tool = negative ? ESM::Apparatus::Retort : ESM::Apparatus::Albemic;

    int setup = 0;
    
    if (!mTools[tool].isEmpty() && !mTools[ESM::Apparatus::Calcinator].isEmpty())
        setup = 1;
    else if (!mTools[tool].isEmpty())
        setup = 2;
    else if (!mTools[ESM::Apparatus::Calcinator].isEmpty())
        setup = 3;
    else
        return;

    float toolQuality = setup==1 || setup==2 ? mTools[tool].get<ESM::Apparatus>()->base->mData.mQuality : 0;
    float calcinatorQuality = setup==1 || setup==3 ?
        mTools[ESM::Apparatus::Calcinator].get<ESM::Apparatus>()->base->mData.mQuality : 0;

    float quality = 1;
    
    switch (setup)
    {
        case 1:
        
            quality = negative ? 2 * toolQuality + 3 * calcinatorQuality :
                (magnitude && duration ?
                2 * toolQuality + calcinatorQuality : 2/3.0 * (toolQuality + calcinatorQuality) + 0.5);
            break;
        
        case 2:
        
            quality = negative ? 1+toolQuality : (magnitude && duration ? toolQuality : toolQuality + 0.5);
            break;
        
        case 3:
        
            quality = magnitude && duration ? calcinatorQuality : calcinatorQuality + 0.5;
            break;
    }

    if (setup==3 || !negative)
    {
        value += quality;
    }
    else
    {
        if (quality==0)
            throw std::runtime_error ("invalid derived alchemy apparatus quality"); 
            
        value /= quality;
    }
}

void MWMechanics::Alchemy::updateEffects()
{
    mEffects.clear();
    mValue = 0;

    if (countIngredients()<2 || mAlchemist.isEmpty() || mTools[ESM::Apparatus::MortarPestle].isEmpty())
        return;

    // find effects
    std::set<EffectKey> effects (listEffects());
    filterEffects (effects);

    // general alchemy factor
    float x = getChance();

    x *= mTools[ESM::Apparatus::MortarPestle].get<ESM::Apparatus>()->base->mData.mQuality;
    x *= MWBase::Environment::get().getWorld()->getStore().gameSettings.find ("fPotionStrengthMult")->getFloat();

    // value
    mValue = static_cast<int> (
        x * MWBase::Environment::get().getWorld()->getStore().gameSettings.find ("iAlchemyMod")->getFloat());

    // build quantified effect list
    for (std::set<EffectKey>::const_iterator iter (effects.begin()); iter!=effects.end(); ++iter)
    {
        const ESM::MagicEffect *magicEffect =
            MWBase::Environment::get().getWorld()->getStore().magicEffects.find (iter->mId);         
        
        if (magicEffect->mData.mBaseCost<=0)
            throw std::runtime_error ("invalid base cost for magic effect " + iter->mId);
        
        float fPotionT1MagMul =
            MWBase::Environment::get().getWorld()->getStore().gameSettings.find ("fPotionT1MagMult")->getFloat();

        if (fPotionT1MagMul<=0)
            throw std::runtime_error ("invalid gmst: fPotionT1MagMul");
        
        float fPotionT1DurMult =
            MWBase::Environment::get().getWorld()->getStore().gameSettings.find ("fPotionT1DurMult")->getFloat();

        if (fPotionT1DurMult<=0)
            throw std::runtime_error ("invalid gmst: fPotionT1DurMult");

        float magnitude = magicEffect->mData.mFlags & ESM::MagicEffect::NoMagnitude ?
            1 : (x / fPotionT1MagMul) / magicEffect->mData.mBaseCost;
        float duration = magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration ?
            1 : (x / fPotionT1DurMult) / magicEffect->mData.mBaseCost;

        if (!(magicEffect->mData.mFlags & ESM::MagicEffect::NoMagnitude))
            applyTools (magicEffect->mData.mFlags, magnitude);

        if (!(magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration))
            applyTools (magicEffect->mData.mFlags, duration);
        
        duration = static_cast<int> (duration+0.5);           
        magnitude = static_cast<int> (magnitude+0.5);

        if (magnitude>0 && duration>0)
        {
            ESM::ENAMstruct effect;
            effect.mEffectID = iter->mId;
            
            effect.mSkill = effect.mAttribute = iter->mArg; // somewhat hack-ish, but should work
            
            effect.mRange = 0;
            effect.mArea = 0;       
            
            effect.mDuration = duration;
            effect.mMagnMin = effect.mMagnMax = magnitude;

            mEffects.push_back (effect);
        }   
    }
}

const ESM::Potion *MWMechanics::Alchemy::getRecord() const
{
    for (ESMS::RecListWithIDT<ESM::Potion>::MapType::const_iterator iter (
        MWBase::Environment::get().getWorld()->getStore().potions.list.begin());
        iter!=MWBase::Environment::get().getWorld()->getStore().potions.list.end(); ++iter)
    {
        if (iter->second.mEffects.mList.size()!=mEffects.size())
            continue;
         
        bool mismatch = false; 

        for (int i=0; i<static_cast<int> (iter->second.mEffects.mList.size()); ++iter)
        {
            const ESM::ENAMstruct& first = iter->second.mEffects.mList[i];
            const ESM::ENAMstruct& second = mEffects[i];
                
            if (first.mEffectID!=second.mEffectID ||
                first.mArea!=second.mArea ||
                first.mRange!=second.mRange ||
                first.mSkill!=second.mSkill ||
                first.mAttribute!=second.mAttribute ||
                first.mMagnMin!=second.mMagnMin ||
                first.mMagnMax!=second.mMagnMax ||
                first.mDuration!=second.mDuration)
            {
                mismatch = true;
                break;
            }
        }
        
        if (!mismatch)
            return &iter->second;
    }
    
    return 0;
}

void MWMechanics::Alchemy::removeIngredients()
{
    bool needsUpdate = false;
    
    for (TIngredientsContainer::iterator iter (mIngredients.begin()); iter!=mIngredients.end(); ++iter)
        if (!iter->isEmpty())
        {
            iter->getRefData().setCount (iter->getRefData().getCount()-1);
            if (iter->getRefData().getCount()<1)
            {
                needsUpdate = true;
                *iter = MWWorld::Ptr();
            }
        }
    
    if (needsUpdate)
        updateEffects();
}

void MWMechanics::Alchemy::addPotion (const std::string& name)
{
    const ESM::Potion *record = getRecord();
    
    if (!record)
    {
        ESM::Potion newRecord;
        
        newRecord.mData.mWeight = 0;
        
        for (TIngredientsIterator iter (beginIngredients()); iter!=endIngredients(); ++iter)
            if (!iter->isEmpty())
                newRecord.mData.mWeight += iter->get<ESM::Ingredient>()->base->mData.mWeight;
            
        newRecord.mData.mWeight /= countIngredients();
        
        newRecord.mData.mValue = mValue;
        newRecord.mData.mAutoCalc = 0;
        
        newRecord.mName = name;

        int index = static_cast<int> (std::rand()/static_cast<double> (RAND_MAX)*6);
        assert (index>=0 && index<6);
        
        static const char *name[] = { "standard", "bargain", "cheap", "fresh", "exclusive", "quality" };
        
        newRecord.mModel = "m\\misc_potion_" + std::string (name[index]) + "_01.nif";
        newRecord.mIcon = "m\\tx_potion_" + std::string (name[index]) + "_01.dds";
        
        newRecord.mEffects.mList = mEffects;
        
        record = MWBase::Environment::get().getWorld()->createRecord (newRecord).second;
    }
    
    MWWorld::ManualRef ref (MWBase::Environment::get().getWorld()->getStore(), record->mId);
    MWWorld::Class::get (mAlchemist).getContainerStore (mAlchemist).add (ref.getPtr());
}

void MWMechanics::Alchemy::increaseSkill()
{
    MWWorld::Class::get (mAlchemist).skillUsageSucceeded (mAlchemist, ESM::Skill::Alchemy, 0);
}

float MWMechanics::Alchemy::getChance() const
{
    const CreatureStats& creatureStats = MWWorld::Class::get (mAlchemist).getCreatureStats (mAlchemist);
    const NpcStats& npcStats = MWWorld::Class::get (mAlchemist).getNpcStats (mAlchemist);
        
    return
        (npcStats.getSkill (ESM::Skill::Alchemy).getModified() +
        0.1 * creatureStats.getAttribute (1).getModified()
        + 0.1 * creatureStats.getAttribute (7).getModified());
}

int MWMechanics::Alchemy::countIngredients() const
{
    int ingredients = 0;

    for (TIngredientsIterator iter (beginIngredients()); iter!=endIngredients(); ++iter)
        if (!iter->isEmpty())
            ++ingredients;

    return ingredients;
}

void MWMechanics::Alchemy::setAlchemist (const MWWorld::Ptr& npc)
{
    mAlchemist = npc;
    
    mIngredients.resize (4);

    std::fill (mIngredients.begin(), mIngredients.end(), MWWorld::Ptr());
    
    mTools.resize (4);
    
    std::fill (mTools.begin(), mTools.end(), MWWorld::Ptr());
    
    mEffects.clear();
    
    MWWorld::ContainerStore& store = MWWorld::Class::get (npc).getContainerStore (npc);
    
    for (MWWorld::ContainerStoreIterator iter (store.begin (MWWorld::ContainerStore::Type_Apparatus));
        iter!=store.end(); ++iter)
    {    
        MWWorld::LiveCellRef<ESM::Apparatus>* ref = iter->get<ESM::Apparatus>();
    
        int type = ref->base->mData.mType;
    
        if (type<0 || type>=static_cast<int> (mTools.size()))
            throw std::runtime_error ("invalid apparatus type");
            
        if (!mTools[type].isEmpty())
            if (ref->base->mData.mQuality<=mTools[type].get<ESM::Apparatus>()->base->mData.mQuality)
                continue;
        
        mTools[type] = *iter;        
    }
}

MWMechanics::Alchemy::TToolsIterator MWMechanics::Alchemy::beginTools() const
{
    return mTools.begin();
}

MWMechanics::Alchemy::TToolsIterator MWMechanics::Alchemy::endTools() const
{
    return mTools.end();
}

MWMechanics::Alchemy::TIngredientsIterator MWMechanics::Alchemy::beginIngredients() const
{
    return mIngredients.begin();
}

MWMechanics::Alchemy::TIngredientsIterator MWMechanics::Alchemy::endIngredients() const
{
    return mIngredients.end();
}

void MWMechanics::Alchemy::clear()
{
    mAlchemist = MWWorld::Ptr();
    mTools.clear();
    mIngredients.clear();
    mEffects.clear();
}

int MWMechanics::Alchemy::addIngredient (const MWWorld::Ptr& ingredient)
{
    // find a free slot
    int slot = -1;

    for (int i=0; i<static_cast<int> (mIngredients.size()); ++i)
        if (mIngredients[i].isEmpty())
        {
            slot = i;
            break;
        }    
    
    if (slot==-1)
        return -1;
        
    for (TIngredientsIterator iter (mIngredients.begin()); iter!=mIngredients.end(); ++iter)
        if (!iter->isEmpty() && ingredient.get<ESM::Ingredient>()==iter->get<ESM::Ingredient>())
            return -1;
        
    mIngredients[slot] = ingredient;
    
    updateEffects();
        
    return slot;
}

void MWMechanics::Alchemy::removeIngredient (int index)
{
    if (index>=0 && index<static_cast<int> (mIngredients.size()))
    {
        mIngredients[index] = MWWorld::Ptr();
        updateEffects();
    }
}

MWMechanics::Alchemy::TEffectsIterator MWMechanics::Alchemy::beginEffects() const
{
    return mEffects.begin();
}

MWMechanics::Alchemy::TEffectsIterator MWMechanics::Alchemy::endEffects() const
{
    return mEffects.end();
}

std::string MWMechanics::Alchemy::getPotionName() const
{
    if (const ESM::Potion *potion = getRecord())
        return potion->mName;
        
    return "";
}

MWMechanics::Alchemy::Result MWMechanics::Alchemy::create (const std::string& name)
{
    if (mTools[ESM::Apparatus::MortarPestle].isEmpty())
        return Result_NoMortarAndPestle;
           
    if (countIngredients()<2)
        return Result_LessThanTwoIngredients;

    if (name.empty() && getPotionName().empty())
        return Result_NoName;
        
    if (beginEffects()==endEffects())
        return Result_NoEffects;

    if (getChance()<std::rand()/static_cast<double> (RAND_MAX)*100)
    {
        removeIngredients();
        return Result_RandomFailure;
    }

    addPotion (name);

    removeIngredients();
       
    increaseSkill();

    return Result_Success;
}
