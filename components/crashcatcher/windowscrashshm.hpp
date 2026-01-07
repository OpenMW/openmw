#ifndef OPENMW_COMPONENTS_CRASHCATCHER_WINDOWSCRASHSHM_HPP
#define OPENMW_COMPONENTS_CRASHCATCHER_WINDOWSCRASHSHM_HPP

#include <components/misc/windows.hpp>

namespace Crash
{

    // Used to communicate between the app and the monitor, fields are is overwritten with each event.
    static constexpr const int MAX_LONG_PATH = 0x7fff;
    static constexpr const int MAX_FILENAME = 0xff;

    struct CrashSHM
    {
        enum class Event
        {
            None,
            Startup,
            Crashed,
            Shutdown
        };

        Event mEvent;

        enum class Status
        {
            Uninitialised,
            Monitoring,
            Dumping,
            DumpedSuccessfully,
            FailedDumping
        };

        Status mMonitorStatus;

        struct Startup
        {
            HANDLE mAppProcessHandle;
            DWORD mAppMainThreadId;
            HANDLE mSignalApp;
            HANDLE mSignalMonitor;
            HANDLE mShmMutex;
            // the size defines are in UTF-16 code units, and lots of things use more UTF-8 code units, so this may
            // explode with really long non-ASCII paths
            // we can't switch to wchar_t as when we've made this struct bigger in the past, things exploded
            char mDumpDirectoryPath[MAX_LONG_PATH];
            char mCrashDumpFileName[MAX_FILENAME];
            char mFreezeDumpFileName[MAX_FILENAME];
        } mStartup;

        struct Crashed
        {
            DWORD mThreadId;
            CONTEXT mContext;
            EXCEPTION_RECORD mExceptionRecord;
        } mCrashed;
    };

} // namespace Crash

#endif
