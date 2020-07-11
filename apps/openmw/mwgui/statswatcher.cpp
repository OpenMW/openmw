#include "statswatcher.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/spellutil.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"

namespace MWGui
{
    // mWatchedTimeToStartDrowning = -1 for correct drowning state check,
    // if stats.getTimeToStartDrowning() == 0 already on game start
    StatsWatcher::StatsWatcher()
      : mWatchedLevel(-1), mWatchedTimeToStartDrowning(-1), mWatchedStatsEmpty(true)
    {
    }

    void StatsWatcher::watchActor(const MWWorld::Ptr& ptr)
    {
        mWatched = ptr;
    }

    void StatsWatcher::update()
    {
        if (mWatched.isEmpty())
            return;

        MWBase::WindowManager *winMgr = MWBase::Environment::get().getWindowManager();
        const MWMechanics::NpcStats &stats = mWatched.getClass().getNpcStats(mWatched);
        for (int i = 0;i < ESM::Attribute::Length;++i)
        {
            if (stats.getAttribute(i) != mWatchedAttributes[i] || mWatchedStatsEmpty)
            {
                std::stringstream attrname;
                attrname << "AttribVal"<<(i+1);

                mWatchedAttributes[i] = stats.getAttribute(i);
                setValue(attrname.str(), stats.getAttribute(i));
            }
        }

        if (stats.getHealth() != mWatchedHealth || mWatchedStatsEmpty)
        {
            static const std::string hbar("HBar");
            mWatchedHealth = stats.getHealth();
            setValue(hbar, stats.getHealth());
        }
        if (stats.getMagicka() != mWatchedMagicka || mWatchedStatsEmpty)
        {
            static const std::string mbar("MBar");
            mWatchedMagicka = stats.getMagicka();
            setValue(mbar, stats.getMagicka());
        }
        if (stats.getFatigue() != mWatchedFatigue || mWatchedStatsEmpty)
        {
            static const std::string fbar("FBar");
            mWatchedFatigue = stats.getFatigue();
            setValue(fbar, stats.getFatigue());
        }

        float timeToDrown = stats.getTimeToStartDrowning();

        if (timeToDrown != mWatchedTimeToStartDrowning)
        {
            static const float fHoldBreathTime = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                    .find("fHoldBreathTime")->mValue.getFloat();

            mWatchedTimeToStartDrowning = timeToDrown;

            if(timeToDrown >= fHoldBreathTime || timeToDrown == -1.0) // -1.0 is a special value during initialization
                winMgr->setDrowningBarVisibility(false);
            else
            {
                winMgr->setDrowningBarVisibility(true);
                winMgr->setDrowningTimeLeft(stats.getTimeToStartDrowning(), fHoldBreathTime);
            }
        }

        //Loop over ESM::Skill::SkillEnum
        for (int i = 0; i < ESM::Skill::Length; ++i)
        {
            if(stats.getSkill(i) != mWatchedSkills[i] || mWatchedStatsEmpty)
            {
                mWatchedSkills[i] = stats.getSkill(i);
                setValue((ESM::Skill::SkillEnum)i, stats.getSkill(i));
            }
        }

        if (stats.getLevel() != mWatchedLevel || mWatchedStatsEmpty)
        {
            mWatchedLevel = stats.getLevel();
            setValue("level", mWatchedLevel);
        }

        if (mWatched.getClass().isNpc())
        {
            const ESM::NPC *watchedRecord = mWatched.get<ESM::NPC>()->mBase;

            if (watchedRecord->mName != mWatchedName || mWatchedStatsEmpty)
            {
                mWatchedName = watchedRecord->mName;
                setValue("name", watchedRecord->mName);
            }

            if (watchedRecord->mRace != mWatchedRace || mWatchedStatsEmpty)
            {
                mWatchedRace = watchedRecord->mRace;
                const ESM::Race *race = MWBase::Environment::get().getWorld()->getStore()
                    .get<ESM::Race>().find(watchedRecord->mRace);
                setValue("race", race->mName);
            }

            if (watchedRecord->mClass != mWatchedClass || mWatchedStatsEmpty)
            {
                mWatchedClass = watchedRecord->mClass;
                const ESM::Class *cls = MWBase::Environment::get().getWorld()->getStore()
                    .get<ESM::Class>().find(watchedRecord->mClass);
                setValue("class", cls->mName);

                MWBase::WindowManager::SkillList majorSkills (5);
                MWBase::WindowManager::SkillList minorSkills (5);

                for (int i=0; i<5; ++i)
                {
                    minorSkills[i] = cls->mData.mSkills[i][0];
                    majorSkills[i] = cls->mData.mSkills[i][1];
                }

                configureSkills(majorSkills, minorSkills);
            }
        }

        mWatchedStatsEmpty = false;
    }

    void StatsWatcher::addListener(StatsListener* listener)
    {
        mListeners.insert(listener);
    }

    void StatsWatcher::removeListener(StatsListener* listener)
    {
        mListeners.erase(listener);
    }

    void StatsWatcher::setValue(const std::string& id, const MWMechanics::AttributeValue& value)
    {
        for (StatsListener* listener : mListeners)
            listener->setValue(id, value);
    }

    void StatsWatcher::setValue(ESM::Skill::SkillEnum parSkill, const MWMechanics::SkillValue& value)
    {
        /// \todo Don't use the skill enum as a parameter type (we will have to drop it anyway, once we
        /// allow custom skills.
        for (StatsListener* listener : mListeners)
            listener->setValue(parSkill, value);
    }

    void StatsWatcher::setValue(const std::string& id, const MWMechanics::DynamicStat<float>& value)
    {
        for (StatsListener* listener : mListeners)
            listener->setValue(id, value);
    }

    void StatsWatcher::setValue(const std::string& id, const std::string& value)
    {
        for (StatsListener* listener : mListeners)
            listener->setValue(id, value);
    }

    void StatsWatcher::setValue(const std::string& id, int value)
    {
        for (StatsListener* listener : mListeners)
            listener->setValue(id, value);
    }

    void StatsWatcher::configureSkills(const std::vector<int>& major, const std::vector<int>& minor)
    {
        for (StatsListener* listener : mListeners)
            listener->configureSkills(major, minor);
    }
}
