#ifndef OPENMW_COMPONENTS_CRASHCATCHER_WINDOWSCRASHCATCHER_HPP
#define OPENMW_COMPONENTS_CRASHCATCHER_WINDOWSCRASHCATCHER_HPP

#include <filesystem>

#include <components/misc/windows.hpp>

#include "crashcatcher.hpp"

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
        static CrashCatcher* instance() { return sInstance; }

        CrashCatcher(int argc, char** argv, const std::filesystem::path& dumpPath,
            const std::filesystem::path& crashDumpName, const std::filesystem::path& freezeDumpName);
        ~CrashCatcher();

        void updateDumpPath(const std::filesystem::path& dumpPath);

        void updateDumpNames(const std::filesystem::path& crashDumpName, const std::filesystem::path& freezeDumpName);

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

        void startMonitorProcess(const std::filesystem::path& dumpPath, const std::filesystem::path& crashDumpName,
            const std::filesystem::path& freezeDumpName);

        void waitMonitor();

        void signalMonitor();

        void installHandler();

        void handleVectoredException(PEXCEPTION_POINTERS info);

    public:
        static LONG WINAPI vectoredExceptionHandler(PEXCEPTION_POINTERS info);
    };

} // namespace Crash

#endif
