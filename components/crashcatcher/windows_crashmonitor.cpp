#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>

#include <DbgHelp.h>

#include <iostream>
#include <memory>
#include <sstream>

#include "windows_crashcatcher.hpp"
#include "windows_crashmonitor.hpp"
#include "windows_crashshm.hpp"
#include <components/debug/debuglog.hpp>

namespace Crash
{

    CrashMonitor::CrashMonitor(HANDLE shmHandle)
        : mShmHandle(shmHandle)
    {
        mShm = reinterpret_cast<CrashSHM*>(MapViewOfFile(mShmHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(CrashSHM)));
        if (mShm == nullptr)
            throw std::runtime_error("Failed to map crash monitor shared memory");

        // accessing SHM without lock is OK here, the parent waits for a signal before continuing

        mShmMutex = mShm->mStartup.mShmMutex;
        mAppProcessHandle = mShm->mStartup.mAppProcessHandle;
        mSignalAppEvent = mShm->mStartup.mSignalApp;
        mSignalMonitorEvent = mShm->mStartup.mSignalMonitor;
    }

    CrashMonitor::~CrashMonitor()
    {
        if (mShm)
            UnmapViewOfFile(mShm);

        // the handles received from the app are duplicates, we must close them

        if (mShmHandle)
            CloseHandle(mShmHandle);

        if (mShmMutex)
            CloseHandle(mShmMutex);

        if (mSignalAppEvent)
            CloseHandle(mSignalAppEvent);

        if (mSignalMonitorEvent)
            CloseHandle(mSignalMonitorEvent);
    }

    void CrashMonitor::shmLock()
    {
        if (WaitForSingleObject(mShmMutex, CrashCatcherTimeout) != WAIT_OBJECT_0)
            throw std::runtime_error("SHM monitor lock timed out");
    }

    void CrashMonitor::shmUnlock()
    {
        ReleaseMutex(mShmMutex);
    }

    void CrashMonitor::signalApp() const
    {
        SetEvent(mSignalAppEvent);
    }

    bool CrashMonitor::waitApp() const
    {
        return WaitForSingleObject(mSignalMonitorEvent, CrashCatcherTimeout) == WAIT_OBJECT_0;
    }

    bool CrashMonitor::isAppAlive() const
    {
        DWORD code = 0;
        GetExitCodeProcess(mAppProcessHandle, &code);
        return code == STILL_ACTIVE;
    }

    void CrashMonitor::run()
    {
        try
        {
            // app waits for monitor start up, let it continue
            signalApp();

            bool running = true;
            while (isAppAlive() && running)
            {
                if (waitApp())
                {
                    shmLock();

                    switch (mShm->mEvent)
                    {
                    case CrashSHM::Event::None:
                        break;
                    case CrashSHM::Event::Crashed:
                        handleCrash();
                        running = false;
                        break;
                    case CrashSHM::Event::Shutdown:
                        running = false;
                        break;
                    case CrashSHM::Event::Startup:
                        break;
                    }

                    shmUnlock();
                }
            }

        } 
        catch (...)
        {
            Log(Debug::Error) << "Exception in crash monitor, exiting";
        }
        signalApp();
    }

    std::wstring utf8ToUtf16(const std::string& utf8)
    {
        const int nLenWide = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), utf8.size(), nullptr, 0);

        std::wstring utf16;
        utf16.resize(nLenWide);
        if (MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), utf8.size(), utf16.data(), nLenWide) != nLenWide)
            return {};

        return utf16;
    }

    void CrashMonitor::handleCrash()
    {
        DWORD processId = GetProcessId(mAppProcessHandle);

        try
        {
            HMODULE dbghelp = LoadLibraryA("dbghelp.dll");
            if (dbghelp == NULL)
                return;
    
            using MiniDumpWirteDumpFn = BOOL (WINAPI*)(
                HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType, PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
                PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, PMINIDUMP_CALLBACK_INFORMATION CallbackParam
            );
    
            MiniDumpWirteDumpFn miniDumpWriteDump = (MiniDumpWirteDumpFn)GetProcAddress(dbghelp, "MiniDumpWriteDump");
            if (miniDumpWriteDump == NULL)
                return;

            std::wstring utf16Path = utf8ToUtf16(mShm->mStartup.mLogFilePath);
            if (utf16Path.empty())
                return;

            if (utf16Path.length() > MAX_PATH)
                utf16Path = LR"(\\?\)" + utf16Path;

            HANDLE hCrashLog = CreateFileW(utf16Path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hCrashLog == NULL || hCrashLog == INVALID_HANDLE_VALUE)
                return;
            if (auto err = GetLastError(); err != ERROR_ALREADY_EXISTS && err != 0)
                return;

            EXCEPTION_POINTERS exp;
            exp.ContextRecord = &mShm->mCrashed.mContext;
            exp.ExceptionRecord = &mShm->mCrashed.mExceptionRecord;
            MINIDUMP_EXCEPTION_INFORMATION infos = {};
            infos.ThreadId = mShm->mCrashed.mThreadId;
            infos.ExceptionPointers = &exp;
            infos.ClientPointers = FALSE;
            MINIDUMP_TYPE type = (MINIDUMP_TYPE)(MiniDumpWithDataSegs | MiniDumpWithHandleData);
            miniDumpWriteDump(mAppProcessHandle, processId, hCrashLog, type, &infos, 0, 0);
        }
        catch (const std::exception&e)
        {
            Log(Debug::Error) << "CrashMonitor: " << e.what();
        }
        catch (...)
        {
            Log(Debug::Error) << "CrashMonitor: unknown exception";
        }
    }

} // namespace Crash
