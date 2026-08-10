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
            Log(Debug::Warning) << "Failed to set idle priority for thread=" << std::this_thread::get_id() << ": "
                                << std::generic_category().message(errno);
    }
}

#elif defined(WIN32)

#include <components/misc/windows.hpp>

namespace Misc
{
    void setCurrentThreadIdlePriority()
    {
        // Background mode also lowers I/O priority.
        if (SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST)
            && SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN))
            Log(Debug::Verbose) << "Using idle priority for thread=" << std::this_thread::get_id();
        else
            Log(Debug::Warning) << "Failed to set idle priority for thread=" << std::this_thread::get_id() << ": "
                                << GetLastError();
    }
}

#elif defined(__FreeBSD__)

#include <sys/rtprio.h>
#include <sys/types.h>

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
            Log(Debug::Warning) << "Failed to set idle priority for thread=" << std::this_thread::get_id() << ": "
                                << std::generic_category().message(errno);
    }
}

#elif defined(__APPLE__)

#include <pthread/qos.h>

namespace Misc
{
    void setCurrentThreadIdlePriority()
    {
        // Background QoS also throttles I/O.
        const int result = pthread_set_qos_class_self_np(QOS_CLASS_BACKGROUND, 0);
        if (result == 0)
            Log(Debug::Verbose) << "Using idle priority for thread=" << std::this_thread::get_id();
        else
            Log(Debug::Warning) << "Failed to set idle priority for thread=" << std::this_thread::get_id() << ": "
                                << std::generic_category().message(result);
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
