#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include <pthread.h>
#include <stdbool.h>
#include <sys/ptrace.h>

#include <string>

#include <SDL_messagebox.h>

#ifdef __linux__
#include <sys/prctl.h>
#include <sys/ucontext.h>
#ifndef PR_SET_PTRACER
#define PR_SET_PTRACER 0x59616d61
#endif
#elif defined (__APPLE__) || defined (__FreeBSD__) || defined(__OpenBSD__)
#include <signal.h>
#endif

#define UNUSED(x) (void)(x)

static const char crash_switch[] = "--cc-handle-crash";

static const char fatal_err[] = "\n\n*** Fatal Error ***\n";
static const char pipe_err[] = "!!! Failed to create pipe\n";
static const char fork_err[] = "!!! Failed to fork debug process\n";
static const char exec_err[] = "!!! Failed to exec debug process\n";

#ifndef PATH_MAX /* Not all platforms (GNU Hurd) have this. */
#   define PATH_MAX 256
#endif

static char argv0[PATH_MAX];

static char altstack[SIGSTKSZ];


static struct {
    int signum;
    pid_t pid;
    int has_siginfo;
    siginfo_t siginfo;
    char buf[1024];
} crash_info;


static const struct {
    const char *name;
    int signum;
} signals[] = {
{ "Segmentation fault", SIGSEGV },
{ "Illegal instruction", SIGILL },
{ "FPU exception", SIGFPE },
{ "System BUS error", SIGBUS },
{ NULL, 0 }
};

static const struct {
    int code;
    const char *name;
} sigill_codes[] = {
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
    { 0, NULL }
};

static const struct {
    int code;
    const char *name;
} sigfpe_codes[] = {
    { FPE_INTDIV, "Integer divide by zero" },
    { FPE_INTOVF, "Integer overflow" },
    { FPE_FLTDIV, "Floating point divide by zero" },
    { FPE_FLTOVF, "Floating point overflow" },
    { FPE_FLTUND, "Floating point underflow" },
    { FPE_FLTRES, "Floating point inexact result" },
    { FPE_FLTINV, "Floating point invalid operation" },
    { FPE_FLTSUB, "Subscript out of range" },
    { 0, NULL }
};

static const struct {
    int code;
    const char *name;
} sigsegv_codes[] = {
    #ifndef __FreeBSD__
    { SEGV_MAPERR, "Address not mapped to object" },
    { SEGV_ACCERR, "Invalid permissions for mapped object" },
    #endif
    { 0, NULL }
};

static const struct {
    int code;
    const char *name;
} sigbus_codes[] = {
    #ifndef __FreeBSD__
    { BUS_ADRALN, "Invalid address alignment" },
    { BUS_ADRERR, "Non-existent physical address" },
    { BUS_OBJERR, "Object specific hardware error" },
    #endif
    { 0, NULL }
};

static int (*cc_user_info)(char*, char*);


static void gdb_info(pid_t pid)
{
    char respfile[64];
    FILE *f;
    int fd;

    /* Create a temp file to put gdb commands into */
    strcpy(respfile, "/tmp/gdb-respfile-XXXXXX");
    if((fd=mkstemp(respfile)) >= 0 && (f=fdopen(fd, "w")) != NULL)
    {
        fprintf(f, "attach %d\n"
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
                "quit\n", pid);
        fclose(f);

        /* Run gdb and print process info. */
        char cmd_buf[128];
        snprintf(cmd_buf, sizeof(cmd_buf), "gdb --quiet --batch --command=%s", respfile);
        printf("Executing: %s\n", cmd_buf);
        fflush(stdout);

        int ret = system(cmd_buf);

        if (ret != 0)
            printf("\nFailed to create a crash report. Please install 'gdb' and crash again!\n");
        fflush(stdout);

        /* Clean up */
        remove(respfile);
    }
    else
    {
        /* Error creating temp file */
        if(fd >= 0)
        {
            close(fd);
            remove(respfile);
        }
        printf("!!! Could not create gdb command file\n");
    }
    fflush(stdout);
}

static void sys_info(void)
{
#ifdef __unix__
    struct utsname info;
    if(uname(&info))
        printf("!!! Failed to get system information\n");
    else
        printf("System: %s %s %s %s %s\n",
               info.sysname, info.nodename, info.release, info.version, info.machine);

    fflush(stdout);
#endif
}

static size_t safe_write(int fd, const void *buf, size_t len)
{
    size_t ret = 0;
    while(ret < len)
    {
        ssize_t rem;
        if((rem=write(fd, (const char*)buf+ret, len-ret)) == -1)
        {
            if(errno == EINTR)
                continue;
            break;
        }
        ret += rem;
    }
    return ret;
}

static void crash_catcher(int signum, siginfo_t *siginfo, void *context)
{
    //ucontext_t *ucontext = (ucontext_t*)context;
    pid_t dbg_pid;
    int fd[2];

    /* Make sure the effective uid is the real uid */
    if(getuid() != geteuid())
    {
        raise(signum);
        return;
    }

    safe_write(STDERR_FILENO, fatal_err, sizeof(fatal_err)-1);
    if(pipe(fd) == -1)
    {
        safe_write(STDERR_FILENO, pipe_err, sizeof(pipe_err)-1);
        raise(signum);
        return;
    }

    crash_info.signum = signum;
    crash_info.pid = getpid();
    crash_info.has_siginfo = !!siginfo;
    if(siginfo)
        crash_info.siginfo = *siginfo;
    if(cc_user_info)
        cc_user_info(crash_info.buf, crash_info.buf+sizeof(crash_info.buf));

    /* Fork off to start a crash handler */
    switch((dbg_pid=fork()))
    {
    /* Error */
    case -1:
        safe_write(STDERR_FILENO, fork_err, sizeof(fork_err)-1);
        raise(signum);
        return;

    case 0:
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        close(fd[1]);

        execl(argv0, argv0, crash_switch, NULL);

        safe_write(STDERR_FILENO, exec_err, sizeof(exec_err)-1);
        _exit(1);

    default:
#ifdef __linux__
        prctl(PR_SET_PTRACER, dbg_pid, 0, 0, 0);
#endif
        safe_write(fd[1], &crash_info, sizeof(crash_info));
        close(fd[0]);
        close(fd[1]);

        /* Wait; we'll be killed when gdb is done */
        do {
            int status;
            if(waitpid(dbg_pid, &status, 0) == dbg_pid &&
                    (WIFEXITED(status) || WIFSIGNALED(status)))
            {
                /* The debug process died before it could kill us */
                raise(signum);
                break;
            }
        } while(1);
    }
}

static void crash_handler(const char *logfile)
{
    const char *sigdesc = "";
    int i;

    if(fread(&crash_info, sizeof(crash_info), 1, stdin) != 1)
    {
        fprintf(stderr, "!!! Failed to retrieve info from crashed process\n");
        exit(1);
    }

    /* Get the signal description */
    for(i = 0;signals[i].name;++i)
    {
        if(signals[i].signum == crash_info.signum)
        {
            sigdesc = signals[i].name;
            break;
        }
    }

    if(crash_info.has_siginfo)
    {
        switch(crash_info.signum)
        {
        case SIGSEGV:
            for(i = 0;sigsegv_codes[i].name;++i)
            {
                if(sigsegv_codes[i].code == crash_info.siginfo.si_code)
                {
                    sigdesc = sigsegv_codes[i].name;
                    break;
                }
            }
            break;

        case SIGFPE:
            for(i = 0;sigfpe_codes[i].name;++i)
            {
                if(sigfpe_codes[i].code == crash_info.siginfo.si_code)
                {
                    sigdesc = sigfpe_codes[i].name;
                    break;
                }
            }
            break;

        case SIGILL:
            for(i = 0;sigill_codes[i].name;++i)
            {
                if(sigill_codes[i].code == crash_info.siginfo.si_code)
                {
                    sigdesc = sigill_codes[i].name;
                    break;
                }
            }
            break;

        case SIGBUS:
            for(i = 0;sigbus_codes[i].name;++i)
            {
                if(sigbus_codes[i].code == crash_info.siginfo.si_code)
                {
                    sigdesc = sigbus_codes[i].name;
                    break;
                }
            }
            break;
        }
    }
    fprintf(stderr, "%s (signal %i)\n", sigdesc, crash_info.signum);
    if(crash_info.has_siginfo)
        fprintf(stderr, "Address: %p\n", crash_info.siginfo.si_addr);
    fputc('\n', stderr);

    if(logfile)
    {
        /* Create crash log file and redirect shell output to it */
        if(freopen(logfile, "wa", stdout) != stdout)
        {
            fprintf(stderr, "!!! Could not create %s following signal\n", logfile);
            exit(1);
        }
        fprintf(stderr, "Generating %s and killing process %d, please wait... ", logfile, crash_info.pid);

        printf("*** Fatal Error ***\n"
               "%s (signal %i)\n", sigdesc, crash_info.signum);
        if(crash_info.has_siginfo)
            printf("Address: %p\n", crash_info.siginfo.si_addr);
        fputc('\n', stdout);
        fflush(stdout);
    }

    sys_info();

    crash_info.buf[sizeof(crash_info.buf)-1] = '\0';
    printf("%s\n", crash_info.buf);
    fflush(stdout);

    if(crash_info.pid > 0)
    {
        gdb_info(crash_info.pid);
        kill(crash_info.pid, SIGKILL);
    }

    // delay between killing of the crashed process and showing the message box to
    // work around occasional X server lock-up. this can only be a bug in X11 since
    // even faulty applications shouldn't be able to freeze the X server.
    usleep(100000);

    if(logfile)
    {
        std::string message = "OpenMW has encountered a fatal error.\nCrash log saved to '" + std::string(logfile) + "'.\n Please report this to https://bugs.openmw.org !";
        SDL_ShowSimpleMessageBox(0, "Fatal Error", message.c_str(), NULL);
    }

    exit(0);
}

int cc_install_handlers(int argc, char **argv, int num_signals, int *signals, const char *logfile, int (*user_info)(char*, char*))
{
    struct sigaction sa;
    stack_t altss;
    int retval;

    if(argc == 2 && strcmp(argv[1], crash_switch) == 0)
        crash_handler(logfile);

    cc_user_info = user_info;

    if(argv[0][0] == '/')
        snprintf(argv0, sizeof(argv0), "%s", argv[0]);
    else
    {
        {
            /* we don't want to disable "ignoring return value" warnings, so we make
             * a special exception here. */
            char * unused;
            unused = getcwd(argv0, sizeof(argv0));
            UNUSED(unused);
        }
        retval = strlen(argv0);
        snprintf(argv0+retval, sizeof(argv0)-retval, "/%s", argv[0]);
    }

    /* Set an alternate signal stack so SIGSEGVs caused by stack overflows
     * still run */
    altss.ss_sp = altstack;
    altss.ss_flags = 0;
    altss.ss_size = sizeof(altstack);
    sigaltstack(&altss, NULL);

    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = crash_catcher;
    sa.sa_flags = SA_RESETHAND | SA_NODEFER | SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&sa.sa_mask);

    retval = 0;
    while(num_signals--)
    {
        if((*signals != SIGSEGV && *signals != SIGILL && *signals != SIGFPE && *signals != SIGABRT &&
            *signals != SIGBUS) || sigaction(*signals, &sa, NULL) == -1)
        {
            *signals = 0;
            retval = -1;
        }
        ++signals;
    }
    return retval;
}


// gdb apparently opens FD(s) 3,4,5 (whereas a typical prog uses only stdin=0, stdout=1,stderr=2)
bool
is_debugger_attached(void)
{
    bool rc = false;
    FILE *fd = fopen("/tmp", "r");

    if (fileno(fd) > 5)
    {
        rc = true;
    }

    fclose(fd);
    return rc;
}
