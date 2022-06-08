#ifndef OPENMW_COMPONENTS_MISC_TIMECONVERT_H
#define OPENMW_COMPONENTS_MISC_TIMECONVERT_H

namespace Misc
{
// Very ugly hack to go from std::chrono::file_clock to any other clock, can be replaced with better solution in C++20
// https://stackoverflow.com/questions/35282308/convert-between-c11-clocks
template <typename DstTimePointT, typename SrcTimePointT, typename DstClockT = typename DstTimePointT::clock, typename SrcClockT = typename SrcTimePointT::clock>
inline DstTimePointT clockCast (const SrcTimePointT tp)
{
    const auto src_now = SrcClockT::now();
    const auto dst_now = DstClockT::now();
    return dst_now + (tp - src_now);
}
} // namespace Misc

#endif
