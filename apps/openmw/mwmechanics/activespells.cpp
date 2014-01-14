#include "activespells.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

namespace MWMechanics
{
    void ActiveSpells::update() const
    {
        bool rebuild = false;

        MWWorld::TimeStamp now = MWBase::Environment::get().getWorld()->getTimeStamp();

        // Erase no longer active spells
        if (mLastUpdate!=now)
        {
            TContainer::iterator iter (mSpells.begin());
            while (iter!=mSpells.end())
                if (!timeToExpire (iter))
                {
                    mSpells.erase (iter++);
                    rebuild = true;
                }
                else
                    ++iter;

            mLastUpdate = now;
        }

        if (mSpellsChanged)
        {
            mSpellsChanged = false;
            rebuild = true;
        }

        if (rebuild)
            rebuildEffects();
    }

    void ActiveSpells::rebuildEffects() const
    {
        MWWorld::TimeStamp now = MWBase::Environment::get().getWorld()->getTimeStamp();
    
        mEffects = MagicEffects();

        for (TIterator iter (begin()); iter!=end(); ++iter)
        {
            const MWWorld::TimeStamp& start = iter->second.mTimeStamp;

            const std::vector<Effect>& effects = iter->second.mEffects;

            for (std::vector<Effect>::const_iterator effectIt = effects.begin(); effectIt != effects.end(); ++effectIt)
            {
                int duration = effectIt->mDuration;
                MWWorld::TimeStamp end = start;
                end += static_cast<double> (duration)*
                    MWBase::Environment::get().getWorld()->getTimeScaleFactor()/(60*60);

                if (end>now)
                    mEffects.add(effectIt->mKey, MWMechanics::EffectParam(effectIt->mMagnitude));
            }
        }    
    }

    ActiveSpells::ActiveSpells()
        : mSpellsChanged (false)
        , mLastUpdate (MWBase::Environment::get().getWorld()->getTimeStamp())
    {}

    const MagicEffects& ActiveSpells::getMagicEffects() const
    {
        update();
        return mEffects;
    }

    ActiveSpells::TIterator ActiveSpells::begin() const
    {
        return mSpells.begin();
    }

    ActiveSpells::TIterator ActiveSpells::end() const
    {
        return mSpells.end();
    }

    double ActiveSpells::timeToExpire (const TIterator& iterator) const
    {
        const std::vector<Effect>& effects = iterator->second.mEffects;

        int duration = 0;

        for (std::vector<Effect>::const_iterator iter (effects.begin());
            iter!=effects.end(); ++iter)
        {
            if (iter->mDuration > duration)
                duration = iter->mDuration;
        }

        double scaledDuration = duration *
              MWBase::Environment::get().getWorld()->getTimeScaleFactor()/(60*60);

        double usedUp = MWBase::Environment::get().getWorld()->getTimeStamp() - iterator->second.mTimeStamp;

        if (usedUp>=scaledDuration)
            return 0;

        return scaledDuration-usedUp;
    }

    bool ActiveSpells::isSpellActive(std::string id) const
    {
        Misc::StringUtils::toLower(id);
        for (TContainer::iterator iter = mSpells.begin(); iter != mSpells.end(); ++iter)
        {
            std::string left = iter->first;
            Misc::StringUtils::toLower(left);

            if (iter->first == id)
                return true;
        }
        return false;
    }

    const ActiveSpells::TContainer& ActiveSpells::getActiveSpells() const
    {
        return mSpells;
    }

    void ActiveSpells::addSpell(const std::string &id, bool stack, std::vector<Effect> effects,
                                const std::string &displayName, const std::string& casterHandle)
    {
        bool exists = false;
        for (TContainer::const_iterator it = begin(); it != end(); ++it)
        {
            if (id == it->first)
                exists = true;
        }

        ActiveSpellParams params;
        params.mTimeStamp = MWBase::Environment::get().getWorld()->getTimeStamp();
        params.mEffects = effects;
        params.mDisplayName = displayName;
        params.mCasterHandle = casterHandle;

        if (!exists || stack)
            mSpells.insert (std::make_pair(id, params));
        else
            mSpells.find(id)->second = params;

        mSpellsChanged = true;
    }

    void ActiveSpells::removeEffects(const std::string &id)
    {
        mSpells.erase(Misc::StringUtils::lowerCase(id));
        mSpellsChanged = true;
    }

    void ActiveSpells::visitEffectSources(EffectSourceVisitor &visitor) const
    {
        for (TContainer::const_iterator it = begin(); it != end(); ++it)
        {
            float timeScale = MWBase::Environment::get().getWorld()->getTimeScaleFactor();

            for (std::vector<Effect>::const_iterator effectIt = it->second.mEffects.begin();
                 effectIt != it->second.mEffects.end(); ++effectIt)
            {
                std::string name = it->second.mDisplayName;

                float remainingTime = effectIt->mDuration +
                        (it->second.mTimeStamp - MWBase::Environment::get().getWorld()->getTimeStamp())*3600/timeScale;
                float magnitude = effectIt->mMagnitude;

                if (magnitude)
                    visitor.visit(effectIt->mKey, name, it->second.mCasterHandle, magnitude, remainingTime);
            }
        }
    }

    void ActiveSpells::purgeAll(float chance)
    {
        for (TContainer::iterator it = mSpells.begin(); it != mSpells.end(); )
        {
            int roll = std::rand()/ (static_cast<double> (RAND_MAX) + 1) * 100; // [0, 99]
            if (roll < chance)
                mSpells.erase(it++);
            else
                ++it;
        }
        mSpellsChanged = true;
    }

    void ActiveSpells::purgeEffect(short effectId)
    {
        for (TContainer::iterator it = mSpells.begin(); it != mSpells.end(); ++it)
        {
            for (std::vector<Effect>::iterator effectIt = it->second.mEffects.begin();
                 effectIt != it->second.mEffects.end();)
            {
                if (effectIt->mKey.mId == effectId)
                    effectIt = it->second.mEffects.erase(effectIt);
                else
                    effectIt++;
            }
        }
        mSpellsChanged = true;
    }

    void ActiveSpells::purge(const std::string &actorHandle)
    {
        for (TContainer::iterator it = mSpells.begin(); it != mSpells.end(); ++it)
        {
            for (std::vector<Effect>::iterator effectIt = it->second.mEffects.begin();
                 effectIt != it->second.mEffects.end();)
            {
                const ESM::MagicEffect* effect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(effectIt->mKey.mId);
                if (effect->mData.mFlags & ESM::MagicEffect::CasterLinked
                        && it->second.mCasterHandle == actorHandle)
                    effectIt = it->second.mEffects.erase(effectIt);
                else
                    effectIt++;
            }
        }
        mSpellsChanged = true;
    }
}
