#ifndef OPENMW_COMPONENTS_MISC_PROGRESSREPORTER_H
#define OPENMW_COMPONENTS_MISC_PROGRESSREPORTER_H

#include <algorithm>
#include <chrono>
#include <mutex>
#include <type_traits>
#include <utility>

namespace Misc
{
    template <class Report>
    class ProgressReporter
    {
    public:
        explicit ProgressReporter(Report&& report = Report {})
            : mReport(std::forward<Report>(report))
        {}

        explicit ProgressReporter(std::chrono::steady_clock::duration interval, Report&& report = Report {})
            : mInterval(interval)
            , mReport(std::forward<Report>(report))
        {}

        void operator()(std::size_t provided, std::size_t expected)
        {
            expected = std::max(expected, provided);
            const bool shouldReport = [&]
            {
                const std::lock_guard lock(mMutex);
                const auto now = std::chrono::steady_clock::now();
                if (mNextReport > now || provided == expected)
                    return false;
                if (mInterval.count() > 0)
                    mNextReport = mNextReport + mInterval * ((now - mNextReport + mInterval).count() / mInterval.count());
                return true;
            } ();
            if (shouldReport)
                mReport(provided, expected);
        }

    private:
        const std::chrono::steady_clock::duration mInterval = std::chrono::seconds(1);
        Report mReport;
        std::mutex mMutex;
        std::chrono::steady_clock::time_point mNextReport {std::chrono::steady_clock::now() + mInterval};
    };
}

#endif
