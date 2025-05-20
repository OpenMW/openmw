#include <benchmark/benchmark.h>

#include "components/misc/strings/conversion.hpp"
#include "components/settings/parser.hpp"
#include "components/settings/settings.hpp"
#include "components/settings/values.hpp"

namespace
{
    void settingsManager(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            benchmark::DoNotOptimize(Settings::Manager::getFloat("sky blending start", "Fog"));
        }
    }

    void settingsManager2(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            benchmark::DoNotOptimize(Settings::Manager::getFloat("near clip", "Camera"));
            benchmark::DoNotOptimize(Settings::Manager::getBool("transparent postpass", "Post Processing"));
        }
    }

    void settingsManager3(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            benchmark::DoNotOptimize(Settings::Manager::getFloat("near clip", "Camera"));
            benchmark::DoNotOptimize(Settings::Manager::getBool("transparent postpass", "Post Processing"));
            benchmark::DoNotOptimize(Settings::Manager::getInt("reflection detail", "Water"));
        }
    }

    void localStatic(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            static float v = Settings::Manager::getFloat("sky blending start", "Fog");
            benchmark::DoNotOptimize(v);
        }
    }

    void localStatic2(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            static float v1 = Settings::Manager::getFloat("near clip", "Camera");
            static bool v2 = Settings::Manager::getBool("transparent postpass", "Post Processing");
            benchmark::DoNotOptimize(v1);
            benchmark::DoNotOptimize(v2);
        }
    }

    void localStatic3(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            static float v1 = Settings::Manager::getFloat("near clip", "Camera");
            static bool v2 = Settings::Manager::getBool("transparent postpass", "Post Processing");
            static int v3 = Settings::Manager::getInt("reflection detail", "Water");
            benchmark::DoNotOptimize(v1);
            benchmark::DoNotOptimize(v2);
            benchmark::DoNotOptimize(v3);
        }
    }

    void settingsStorage(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            float v = Settings::fog().mSkyBlendingStart.get();
            benchmark::DoNotOptimize(v);
        }
    }

    void settingsStorage2(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            bool v1 = Settings::postProcessing().mTransparentPostpass.get();
            float v2 = Settings::camera().mNearClip.get();
            benchmark::DoNotOptimize(v1);
            benchmark::DoNotOptimize(v2);
        }
    }

    void settingsStorage3(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            bool v1 = Settings::postProcessing().mTransparentPostpass.get();
            float v2 = Settings::camera().mNearClip.get();
            int v3 = Settings::water().mReflectionDetail.get();
            benchmark::DoNotOptimize(v1);
            benchmark::DoNotOptimize(v2);
            benchmark::DoNotOptimize(v3);
        }
    }

    void settingsStorageGet(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            benchmark::DoNotOptimize(Settings::get<float>("Fog", "sky blending start"));
        }
    }

    void settingsStorageGet2(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            benchmark::DoNotOptimize(Settings::get<bool>("Post Processing", "transparent postpass"));
            benchmark::DoNotOptimize(Settings::get<float>("Camera", "near clip"));
        }
    }

    void settingsStorageGet3(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            benchmark::DoNotOptimize(Settings::get<bool>("Post Processing", "transparent postpass"));
            benchmark::DoNotOptimize(Settings::get<float>("Camera", "near clip"));
            benchmark::DoNotOptimize(Settings::get<int>("Water", "reflection detail"));
        }
    }
}

BENCHMARK(settingsManager);
BENCHMARK(localStatic);
BENCHMARK(settingsStorage);
BENCHMARK(settingsStorageGet);

BENCHMARK(settingsManager2);
BENCHMARK(localStatic2);
BENCHMARK(settingsStorage2);
BENCHMARK(settingsStorageGet2);

BENCHMARK(settingsManager3);
BENCHMARK(localStatic3);
BENCHMARK(settingsStorage3);
BENCHMARK(settingsStorageGet3);

int main(int argc, char* argv[])
{
    const std::filesystem::path settingsDefaultPath = std::filesystem::path{ OPENMW_PROJECT_SOURCE_DIR } / "files"
        / Misc::StringUtils::stringToU8String("settings-default.cfg");

    Settings::SettingsFileParser parser;
    parser.loadSettingsFile(settingsDefaultPath, Settings::Manager::mDefaultSettings);

    Settings::StaticValues::initDefaults();

    Settings::Manager::mUserSettings = Settings::Manager::mDefaultSettings;
    Settings::Manager::mUserSettings.erase({ "Camera", "near clip" });
    Settings::Manager::mUserSettings.erase({ "Post Processing", "transparent postpass" });
    Settings::Manager::mUserSettings.erase({ "Water", "reflection detail" });

    Settings::StaticValues::init();

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();

    return 0;
}
