#ifndef MWGUI_STATSWATCHER_H
#define MWGUI_STATSWATCHER_H

#include <set>

#include <components/esm/attr.hpp>
#include <components/esm/loadskil.hpp>

#include "../mwmechanics/stat.hpp"

#include "../mwworld/ptr.hpp"

namespace MWGui
{
    class StatsListener
    {
    public:
        /// Set value for the given ID.
        virtual void setValue(const std::string& id, const MWMechanics::AttributeValue& value) {}
        virtual void setValue(const std::string& id, const MWMechanics::DynamicStat<float>& value) {}
        virtual void setValue(const std::string& id, const std::string& value) {}
        virtual void setValue(const std::string& id, int value) {}
        virtual void setValue(const ESM::Skill::SkillEnum parSkill, const MWMechanics::SkillValue& value) {}
        virtual void configureSkills(const std::vector<int>& major, const std::vector<int>& minor) {}
    };

    class StatsWatcher
    {
        MWWorld::Ptr mWatched;

        MWMechanics::AttributeValue mWatchedAttributes[ESM::Attribute::Length];
        MWMechanics::SkillValue mWatchedSkills[ESM::Skill::Length];

        MWMechanics::DynamicStat<float> mWatchedHealth;
        MWMechanics::DynamicStat<float> mWatchedMagicka;
        MWMechanics::DynamicStat<float> mWatchedFatigue;

        std::string mWatchedName;
        std::string mWatchedRace;
        std::string mWatchedClass;

        int mWatchedLevel;

        float mWatchedTimeToStartDrowning;

        bool mWatchedStatsEmpty;

        std::set<StatsListener*> mListeners;

        void setValue(const std::string& id, const MWMechanics::AttributeValue& value);
        void setValue(const std::string& id, const MWMechanics::DynamicStat<float>& value);
        void setValue(const std::string& id, const std::string& value);
        void setValue(const std::string& id, int value);
        void setValue(const ESM::Skill::SkillEnum parSkill, const MWMechanics::SkillValue& value);
        void configureSkills(const std::vector<int>& major, const std::vector<int>& minor);

    public:
        StatsWatcher();

        void update();
        void addListener(StatsListener* listener);
        void removeListener(StatsListener* listener);

        void watchActor(const MWWorld::Ptr& ptr);
        MWWorld::Ptr getWatchedActor() const { return mWatched; }

        void forceUpdate() { mWatchedStatsEmpty = true; }
    };
}

#endif
