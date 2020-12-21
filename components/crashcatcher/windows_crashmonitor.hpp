#ifndef WINDOWS_CRASHMONITOR_HPP
#define WINDOWS_CRASHMONITOR_HPP

#include <windef.h>

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

    // triggered when the monitor process wants to wake the parent process (received via SHM)
    HANDLE mSignalAppEvent = nullptr;
    // triggered when the application wants to wake the monitor process (received via SHM)
    HANDLE mSignalMonitorEvent = nullptr;

    CrashSHM* mShm = nullptr;
    HANDLE mShmHandle = nullptr;
    HANDLE mShmMutex = nullptr;

    void signalApp() const;

    bool waitApp() const;

    bool isAppAlive() const;

    void shmLock();

    void shmUnlock();

    void handleCrash();
};

} // namespace Crash

#endif // WINDOWS_CRASHMONITOR_HPP
