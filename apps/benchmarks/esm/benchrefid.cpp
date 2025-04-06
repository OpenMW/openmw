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

    template <class Random>
    std::vector<ESM::RefId> generateIndexRefIds(Random& random)
    {
        std::vector<ESM::RefId> result;
        result.reserve(refIdsCount);
        std::uniform_int_distribution<std::uint32_t> distribution(0, std::numeric_limits<std::uint32_t>::max());
        std::generate_n(std::back_inserter(result), refIdsCount,
            [&] { return ESM::IndexRefId(ESM::REC_ARMO, distribution(random)); });
        return result;
    }

    template <class Random, class Serialize>
    std::vector<std::string> generateSerializedIndexRefIds(Random& random, Serialize&& serialize)
    {
        return generateSerializedRefIds(generateIndexRefIds(random), serialize);
    }

    template <class Random>
    std::vector<ESM::RefId> generateGeneratedRefIds(Random& random)
    {
        std::vector<ESM::RefId> result;
        result.reserve(refIdsCount);
        std::uniform_int_distribution<std::uint64_t> distribution(0, std::numeric_limits<std::uint64_t>::max());
        std::generate_n(
            std::back_inserter(result), refIdsCount, [&] { return ESM::GeneratedRefId(distribution(random)); });
        return result;
    }

    template <class Random, class Serialize>
    std::vector<std::string> generateSerializedGeneratedRefIds(Random& random, Serialize&& serialize)
    {
        return generateSerializedRefIds(generateGeneratedRefIds(random), serialize);
    }

    template <class Random>
    std::vector<ESM::RefId> generateESM3ExteriorCellRefIds(Random& random)
    {
        std::vector<ESM::RefId> result;
        result.reserve(refIdsCount);
        std::uniform_int_distribution<std::int32_t> distribution(-100, 100);
        std::generate_n(std::back_inserter(result), refIdsCount,
            [&] { return ESM::ESM3ExteriorCellRefId(distribution(random), distribution(random)); });
        return result;
    }

    template <class Random, class Serialize>
    std::vector<std::string> generateSerializedESM3ExteriorCellRefIds(Random& random, Serialize&& serialize)
    {
        return generateSerializedRefIds(generateESM3ExteriorCellRefIds(random), serialize);
    }

    void serializeRefId(benchmark::State& state)
    {
        std::minstd_rand random;
        std::vector<ESM::RefId> refIds = generateStringRefIds(state.range(0), random);
        std::size_t i = 0;
        for ([[maybe_unused]] auto _ : state)
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
        for ([[maybe_unused]] auto _ : state)
        {
            benchmark::DoNotOptimize(ESM::RefId::deserialize(serializedRefIds[i]));
            if (++i >= serializedRefIds.size())
                i = 0;
        }
    }

    void serializeTextStringRefId(benchmark::State& state)
    {
        std::minstd_rand random;
        std::vector<ESM::RefId> refIds = generateStringRefIds(state.range(0), random);
        std::size_t i = 0;
        for ([[maybe_unused]] auto _ : state)
        {
            benchmark::DoNotOptimize(refIds[i].serializeText());
            if (++i >= refIds.size())
                i = 0;
        }
    }

    void deserializeTextStringRefId(benchmark::State& state)
    {
        std::minstd_rand random;
        std::vector<std::string> serializedRefIds
            = generateSerializedStringRefIds(state.range(0), random, [](ESM::RefId v) { return v.serializeText(); });
        std::size_t i = 0;
        for ([[maybe_unused]] auto _ : state)
        {
            benchmark::DoNotOptimize(ESM::RefId::deserializeText(serializedRefIds[i]));
            if (++i >= serializedRefIds.size())
                i = 0;
        }
    }

    void serializeTextGeneratedRefId(benchmark::State& state)
    {
        std::minstd_rand random;
        std::vector<ESM::RefId> refIds = generateGeneratedRefIds(random);
        std::size_t i = 0;
        for ([[maybe_unused]] auto _ : state)
        {
            benchmark::DoNotOptimize(refIds[i].serializeText());
            if (++i >= refIds.size())
                i = 0;
        }
    }

    void deserializeTextGeneratedRefId(benchmark::State& state)
    {
        std::minstd_rand random;
        std::vector<std::string> serializedRefIds
            = generateSerializedGeneratedRefIds(random, [](ESM::RefId v) { return v.serializeText(); });
        std::size_t i = 0;
        for ([[maybe_unused]] auto _ : state)
        {
            benchmark::DoNotOptimize(ESM::RefId::deserializeText(serializedRefIds[i]));
            if (++i >= serializedRefIds.size())
                i = 0;
        }
    }

    void serializeTextIndexRefId(benchmark::State& state)
    {
        std::minstd_rand random;
        std::vector<ESM::RefId> refIds = generateIndexRefIds(random);
        std::size_t i = 0;
        for ([[maybe_unused]] auto _ : state)
        {
            benchmark::DoNotOptimize(refIds[i].serializeText());
            if (++i >= refIds.size())
                i = 0;
        }
    }

    void deserializeTextIndexRefId(benchmark::State& state)
    {
        std::minstd_rand random;
        std::vector<std::string> serializedRefIds
            = generateSerializedIndexRefIds(random, [](ESM::RefId v) { return v.serializeText(); });
        std::size_t i = 0;
        for ([[maybe_unused]] auto _ : state)
        {
            benchmark::DoNotOptimize(ESM::RefId::deserializeText(serializedRefIds[i]));
            if (++i >= serializedRefIds.size())
                i = 0;
        }
    }

    void serializeTextESM3ExteriorCellRefId(benchmark::State& state)
    {
        std::minstd_rand random;
        std::vector<ESM::RefId> refIds = generateESM3ExteriorCellRefIds(random);
        std::size_t i = 0;
        for ([[maybe_unused]] auto _ : state)
        {
            benchmark::DoNotOptimize(refIds[i].serializeText());
            if (++i >= refIds.size())
                i = 0;
        }
    }

    void deserializeTextESM3ExteriorCellRefId(benchmark::State& state)
    {
        std::minstd_rand random;
        std::vector<std::string> serializedRefIds
            = generateSerializedESM3ExteriorCellRefIds(random, [](ESM::RefId v) { return v.serializeText(); });
        std::size_t i = 0;
        for ([[maybe_unused]] auto _ : state)
        {
            benchmark::DoNotOptimize(ESM::RefId::deserializeText(serializedRefIds[i]));
            if (++i >= serializedRefIds.size())
                i = 0;
        }
    }
}

BENCHMARK(serializeRefId)->RangeMultiplier(4)->Range(8, 64);
BENCHMARK(deserializeRefId)->RangeMultiplier(4)->Range(8, 64);
BENCHMARK(serializeTextStringRefId)->RangeMultiplier(4)->Range(8, 64);
BENCHMARK(deserializeTextStringRefId)->RangeMultiplier(4)->Range(8, 64);
BENCHMARK(serializeTextGeneratedRefId);
BENCHMARK(deserializeTextGeneratedRefId);
BENCHMARK(serializeTextIndexRefId);
BENCHMARK(deserializeTextIndexRefId);
BENCHMARK(serializeTextESM3ExteriorCellRefId);
BENCHMARK(deserializeTextESM3ExteriorCellRefId);

BENCHMARK_MAIN();
