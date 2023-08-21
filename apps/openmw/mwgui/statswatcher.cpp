#include "statswatcher.hpp"

#include <components/esm3/loadclas.hpp>
#include <components/esm3/loadrace.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include <string>

namespace MWGui
{
    // mWatchedTimeToStartDrowning = -1 for correct drowning state check,
    // if stats.getTimeToStartDrowning() == 0 already on game start
    StatsWatcher::StatsWatcher()
        : mWatchedLevel(-1)
        , mWatchedTimeToStartDrowning(-1)
        , mWatchedStatsEmpty(true)
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

        const auto& store = MWBase::Environment::get().getESMStore();
        MWBase::WindowManager* winMgr = MWBase::Environment::get().getWindowManager();
        const MWMechanics::NpcStats& stats = mWatched.getClass().getNpcStats(mWatched);
        for (const ESM::Attribute& attribute : store->get<ESM::Attribute>())
        {
            const auto& value = stats.getAttribute(attribute.mId);
            if (value != mWatchedAttributes[attribute.mId] || mWatchedStatsEmpty)
            {
                mWatchedAttributes[attribute.mId] = value;
                setAttribute(attribute.mId, value);
            }
        }

        if (stats.getHealth() != mWatchedHealth || mWatchedStatsEmpty)
        {
            mWatchedHealth = stats.getHealth();
            setValue("HBar", stats.getHealth());
        }
        if (stats.getMagicka() != mWatchedMagicka || mWatchedStatsEmpty)
        {
            mWatchedMagicka = stats.getMagicka();
            setValue("MBar", stats.getMagicka());
        }
        if (stats.getFatigue() != mWatchedFatigue || mWatchedStatsEmpty)
        {
            mWatchedFatigue = stats.getFatigue();
            setValue("FBar", stats.getFatigue());
        }

        float timeToDrown = stats.getTimeToStartDrowning();

        if (timeToDrown != mWatchedTimeToStartDrowning)
        {
            static const float fHoldBreathTime = MWBase::Environment::get()
                                                     .getESMStore()
                                                     ->get<ESM::GameSetting>()
                                                     .find("fHoldBreathTime")
                                                     ->mValue.getFloat();

            mWatchedTimeToStartDrowning = timeToDrown;

            if (timeToDrown >= fHoldBreathTime || timeToDrown == -1.0) // -1.0 is a special value during initialization
                winMgr->setDrowningBarVisibility(false);
            else
            {
                winMgr->setDrowningBarVisibility(true);
                winMgr->setDrowningTimeLeft(stats.getTimeToStartDrowning(), fHoldBreathTime);
            }
        }

        for (const ESM::Skill& skill : store->get<ESM::Skill>())
        {
            const auto& value = stats.getSkill(skill.mId);
            if (value != mWatchedSkills[skill.mId] || mWatchedStatsEmpty)
            {
                mWatchedSkills[skill.mId] = value;
                setValue(skill.mId, value);
            }
        }

        if (stats.getLevel() != mWatchedLevel || mWatchedStatsEmpty)
        {
            mWatchedLevel = stats.getLevel();
            setValue("level", mWatchedLevel);
        }

        if (mWatched.getClass().isNpc())
        {
            const ESM::NPC* watchedRecord = mWatched.get<ESM::NPC>()->mBase;

            if (watchedRecord->mName != mWatchedName || mWatchedStatsEmpty)
            {
                mWatchedName = watchedRecord->mName;
                setValue("name", watchedRecord->mName);
            }

            if (watchedRecord->mRace != mWatchedRace || mWatchedStatsEmpty)
            {
                mWatchedRace = watchedRecord->mRace;
                const ESM::Race* race = store->get<ESM::Race>().find(watchedRecord->mRace);
                setValue("race", race->mName);
            }

            if (watchedRecord->mClass != mWatchedClass || mWatchedStatsEmpty)
            {
                mWatchedClass = watchedRecord->mClass;
                const ESM::Class* cls = store->get<ESM::Class>().find(watchedRecord->mClass);
                setValue("class", cls->mName);

                size_t size = cls->mData.mSkills.size();
                std::vector<ESM::RefId> majorSkills(size);
                std::vector<ESM::RefId> minorSkills(size);

                for (size_t i = 0; i < size; ++i)
                {
                    minorSkills[i] = ESM::Skill::indexToRefId(cls->mData.mSkills[i][0]);
                    majorSkills[i] = ESM::Skill::indexToRefId(cls->mData.mSkills[i][1]);
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

    void StatsWatcher::setAttribute(ESM::RefId id, const MWMechanics::AttributeValue& value)
    {
        for (StatsListener* listener : mListeners)
            listener->setAttribute(id, value);
    }

    void StatsWatcher::setValue(ESM::RefId id, const MWMechanics::SkillValue& value)
    {
        for (StatsListener* listener : mListeners)
            listener->setValue(id, value);
    }

    void StatsWatcher::setValue(std::string_view id, const MWMechanics::DynamicStat<float>& value)
    {
        for (StatsListener* listener : mListeners)
            listener->setValue(id, value);
    }

    void StatsWatcher::setValue(std::string_view id, const std::string& value)
    {
        for (StatsListener* listener : mListeners)
            listener->setValue(id, value);
    }

    void StatsWatcher::setValue(std::string_view id, int value)
    {
        for (StatsListener* listener : mListeners)
            listener->setValue(id, value);
    }

    void StatsWatcher::configureSkills(const std::vector<ESM::RefId>& major, const std::vector<ESM::RefId>& minor)
    {
        for (StatsListener* listener : mListeners)
            listener->configureSkills(major, minor);
    }
}
