#ifndef OPENMW_PROFILE_H
#define OPENMW_PROFILE_H

#include <osg/Stats>
#include <osg/Timer>

#include <cstddef>
#include <string>

namespace OMW
{
    struct UserStats
    {
        const std::string mLabel;
        const std::string mBegin;
        const std::string mEnd;
        const std::string mTaken;

        explicit UserStats(const std::string& label, const std::string& prefix)
            : mLabel(label)
            , mBegin(prefix + "_time_begin")
            , mEnd(prefix + "_time_end")
            , mTaken(prefix + "_time_taken")
        {
        }
    };

    enum class UserStatsType : std::size_t
    {
        Input,
        Sound,
        LuaSyncUpdate,
        State,
        Script,
        Mechanics,
        Physics,
        PhysicsWorker,
        World,
        Gui,
        WindowManager,
        Lua,
        Number,
    };

    template <UserStatsType type>
    struct UserStatsValue
    {
        static const UserStats sValue;
    };

    template <>
    inline const UserStats UserStatsValue<UserStatsType::Input>::sValue{ "Input", "input" };

    template <>
    inline const UserStats UserStatsValue<UserStatsType::Sound>::sValue{ "Sound", "sound" };

    template <>
    inline const UserStats UserStatsValue<UserStatsType::State>::sValue{ "State", "state" };

    template <>
    inline const UserStats UserStatsValue<UserStatsType::Script>::sValue{ "Script", "script" };

    template <>
    inline const UserStats UserStatsValue<UserStatsType::Mechanics>::sValue{ "Mech", "mechanics" };

    template <>
    inline const UserStats UserStatsValue<UserStatsType::Physics>::sValue{ "Phys", "physics" };

    template <>
    inline const UserStats UserStatsValue<UserStatsType::PhysicsWorker>::sValue{ " -Async", "physicsworker" };

    template <>
    inline const UserStats UserStatsValue<UserStatsType::World>::sValue{ "World", "world" };

    template <>
    inline const UserStats UserStatsValue<UserStatsType::Gui>::sValue{ "Gui", "gui" };

    template <>
    inline const UserStats UserStatsValue<UserStatsType::Lua>::sValue{ "Lua", "lua" };

    template <>
    inline const UserStats UserStatsValue<UserStatsType::LuaSyncUpdate>::sValue{ "LuaSync", "luasyncupdate" };

    template <>
    inline const UserStats UserStatsValue<UserStatsType::WindowManager>::sValue{ "WindowManager", "windowmanager" };

    template <UserStatsType type>
    struct ForEachUserStatsValue
    {
        template <class F>
        static void apply(F&& f)
        {
            f(UserStatsValue<type>::sValue);
            using Next = ForEachUserStatsValue<static_cast<UserStatsType>(static_cast<std::size_t>(type) + 1)>;
            Next::apply(std::forward<F>(f));
        }
    };

    template <>
    struct ForEachUserStatsValue<UserStatsType::Number>
    {
        template <class F>
        static void apply(F&&)
        {
        }
    };

    template <class F>
    void forEachUserStatsValue(F&& f)
    {
        ForEachUserStatsValue<static_cast<UserStatsType>(0)>::apply(std::forward<F>(f));
    }

    template <UserStatsType type>
    class ScopedProfile
    {
    public:
        explicit ScopedProfile(
            osg::Timer_t frameStart, unsigned int frameNumber, const osg::Timer& timer, osg::Stats& stats)
            : mScopeStart(timer.tick())
            , mFrameStart(frameStart)
            , mFrameNumber(frameNumber)
            , mTimer(timer)
            , mStats(stats)
        {
        }

        ScopedProfile(const ScopedProfile&) = delete;
        ScopedProfile& operator=(const ScopedProfile&) = delete;

        ~ScopedProfile()
        {
            if (!mStats.collectStats("engine"))
                return;

            const osg::Timer_t end = mTimer.tick();
            const UserStats& stats = UserStatsValue<type>::sValue;

            mStats.setAttribute(mFrameNumber, stats.mBegin, mTimer.delta_s(mFrameStart, mScopeStart));
            mStats.setAttribute(mFrameNumber, stats.mTaken, mTimer.delta_s(mScopeStart, end));
            mStats.setAttribute(mFrameNumber, stats.mEnd, mTimer.delta_s(mFrameStart, end));
        }

    private:
        const osg::Timer_t mScopeStart;
        const osg::Timer_t mFrameStart;
        const unsigned int mFrameNumber;
        const osg::Timer& mTimer;
        osg::Stats& mStats;
    };
}

#endif
