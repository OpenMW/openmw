#include <benchmark/benchmark.h>

#include <components/files/configurationmanager.hpp>
#include <components/settings/settings.hpp>
#include <components/settings/values.hpp>

#include <boost/program_options.hpp>

#include <algorithm>
#include <iostream>

namespace
{
    namespace bpo = boost::program_options;

    bpo::options_description makeOptionsDescription()
    {
        bpo::options_description result;
        auto addOption = result.add_options();
        addOption("help", "print help message");
        Files::ConfigurationManager::addCommonOptions(result);
        return result;
    }

    void settingsManager(benchmark::State& state)
    {
        for (auto _ : state)
        {
            benchmark::DoNotOptimize(Settings::Manager::getFloat("sky blending start", "Fog"));
        }
    }

    void localStatic(benchmark::State& state)
    {
        for (auto _ : state)
        {
            static const float v = Settings::Manager::getFloat("sky blending start", "Fog");
            benchmark::DoNotOptimize(v);
        }
    }

    void settingsStorage(benchmark::State& state)
    {
        for (auto _ : state)
        {
            benchmark::DoNotOptimize(Settings::fog().mSkyBlendingStart.get());
        }
    }
}

BENCHMARK(settingsManager);
BENCHMARK(localStatic);
BENCHMARK(settingsStorage);

int main(int argc, char* argv[])
{
    bpo::options_description desc = makeOptionsDescription();

    bpo::parsed_options options = bpo::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
    bpo::variables_map variables;

    bpo::store(options, variables);
    bpo::notify(variables);

    if (variables.find("help") != variables.end())
    {
        std::cout << desc << std::endl;
        benchmark::Initialize(&argc, argv);
        benchmark::Shutdown();
        return 1;
    }

    Files::ConfigurationManager config;

    bpo::variables_map composingVariables = Files::separateComposingVariables(variables, desc);
    config.readConfiguration(variables, desc);
    Files::mergeComposingVariables(variables, composingVariables, desc);

    Settings::Manager::load(config);

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();

    return 0;
}
