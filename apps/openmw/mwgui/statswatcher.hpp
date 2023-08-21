#ifndef MWGUI_STATSWATCHER_H
#define MWGUI_STATSWATCHER_H

#include <map>
#include <set>

#include <components/esm/attr.hpp>
#include <components/esm3/loadskil.hpp>

#include "../mwmechanics/stat.hpp"

#include "../mwworld/ptr.hpp"

namespace MWGui
{
    class StatsListener
    {
    public:
        virtual ~StatsListener() = default;

        /// Set value for the given ID.
        virtual void setAttribute(ESM::RefId id, const MWMechanics::AttributeValue& value) {}
        virtual void setValue(std::string_view id, const MWMechanics::DynamicStat<float>& value) {}
        virtual void setValue(std::string_view, const std::string& value) {}
        virtual void setValue(std::string_view, int value) {}
        virtual void setValue(ESM::RefId id, const MWMechanics::SkillValue& value) {}
        virtual void configureSkills(const std::vector<ESM::RefId>& major, const std::vector<ESM::RefId>& minor) {}
    };

    class StatsWatcher
    {
        MWWorld::Ptr mWatched;

        std::map<ESM::RefId, MWMechanics::AttributeValue> mWatchedAttributes;
        std::map<ESM::RefId, MWMechanics::SkillValue> mWatchedSkills;

        MWMechanics::DynamicStat<float> mWatchedHealth;
        MWMechanics::DynamicStat<float> mWatchedMagicka;
        MWMechanics::DynamicStat<float> mWatchedFatigue;

        std::string mWatchedName;
        ESM::RefId mWatchedRace;
        ESM::RefId mWatchedClass;

        int mWatchedLevel;

        float mWatchedTimeToStartDrowning;

        bool mWatchedStatsEmpty;

        std::set<StatsListener*> mListeners;

        void setAttribute(ESM::RefId id, const MWMechanics::AttributeValue& value);
        void setValue(std::string_view id, const MWMechanics::DynamicStat<float>& value);
        void setValue(std::string_view id, const std::string& value);
        void setValue(std::string_view id, int value);
        void setValue(ESM::RefId id, const MWMechanics::SkillValue& value);
        void configureSkills(const std::vector<ESM::RefId>& major, const std::vector<ESM::RefId>& minor);

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
