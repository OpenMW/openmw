#include "thread.hpp"

#include <components/debug/debuglog.hpp>

#include <cstring>
#include <thread>

#ifdef __linux__

#include <pthread.h>
#include <sched.h>

namespace Misc
{
    void setCurrentThreadIdlePriority()
    {
        sched_param param;
        param.sched_priority = 0;
        if (pthread_setschedparam(pthread_self(), SCHED_IDLE, &param) == 0)
            Log(Debug::Verbose) << "Using idle priority for thread=" << std::this_thread::get_id();
        else
            Log(Debug::Warning) << "Failed to set idle priority for thread=" << std::this_thread::get_id() << ": " << std::strerror(errno);
    }
}

#elif defined(WIN32)

#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

namespace Misc
{
    void setCurrentThreadIdlePriority()
    {
        if (SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST))
            Log(Debug::Verbose) << "Using idle priority for thread=" << std::this_thread::get_id();
        else
            Log(Debug::Warning) << "Failed to set idle priority for thread=" << std::this_thread::get_id() << ": " << GetLastError();
    }
}

#elif defined(__FreeBSD__)

#include <sys/types.h>
#include <sys/rtprio.h>

namespace Misc
{
    void setCurrentThreadIdlePriority()
    {
        struct rtprio prio;
        prio.type = RTP_PRIO_IDLE;
        prio.prio = RTP_PRIO_MAX;
        if (rtprio_thread(RTP_SET, 0, &prio) == 0)
            Log(Debug::Verbose) << "Using idle priority for thread=" << std::this_thread::get_id();
        else
            Log(Debug::Warning) << "Failed to set idle priority for thread=" << std::this_thread::get_id() << ": " << std::strerror(errno);
    }
}

#else

namespace Misc
{
    void setCurrentThreadIdlePriority()
    {
        Log(Debug::Warning) << "Idle thread priority is not supported on this system";
    }
}

#endif
