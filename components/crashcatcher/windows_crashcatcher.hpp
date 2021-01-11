#ifndef WINDOWS_CRASHCATCHER_HPP
#define WINDOWS_CRASHCATCHER_HPP

#include <string>

#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <components/crashcatcher/crashcatcher.hpp>

namespace Crash
{

    // The implementation spawns the current executable as a monitor process which waits
    // for a global synchronization event which is sent when the parent process crashes.
    // The monitor process then extracts crash information from the parent process while
    // the parent process waits for the monitor process to finish. The crashed process
    // quits and the monitor writes the crash information to a file.
    //
    // To detect unexpected shutdowns of the application which are not handled by the
    // crash handler, the monitor periodically checks the exit code of the parent
    // process and exits if it does not return STILL_ACTIVE. You can test this by closing
    // the main openmw process in task manager.

    static constexpr const int CrashCatcherTimeout = 2500;

    struct CrashSHM;

    class CrashCatcher final
    {
    public:

        CrashCatcher(int argc, char **argv, const std::string& crashLogPath);
        ~CrashCatcher();

    private:

        static CrashCatcher* sInstance;

        //  mapped SHM area
        CrashSHM* mShm = nullptr;
        // the handle is allocated by the catcher and passed to the monitor
        // process via the command line which maps the SHM and sends / receives
        // events through it
        HANDLE mShmHandle = nullptr;
        // mutex which guards SHM area
        HANDLE mShmMutex = nullptr;

        // triggered when the monitor signals the application
        HANDLE mSignalAppEvent = INVALID_HANDLE_VALUE;

        // triggered when the application wants to wake the monitor process
        HANDLE mSignalMonitorEvent = INVALID_HANDLE_VALUE;

        void setupIpc();

        void shmLock();

        void shmUnlock();

        void startMonitorProcess(const std::string& crashLogPath);

        void waitMonitor();

        void signalMonitor();

        void installHandler();

        void handleVectoredException(PEXCEPTION_POINTERS info);

    public:

        static LONG WINAPI vectoredExceptionHandler(PEXCEPTION_POINTERS info);
    };

} // namespace Crash

#endif // WINDOWS_CRASHCATCHER_HPP
