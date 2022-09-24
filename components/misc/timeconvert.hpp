#ifndef OPENMW_COMPONENTS_MISC_TIMECONVERT_H
#define OPENMW_COMPONENTS_MISC_TIMECONVERT_H

#include <chrono>

namespace Misc
{
    template <typename TP>
    inline std::time_t toTimeT(TP tp)
    {
        using namespace std::chrono;
        auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now() + system_clock::now());
        return system_clock::to_time_t(sctp);
    }

    inline std::string timeTToString(const std::time_t tp, const char* fmt)
    {
        tm time_info{};
#ifdef _WIN32
        (void)localtime_s(&time_info, &tp);
#else
        (void)localtime_r(&tp, &time_info);
#endif
        std::stringstream out;
        out << std::put_time(&time_info, fmt);
        return out.str();
    }

    inline std::string fileTimeToString(const std::filesystem::file_time_type& tp, const char* fmt)
    {
        return timeTToString(toTimeT(tp), fmt);
    }

    inline std::string timeToString(const std::chrono::system_clock::time_point& tp, const char* fmt)
    {
        return timeTToString(std::chrono::system_clock::to_time_t(tp), fmt);
    }
} // namespace Misc

#endif
