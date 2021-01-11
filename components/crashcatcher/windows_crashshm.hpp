#ifndef WINDOWS_CRASHSHM_HPP
#define WINDOWS_CRASHSHM_HPP

#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace Crash
{

    // Used to communicate between the app and the monitor, fields are is overwritten with each event.
    static constexpr const int MAX_LONG_PATH = 0x7fff;

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

        struct Startup
        {
            HANDLE mAppProcessHandle;
            HANDLE mSignalApp;
            HANDLE mSignalMonitor;
            HANDLE mShmMutex;
            char mLogFilePath[MAX_LONG_PATH];
        } mStartup;

        struct Crashed
        {
            DWORD mThreadId;
            CONTEXT mContext;
            EXCEPTION_RECORD mExceptionRecord;
        } mCrashed;
    };

} // namespace Crash

#endif // WINDOWS_CRASHSHM_HPP
