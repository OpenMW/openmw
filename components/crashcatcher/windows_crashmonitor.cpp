#include "windows_crashmonitor.hpp"

#include <Psapi.h>
#include <components/windows.hpp>

#include <DbgHelp.h>

#include <memory>
#include <thread>

#include <SDL_messagebox.h>

#include "windows_crashcatcher.hpp"
#include "windows_crashshm.hpp"
#include <components/debug/debuglog.hpp>

namespace Crash
{
    std::unordered_map<HWINEVENTHOOK, CrashMonitor*> CrashMonitor::smEventHookOwners{};

    using IsHungAppWindowFn = BOOL(WINAPI*)(HWND hwnd);

    // Obtains the pointer to user32.IsHungAppWindow, this function may be removed in the future.
    // See: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-ishungappwindow
    static IsHungAppWindowFn getIsHungAppWindow() noexcept
    {
        auto user32Handle = LoadLibraryA("user32.dll");
        if (user32Handle == nullptr)
            return nullptr;

        return reinterpret_cast<IsHungAppWindowFn>(GetProcAddress(user32Handle, "IsHungAppWindow"));
    }

    static const IsHungAppWindowFn sIsHungAppWindow = getIsHungAppWindow();

    CrashMonitor::CrashMonitor(HANDLE shmHandle)
        : mShmHandle(shmHandle)
    {
        mShm = reinterpret_cast<CrashSHM*>(MapViewOfFile(mShmHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(CrashSHM)));
        if (mShm == nullptr)
            throw std::runtime_error("Failed to map crash monitor shared memory");

        // accessing SHM without lock is OK here, the parent waits for a signal before continuing

        mShmMutex = mShm->mStartup.mShmMutex;
        mAppProcessHandle = mShm->mStartup.mAppProcessHandle;
        mAppMainThreadId = mShm->mStartup.mAppMainThreadId;
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

    bool CrashMonitor::isAppFrozen()
    {
        MSG message;
        // Allow the event hook callback to run
        PeekMessage(&message, nullptr, 0, 0, PM_NOREMOVE);

        if (!mAppWindowHandle)
        {
            EnumWindows(
                [](HWND handle, LPARAM param) -> BOOL {
                    CrashMonitor& crashMonitor = *(CrashMonitor*)param;
                    DWORD processId;
                    if (GetWindowThreadProcessId(handle, &processId) == crashMonitor.mAppMainThreadId
                        && processId == GetProcessId(crashMonitor.mAppProcessHandle))
                    {
                        if (GetWindow(handle, GW_OWNER) == 0)
                        {
                            crashMonitor.mAppWindowHandle = handle;
                            return false;
                        }
                    }
                    return true;
                },
                (LPARAM)this);
            if (mAppWindowHandle)
            {
                DWORD processId;
                GetWindowThreadProcessId(mAppWindowHandle, &processId);
                HWINEVENTHOOK eventHookHandle = SetWinEventHook(
                    EVENT_OBJECT_DESTROY, EVENT_OBJECT_DESTROY, nullptr,
                    [](HWINEVENTHOOK hWinEventHook, DWORD event, HWND windowHandle, LONG objectId, LONG childId,
                        DWORD eventThread, DWORD eventTime) {
                        CrashMonitor& crashMonitor = *smEventHookOwners[hWinEventHook];
                        if (event == EVENT_OBJECT_DESTROY && windowHandle == crashMonitor.mAppWindowHandle
                            && objectId == OBJID_WINDOW && childId == INDEXID_CONTAINER)
                        {
                            crashMonitor.mAppWindowHandle = nullptr;
                            smEventHookOwners.erase(hWinEventHook);
                            UnhookWinEvent(hWinEventHook);
                        }
                    },
                    processId, mAppMainThreadId, WINEVENT_OUTOFCONTEXT);
                smEventHookOwners[eventHookHandle] = this;
            }
            else
                return false;
        }
        if (sIsHungAppWindow != nullptr)
            return sIsHungAppWindow(mAppWindowHandle);
        else
        {
            BOOL debuggerPresent;

            if (CheckRemoteDebuggerPresent(mAppProcessHandle, &debuggerPresent) && debuggerPresent)
                return false;
            if (SendMessageTimeoutA(mAppWindowHandle, WM_NULL, 0, 0, 0, 5000, nullptr) == 0)
                return GetLastError() == ERROR_TIMEOUT;
        }
        return false;
    }

    void CrashMonitor::run()
    {
        try
        {
            // app waits for monitor start up, let it continue
            signalApp();

            bool running = true;
            bool frozen = false;
            while (isAppAlive() && running && !mFreezeAbort)
            {
                if (isAppFrozen())
                {
                    if (!frozen)
                    {
                        showFreezeMessageBox();
                        frozen = true;
                    }
                }
                else if (frozen)
                {
                    hideFreezeMessageBox();
                    frozen = false;
                }

                if (!mFreezeAbort && waitApp())
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

            if (frozen)
                hideFreezeMessageBox();

            if (mFreezeAbort)
            {
                handleCrash();
                TerminateProcess(mAppProcessHandle, 0xDEAD);
                std::string message = "OpenMW appears to have frozen.\nCrash log saved to '"
                    + std::string(mShm->mStartup.mLogFilePath)
                    + "'.\nPlease report this to https://gitlab.com/OpenMW/openmw/issues !";
                SDL_ShowSimpleMessageBox(0, "Fatal Error", message.c_str(), nullptr);
            }
        }
        catch (...)
        {
            Log(Debug::Error) << "Exception in crash monitor, exiting";
        }
        signalApp();
    }

    static std::wstring utf8ToUtf16(const std::string& utf8)
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

            using MiniDumpWirteDumpFn = BOOL(WINAPI*)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile,
                MINIDUMP_TYPE DumpType, PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
                PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

            MiniDumpWirteDumpFn miniDumpWriteDump = (MiniDumpWirteDumpFn)GetProcAddress(dbghelp, "MiniDumpWriteDump");
            if (miniDumpWriteDump == NULL)
                return;

            std::wstring utf16Path = utf8ToUtf16(mShm->mStartup.mLogFilePath);
            if (utf16Path.empty())
                return;

            if (utf16Path.length() > MAX_PATH)
                utf16Path = LR"(\\?\)" + utf16Path;

            HANDLE hCrashLog = CreateFileW(utf16Path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL, nullptr);
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
        catch (const std::exception& e)
        {
            Log(Debug::Error) << "CrashMonitor: " << e.what();
        }
        catch (...)
        {
            Log(Debug::Error) << "CrashMonitor: unknown exception";
        }
    }

    void CrashMonitor::showFreezeMessageBox()
    {
        std::thread messageBoxThread([&]() {
            SDL_MessageBoxButtonData button = { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Abort" };
            SDL_MessageBoxData messageBoxData = { SDL_MESSAGEBOX_ERROR, nullptr, "OpenMW appears to have frozen",
                "OpenMW appears to have frozen. Press Abort to terminate it and generate a crash dump.\nIf OpenMW "
                "hasn't actually frozen, this message box will disappear a within a few seconds of it becoming "
                "responsive.",
                1, &button, nullptr };

            int buttonId;
            if (SDL_ShowMessageBox(&messageBoxData, &buttonId) == 0 && buttonId == 0)
                mFreezeAbort = true;
        });

        mFreezeMessageBoxThreadId = GetThreadId(messageBoxThread.native_handle());
        messageBoxThread.detach();
    }

    void CrashMonitor::hideFreezeMessageBox()
    {
        if (!mFreezeMessageBoxThreadId)
            return;

        EnumWindows(
            [](HWND handle, LPARAM param) -> BOOL {
                CrashMonitor& crashMonitor = *(CrashMonitor*)param;
                DWORD processId;
                if (GetWindowThreadProcessId(handle, &processId) == crashMonitor.mFreezeMessageBoxThreadId
                    && processId == GetCurrentProcessId())
                    PostMessage(handle, WM_CLOSE, 0, 0);
                return true;
            },
            (LPARAM)this);

        mFreezeMessageBoxThreadId = 0;
    }

} // namespace Crash
