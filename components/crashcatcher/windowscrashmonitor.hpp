#ifndef OPENMW_COMPONENTS_CRASHCATCHER_WINDOWSCRASHMONITOR_HPP
#define OPENMW_COMPONENTS_CRASHCATCHER_WINDOWSCRASHMONITOR_HPP

#include <components/misc/windows.hpp>

#include <atomic>
#include <unordered_map>

namespace Crash
{

    struct CrashSHM;

    class CrashMonitor final
    {
    public:
        CrashMonitor(HANDLE shmHandle);

        ~CrashMonitor();

        void run();

    private:
        HANDLE mAppProcessHandle = nullptr;
        DWORD mAppMainThreadId = 0;
        HWND mAppWindowHandle = nullptr;

        // triggered when the monitor process wants to wake the parent process (received via SHM)
        HANDLE mSignalAppEvent = nullptr;
        // triggered when the application wants to wake the monitor process (received via SHM)
        HANDLE mSignalMonitorEvent = nullptr;

        CrashSHM* mShm = nullptr;
        HANDLE mShmHandle = nullptr;
        HANDLE mShmMutex = nullptr;

        DWORD mFreezeMessageBoxThreadId = 0;
        std::atomic_bool mFreezeAbort;

        static std::unordered_map<HWINEVENTHOOK, CrashMonitor*> smEventHookOwners;

        void signalApp() const;

        bool waitApp() const;

        bool isAppAlive() const;

        bool isAppFrozen();

        void shmLock();

        void shmUnlock();

        void handleCrash(bool isFreeze);

        void showFreezeMessageBox();

        void hideFreezeMessageBox();
    };

} // namespace Crash

#endif
