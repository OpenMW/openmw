#include <cassert>
#include <cwchar>
#include <iostream>
#include <sstream>
#include <thread>

#include "windows_crashcatcher.hpp"
#include "windows_crashmonitor.hpp"
#include "windows_crashshm.hpp"
#include <SDL_messagebox.h>

namespace Crash
{

    HANDLE duplicateHandle(HANDLE handle)
    {
        HANDLE duplicate;
        if (!DuplicateHandle(GetCurrentProcess(), handle,
                             GetCurrentProcess(), &duplicate,
                             0, TRUE, DUPLICATE_SAME_ACCESS))
        {
            throw std::runtime_error("Crash monitor could not duplicate handle");
        }
        return duplicate;
    }

    CrashCatcher* CrashCatcher::sInstance = nullptr;

    CrashCatcher::CrashCatcher(int argc, char **argv, const std::string& crashLogPath)
    {
        assert(sInstance == nullptr); // don't allow two instances

        sInstance = this;

        HANDLE shmHandle = nullptr;
        for (int i=0; i<argc; ++i)
        {
            if (strcmp(argv[i], "--crash-monitor"))
                continue;

            if (i >= argc - 1)
                throw std::runtime_error("Crash monitor is missing the SHM handle argument");

            sscanf(argv[i + 1], "%p", &shmHandle);
            break;
        }

        if (!shmHandle)
        {
            setupIpc();
            startMonitorProcess(crashLogPath);
            installHandler();
        }
        else
        {
            CrashMonitor(shmHandle).run();
            exit(0);
        }
    }

    CrashCatcher::~CrashCatcher()
    {
        sInstance = nullptr;

        if (mShm && mSignalMonitorEvent)
        {
            shmLock();
            mShm->mEvent = CrashSHM::Event::Shutdown;
            shmUnlock();

            SetEvent(mSignalMonitorEvent);
        }

        if (mShmHandle)
            CloseHandle(mShmHandle);
    }

    void CrashCatcher::setupIpc()
    {
        SECURITY_ATTRIBUTES attributes;
        ZeroMemory(&attributes, sizeof(attributes));
        attributes.bInheritHandle = TRUE;

        mSignalAppEvent = CreateEventW(&attributes, FALSE, FALSE, NULL);
        mSignalMonitorEvent = CreateEventW(&attributes, FALSE, FALSE, NULL);

        mShmHandle = CreateFileMappingW(INVALID_HANDLE_VALUE, &attributes, PAGE_READWRITE, HIWORD(sizeof(CrashSHM)), LOWORD(sizeof(CrashSHM)), NULL);
        if (mShmHandle == nullptr)
            throw std::runtime_error("Failed to allocate crash catcher shared memory");

        mShm = reinterpret_cast<CrashSHM*>(MapViewOfFile(mShmHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(CrashSHM)));
        if (mShm == nullptr)
            throw std::runtime_error("Failed to map crash catcher shared memory");

        mShmMutex = CreateMutexW(&attributes, FALSE, NULL);
        if (mShmMutex == nullptr)
            throw std::runtime_error("Failed to create crash catcher shared memory mutex");
    }

    void CrashCatcher::shmLock()
    {
        if (WaitForSingleObject(mShmMutex, CrashCatcherTimeout) != WAIT_OBJECT_0)
            throw std::runtime_error("SHM lock timed out");
    }

    void CrashCatcher::shmUnlock()
    {
        ReleaseMutex(mShmMutex);
    }

    void CrashCatcher::waitMonitor()
    {
        if (WaitForSingleObject(mSignalAppEvent, CrashCatcherTimeout) != WAIT_OBJECT_0)
            throw std::runtime_error("Waiting for monitor failed");
    }

    void CrashCatcher::signalMonitor()
    {
        SetEvent(mSignalMonitorEvent);
    }

    void CrashCatcher::installHandler()
    {
        SetUnhandledExceptionFilter(vectoredExceptionHandler);
    }

    void CrashCatcher::startMonitorProcess(const std::string& crashLogPath)
    {
        std::wstring executablePath;
        DWORD copied = 0;
        do {
            executablePath.resize(executablePath.size() + MAX_PATH);
            copied = GetModuleFileNameW(nullptr, executablePath.data(), static_cast<DWORD>(executablePath.size()));
        } while (copied >= executablePath.size());
        executablePath.resize(copied);

        memset(mShm->mStartup.mLogFilePath, 0, sizeof(mShm->mStartup.mLogFilePath));
        size_t length = crashLogPath.length();
        if (length >= MAX_LONG_PATH) length = MAX_LONG_PATH - 1;
        strncpy(mShm->mStartup.mLogFilePath, crashLogPath.c_str(), length);
        mShm->mStartup.mLogFilePath[length] = '\0';

        // note that we don't need to lock the SHM here, the other process has not started yet
        mShm->mEvent = CrashSHM::Event::Startup;
        mShm->mStartup.mShmMutex = duplicateHandle(mShmMutex);
        mShm->mStartup.mAppProcessHandle = duplicateHandle(GetCurrentProcess());
        mShm->mStartup.mSignalApp = duplicateHandle(mSignalAppEvent);
        mShm->mStartup.mSignalMonitor = duplicateHandle(mSignalMonitorEvent);

        std::wstringstream ss;
        ss << "--crash-monitor " << std::hex << duplicateHandle(mShmHandle);
        std::wstring arguments(ss.str());

        STARTUPINFOW si;
        ZeroMemory(&si, sizeof(si));

        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(pi));

        if (!CreateProcessW(executablePath.data(), arguments.data(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
            throw std::runtime_error("Could not start crash monitor process");

        waitMonitor();
    }

    LONG CrashCatcher::vectoredExceptionHandler(PEXCEPTION_POINTERS info)
    {
        switch (info->ExceptionRecord->ExceptionCode)
        {
        case EXCEPTION_SINGLE_STEP:
        case EXCEPTION_BREAKPOINT:
        case DBG_PRINTEXCEPTION_C:
            return EXCEPTION_EXECUTE_HANDLER;
        }
        if (!sInstance)
            return EXCEPTION_EXECUTE_HANDLER;

        sInstance->handleVectoredException(info);

        _Exit(1);
    }

    void CrashCatcher::handleVectoredException(PEXCEPTION_POINTERS info)
    {
        shmLock();

        mShm->mEvent = CrashSHM::Event::Crashed;
        mShm->mCrashed.mThreadId = GetCurrentThreadId();
        mShm->mCrashed.mContext = *info->ContextRecord;
        mShm->mCrashed.mExceptionRecord = *info->ExceptionRecord;

        shmUnlock();

        signalMonitor();

        // must remain until monitor has finished
        waitMonitor();

        std::string message = "OpenMW has encountered a fatal error.\nCrash log saved to '" + std::string(mShm->mStartup.mLogFilePath) + "'.\n Please report this to https://gitlab.com/OpenMW/openmw/issues !";
        SDL_ShowSimpleMessageBox(0, "Fatal Error", message.c_str(), nullptr);
    }

} // namespace Crash
