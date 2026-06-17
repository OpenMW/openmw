#include <components/misc/progressreporter.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>

namespace
{
    using namespace testing;
    using namespace Misc;

    struct ReportMock
    {
        MOCK_METHOD(void, call, (std::size_t, std::size_t), ());
    };

    struct Report
    {
        StrictMock<ReportMock>* mImpl;

        void operator()(std::size_t provided, std::size_t expected) { mImpl->call(provided, expected); }
    };

    TEST(MiscProgressReporterTest, shouldCallReportWhenPassedInterval)
    {
        StrictMock<ReportMock> report;
        EXPECT_CALL(report, call(13, 42)).WillOnce(Return());
        ProgressReporter reporter(std::chrono::steady_clock::duration(0), Report{ &report });
        reporter(13, 42);
    }

    TEST(MiscProgressReporterTest, shouldNotCallReportWhenIntervalIsNotPassed)
    {
        StrictMock<ReportMock> report;
        EXPECT_CALL(report, call(13, 42)).Times(0);
        ProgressReporter reporter(std::chrono::seconds(1000), Report{ &report });
        reporter(13, 42);
    }
}
