#ifndef OPENMW_COMPONENTS_MISC_FRAMESTATS_H
#define OPENMW_COMPONENTS_MISC_FRAMESTATS_H

#include <chrono>
#include <string>

namespace Misc
{
    // Per-frame profiling values reported by engine subsystems. The renderer owns the actual store and the on-screen
    // display, so code outside of it reports through this interface and the engine adapts it to the renderer.
    //
    // Implementations must be safe to call concurrently: the Lua worker reports from its own thread while the main
    // thread reports and the renderer reads. They must store each value under the frame number it is reported with,
    // and keep a history of recent frames writable, because values arrive late: physics reports the preceding frame
    // and GPU timings come in after submission.
    //
    // Timing attributes (*_time_begin, *_time_end) are seconds since a single epoch shared by every reporter, the
    // viewer start, so engine spans line up with each other and with GPU spans. *_time_taken is a plain duration.
    class FrameStats
    {
    public:
        virtual ~FrameStats() = default;

        virtual bool collectStats(const std::string& category) const = 0;

        virtual void setAttribute(unsigned int frameNumber, const std::string& name, double value) = 0;
    };

    using FrameClock = std::chrono::steady_clock;

    inline double secondsBetween(FrameClock::time_point from, FrameClock::time_point to)
    {
        return std::chrono::duration<double>(to - from).count();
    }
}

#endif
