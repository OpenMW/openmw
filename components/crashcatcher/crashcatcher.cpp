#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>

#include <components/debug/debuglog.hpp>
#include <components/files/conversion.hpp>

#include <SDL_messagebox.h>

#ifdef __linux__
#include <sys/prctl.h>
#include <sys/ucontext.h>
#ifndef PR_SET_PTRACER
#define PR_SET_PTRACER 0x59616d61
#endif
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#include <signal.h>
#endif

#if defined(__APPLE__)
#include <cassert>
#include <libproc.h>
#include <sys/sysctl.h>
#endif

#if defined(__FreeBSD__)
#include <sys/sysctl.h>
#include <sys/user.h>
#endif

#include "crashcatcher.hpp"

static const char fatal_err[] = "\n\n*** Fatal Error ***\n";
static const char pipe_err[] = "!!! Failed to create pipe\n";
static const char fork_err[] = "!!! Failed to fork debug process\n";
static const char exec_err[] = "!!! Failed to exec debug process\n";

#ifndef PATH_MAX /* Not all platforms (GNU Hurd) have this. */
#define PATH_MAX 256
#endif

static char argv0[PATH_MAX];

static struct
{
    int signum;
    pid_t pid;
    std::optional<siginfo_t> siginfo;
} crash_info;

namespace
{
    constexpr char crash_switch[] = "--cc-handle-crash";

    struct SignalInfo
    {
        int mCode;
        const char* mDescription;
        const char* mName = "";
    };

    constexpr SignalInfo signals[] = {
        { SIGSEGV, "Segmentation fault", "SIGSEGV" },
        { SIGILL, "Illegal instruction", "SIGILL" },
        { SIGFPE, "FPU exception", "SIGFPE" },
        { SIGBUS, "System BUS error", "SIGBUS" },
        { SIGABRT, "Abnormal termination condition", "SIGABRT" },
    };

    constexpr SignalInfo sigIllCodes[] = {
#if !defined(__FreeBSD__) && !defined(__FreeBSD_kernel__)
        { ILL_ILLOPC, "Illegal opcode" },
        { ILL_ILLOPN, "Illegal operand" },
        { ILL_ILLADR, "Illegal addressing mode" },
        { ILL_ILLTRP, "Illegal trap" },
        { ILL_PRVOPC, "Privileged opcode" },
        { ILL_PRVREG, "Privileged register" },
        { ILL_COPROC, "Coprocessor error" },
        { ILL_BADSTK, "Internal stack error" },
#endif
    };

    constexpr SignalInfo sigFpeCodes[] = {
        { FPE_INTDIV, "Integer divide by zero" },
        { FPE_INTOVF, "Integer overflow" },
        { FPE_FLTDIV, "Floating point divide by zero" },
        { FPE_FLTOVF, "Floating point overflow" },
        { FPE_FLTUND, "Floating point underflow" },
        { FPE_FLTRES, "Floating point inexact result" },
        { FPE_FLTINV, "Floating point invalid operation" },
        { FPE_FLTSUB, "Subscript out of range" },
    };

    constexpr SignalInfo sigSegvCodes[] = {
#ifndef __FreeBSD__
        { SEGV_MAPERR, "Address not mapped to object" },
        { SEGV_ACCERR, "Invalid permissions for mapped object" },
#endif
    };

    constexpr SignalInfo sigBusCodes[] = {
#ifndef __FreeBSD__
        { BUS_ADRALN, "Invalid address alignment" },
        { BUS_ADRERR, "Non-existent physical address" },
        { BUS_OBJERR, "Object specific hardware error" },
#endif
    };

    const char* findSignalDescription(std::span<const SignalInfo> info, int code)
    {
        const auto it = std::find_if(info.begin(), info.end(), [&](const SignalInfo& v) { return v.mCode == code; });
        return it == info.end() ? "" : it->mDescription;
    }
}

static void printGdbInfo(pid_t pid)
{
    /*
     * Create a temp file to put gdb commands into.
     * Note: POSIX.1-2008 declares that the file should be already created with mode 0600 by default.
     * Modern systems implement it and suggest to do not touch masks in multithreaded applications.
     * So CoverityScan warning is valid only for ancient versions of stdlib.
     */
    char respfile[64] = "/tmp/gdb-respfile-XXXXXX";

#ifdef __COVERITY__
    umask(0600);
#endif

    const int fd = mkstemp(respfile);
    if (fd == -1)
    {
        printf("Failed to call mkstemp: %s\n", std::generic_category().message(errno).c_str());
        return;
    }

    FILE* const f = fdopen(fd, "w");
    if (f == nullptr)
    {
        printf("Failed to open file for gdb output \"%s\": %s\n", respfile,
            std::generic_category().message(errno).c_str());
    }
    else
    {
        fprintf(f,
            "attach %d\n"
            "shell echo \"\"\n"
            "shell echo \"* Loaded Libraries\"\n"
            "info sharedlibrary\n"
            "shell echo \"\"\n"
            "shell echo \"* Threads\"\n"
            "info threads\n"
            "shell echo \"\"\n"
            "shell echo \"* FPU Status\"\n"
            "info float\n"
            "shell echo \"\"\n"
            "shell echo \"* Registers\"\n"
            "info registers\n"
            "shell echo \"\"\n"
            "shell echo \"* Backtrace\"\n"
            "thread apply all backtrace full 1000\n"
            "detach\n"
            "quit\n",
            pid);
        fclose(f);

        /* Run gdb and print process info. */
        char cmd_buf[128];
        snprintf(cmd_buf, sizeof(cmd_buf), "gdb --quiet --batch --command=%s", respfile);
        printf("Executing: %s\n", cmd_buf);
        fflush(stdout);

        int ret = system(cmd_buf);

        if (ret == -1)
            printf(
                "\nFailed to create a crash report: %s.\n"
                "Please make sure that 'gdb' is installed and present in PATH then crash again.\n"
                "Current PATH: %s\n",
                std::generic_category().message(errno).c_str(), getenv("PATH"));
        else if (ret != 0)
            printf(
                "\nFailed to create a crash report.\n"
                "Please make sure that 'gdb' is installed and present in PATH then crash again.\n"
                "Current PATH: %s\n",
                getenv("PATH"));

        fflush(stdout);
    }

    close(fd);
    remove(respfile);
    fflush(stdout);
}

static void sys_info(void)
{
#ifdef __unix__
    struct utsname info;
    if (uname(&info))
        printf("!!! Failed to get system information\n");
    else
        printf("System: %s %s %s %s %s\n", info.sysname, info.nodename, info.release, info.version, info.machine);

    fflush(stdout);
#endif
}

static size_t safe_write(int fd, const void* buf, size_t len)
{
    size_t ret = 0;
    while (ret < len)
    {
        const ssize_t rem = write(fd, (const char*)buf + ret, len - ret);
        if (rem == -1)
        {
            if (errno == EINTR)
                continue;
            break;
        }
        ret += rem;
    }
    return ret;
}

static void crash_catcher(int signum, siginfo_t* siginfo, void* /*context*/)
{
    /* Make sure the effective uid is the real uid */
    if (getuid() != geteuid())
    {
        raise(signum);
        return;
    }

    safe_write(STDERR_FILENO, fatal_err, sizeof(fatal_err) - 1);
    int fd[2];
    if (pipe(fd) == -1)
    {
        safe_write(STDERR_FILENO, pipe_err, sizeof(pipe_err) - 1);
        raise(signum);
        return;
    }

    crash_info.signum = signum;
    crash_info.pid = getpid();
    if (siginfo == nullptr)
        crash_info.siginfo = std::nullopt;
    else
        crash_info.siginfo = *siginfo;

    const pid_t dbg_pid = fork();
    /* Fork off to start a crash handler */
    switch (dbg_pid)
    {
        /* Error */
        case -1:
            safe_write(STDERR_FILENO, fork_err, sizeof(fork_err) - 1);
            raise(signum);
            return;

        case 0:
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
            close(fd[1]);

            execl(argv0, argv0, crash_switch, nullptr);

            safe_write(STDERR_FILENO, exec_err, sizeof(exec_err) - 1);
            _exit(1);

        default:
#ifdef __linux__
            prctl(PR_SET_PTRACER, dbg_pid, 0, 0, 0);
#endif
            safe_write(fd[1], &crash_info, sizeof(crash_info));
            close(fd[0]);
            close(fd[1]);

            /* Wait; we'll be killed when gdb is done */
            do
            {
                int status;
                if (waitpid(dbg_pid, &status, 0) == dbg_pid && (WIFEXITED(status) || WIFSIGNALED(status)))
                {
                    /* The debug process died before it could kill us */
                    raise(signum);
                    break;
                }
            } while (1);
    }
}

[[noreturn]] static void handleCrash(const char* logfile)
{
    if (fread(&crash_info, sizeof(crash_info), 1, stdin) != 1)
    {
        fprintf(stderr, "!!! Failed to retrieve info from crashed process\n");
        exit(1);
    }

    const char* sigdesc = findSignalDescription(signals, crash_info.signum);

    if (crash_info.siginfo.has_value())
    {
        switch (crash_info.signum)
        {
            case SIGSEGV:
                sigdesc = findSignalDescription(sigSegvCodes, crash_info.siginfo->si_code);
                break;

            case SIGFPE:
                sigdesc = findSignalDescription(sigFpeCodes, crash_info.siginfo->si_code);
                break;

            case SIGILL:
                sigdesc = findSignalDescription(sigIllCodes, crash_info.siginfo->si_code);
                break;

            case SIGBUS:
                sigdesc = findSignalDescription(sigBusCodes, crash_info.siginfo->si_code);
                break;
        }
    }
    fprintf(stderr, "%s (signal %i)\n", sigdesc, crash_info.signum);
    if (crash_info.siginfo.has_value())
        fprintf(stderr, "Address: %p\n", crash_info.siginfo->si_addr);
    fputc('\n', stderr);

    /* Create crash log file and redirect shell output to it */
    if (freopen(logfile, "wa", stdout) != stdout)
    {
        fprintf(stderr, "!!! Could not create %s following signal\n", logfile);
        exit(1);
    }
    fprintf(stderr, "Generating %s and killing process %d, please wait... ", logfile, crash_info.pid);

    printf(
        "*** Fatal Error ***\n"
        "%s (signal %i)\n",
        sigdesc, crash_info.signum);
    if (crash_info.siginfo.has_value())
        printf("Address: %p\n", crash_info.siginfo->si_addr);
    fputc('\n', stdout);
    fflush(stdout);

    sys_info();

    fflush(stdout);

    if (crash_info.pid > 0)
    {
        printGdbInfo(crash_info.pid);
        kill(crash_info.pid, SIGKILL);
    }

    // delay between killing of the crashed process and showing the message box to
    // work around occasional X server lock-up. this can only be a bug in X11 since
    // even faulty applications shouldn't be able to freeze the X server.
    usleep(100000);

    const std::string message = "OpenMW has encountered a fatal error.\nCrash log saved to '" + std::string(logfile)
        + "'.\n Please report this to https://gitlab.com/OpenMW/openmw/issues !";
    SDL_ShowSimpleMessageBox(0, "Fatal Error", message.c_str(), nullptr);

    exit(0);
}

static void getExecPath(char** argv)
{
#if defined(__FreeBSD__)
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
    size_t size = sizeof(argv0);

    if (sysctl(mib, 4, argv0, &size, nullptr, 0) == 0)
        return;

    Log(Debug::Warning) << "Failed to call sysctl: " << std::generic_category().message(errno);
#endif

#if defined(__APPLE__)
    if (proc_pidpath(getpid(), argv0, sizeof(argv0)) > 0)
        return;

    Log(Debug::Warning) << "Failed to call proc_pidpath: " << std::generic_category().message(errno);
#endif
    const char* statusPaths[] = { "/proc/self/exe", "/proc/self/file", "/proc/curproc/exe", "/proc/curproc/file" };
    memset(argv0, 0, sizeof(argv0));

    for (const char* path : statusPaths)
    {
        if (readlink(path, argv0, sizeof(argv0)) != -1)
            return;

        Log(Debug::Warning) << "Failed to call readlink for \"" << path
                            << "\": " << std::generic_category().message(errno);
    }

    if (argv[0][0] == '/')
    {
        snprintf(argv0, sizeof(argv0), "%s", argv[0]);
        return;
    }

    if (getcwd(argv0, sizeof(argv0)) == nullptr)
    {
        Log(Debug::Error) << "Failed to call getcwd: " << std::generic_category().message(errno);
        return;
    }

    const int cwdlen = strlen(argv0);
    snprintf(argv0 + cwdlen, sizeof(argv0) - cwdlen, "/%s", argv[0]);
}

static bool crashCatcherInstallHandlers(char** argv)
{
    getExecPath(argv);

    /* Set an alternate signal stack so SIGSEGVs caused by stack overflows
     * still run */
    static char* altstack = new char[SIGSTKSZ];
    stack_t altss;
    altss.ss_sp = altstack;
    altss.ss_flags = 0;
    altss.ss_size = SIGSTKSZ;
    if (sigaltstack(&altss, nullptr) == -1)
    {
        Log(Debug::Error) << "Failed to call sigaltstack: " << std::generic_category().message(errno);
        return false;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = crash_catcher;
    sa.sa_flags = SA_RESETHAND | SA_NODEFER | SA_SIGINFO | SA_ONSTACK;
    if (sigemptyset(&sa.sa_mask) == -1)
    {
        Log(Debug::Error) << "Failed to call sigemptyset: " << std::generic_category().message(errno);
        return false;
    }

    for (const SignalInfo& signal : signals)
    {
        if (sigaction(signal.mCode, &sa, nullptr) == -1)
        {
            Log(Debug::Error) << "Failed to call sigaction for signal " << signal.mName << " (" << signal.mCode
                              << "): " << std::generic_category().message(errno);
            return false;
        }
    }

    return true;
}

static bool is_debugger_present()
{
#if defined(__linux__)
    std::filesystem::path procstatus = std::filesystem::path("/proc/self/status");
    if (std::filesystem::exists(procstatus))
    {
        std::ifstream file((procstatus));
        while (!file.eof())
        {
            std::string word;
            file >> word;
            if (word == "TracerPid:")
            {
                file >> word;
                return word != "0";
            }
        }
    }
    return false;
#elif defined(__APPLE__)
    struct kinfo_proc info;

    // Initialize the flags so that, if sysctl fails for some bizarre
    // reason, we get a predictable result.

    info.kp_proc.p_flag = 0;

    // Initialize mib, which tells sysctl the info we want, in this case
    // we're looking for information about a specific process ID.
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid() };

    // Call sysctl.

    size_t size = sizeof(info);
    const int junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, nullptr, 0);
    assert(junk == 0);

    // We're being debugged if the P_TRACED flag is set.

    return (info.kp_proc.p_flag & P_TRACED) != 0;
#elif defined(__FreeBSD__)
    struct kinfo_proc info;
    size_t size = sizeof(info);
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid() };

    if (sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, nullptr, 0) == 0)
        return (info.ki_flag & P_TRACED) != 0;
    else
        perror("Failed to retrieve process info");
    return false;
#else
    return false;
#endif
}

void crashCatcherInstall(int argc, char** argv, const std::filesystem::path& crashLogPath)
{
#if (defined(__APPLE__) || (defined(__linux) && !defined(ANDROID)) || (defined(__unix) && !defined(ANDROID))           \
    || defined(__posix))
    if (argc == 2 && strcmp(argv[1], crash_switch) == 0)
        handleCrash(Files::pathToUnicodeString(crashLogPath).c_str());

    if (is_debugger_present())
        return;

    if (crashCatcherInstallHandlers(argv))
        Log(Debug::Info) << "Crash handler installed";
    else
        Log(Debug::Warning) << "Installing crash handler failed";
#endif
}
