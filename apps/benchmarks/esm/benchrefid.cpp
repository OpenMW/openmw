#include <benchmark/benchmark.h>

#include "components/esm/refid.hpp"

#include <algorithm>
#include <cstddef>
#include <random>
#include <string>
#include <vector>

namespace
{
    constexpr std::size_t refIdsCount = 64 * 1024;

    template <class Random>
    std::string generateText(std::size_t size, Random& random)
    {
        std::uniform_int_distribution<int> distribution('A', 'z');
        std::string result;
        result.reserve(size);
        std::generate_n(std::back_inserter(result), size, [&] { return distribution(random); });
        return result;
    }

    template <class Random>
    std::vector<ESM::RefId> generateStringRefIds(std::size_t size, Random& random)
    {
        std::vector<ESM::RefId> result;
        result.reserve(refIdsCount);
        std::generate_n(
            std::back_inserter(result), refIdsCount, [&] { return ESM::StringRefId(generateText(size, random)); });
        return result;
    }

    template <class Serialize>
    std::vector<std::string> generateSerializedRefIds(const std::vector<ESM::RefId>& generated, Serialize&& serialize)
    {
        std::vector<std::string> result;
        result.reserve(generated.size());
        for (ESM::RefId refId : generated)
            result.push_back(serialize(refId));
        return result;
    }

    template <class Random, class Serialize>
    std::vector<std::string> generateSerializedStringRefIds(std::size_t size, Random& random, Serialize&& serialize)
    {
        return generateSerializedRefIds(generateStringRefIds(size, random), serialize);
    }

    void serializeRefId(benchmark::State& state)
    {
        std::minstd_rand random;
        std::vector<ESM::RefId> refIds = generateStringRefIds(state.range(0), random);
        std::size_t i = 0;
        for (auto _ : state)
        {
            benchmark::DoNotOptimize(refIds[i].serialize());
            if (++i >= refIds.size())
                i = 0;
        }
    }

    void deserializeRefId(benchmark::State& state)
    {
        std::minstd_rand random;
        std::vector<std::string> serializedRefIds
            = generateSerializedStringRefIds(state.range(0), random, [](ESM::RefId v) { return v.serialize(); });
        std::size_t i = 0;
        for (auto _ : state)
        {
            benchmark::DoNotOptimize(ESM::RefId::deserialize(serializedRefIds[i]));
            if (++i >= serializedRefIds.size())
                i = 0;
        }
    }
}

BENCHMARK(serializeRefId)->RangeMultiplier(4)->Range(8, 64);
BENCHMARK(deserializeRefId)->RangeMultiplier(4)->Range(8, 64);

BENCHMARK_MAIN();
