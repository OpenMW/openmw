#include "apps/openmw/mwsound/warmqueue.hpp"

#include "apps/openmw/mwsound/headcache.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>

#include <gtest/gtest.h>

#include <components/testing/util.hpp>
#include <components/vfs/manager.hpp>
#include <components/vfs/pathutil.hpp>

namespace MWSound
{
    namespace
    {
        constexpr VFS::Path::NormalizedView sStreamed("sound/vo/a.wav");

        void appendLe(std::string& out, std::uint32_t value, std::size_t bytes)
        {
            for (std::size_t i = 0; i < bytes; ++i)
                out.push_back(static_cast<char>((value >> (8 * i)) & 0xff));
        }

        // Exercise full decoder initialization.
        std::string makeWav(std::size_t samples)
        {
            std::string wav = "RIFF";
            appendLe(wav, static_cast<std::uint32_t>(36 + samples * 2), 4);
            wav += "WAVEfmt ";
            appendLe(wav, 16, 4); // subchunk size
            appendLe(wav, 1, 2); // PCM
            appendLe(wav, 1, 2); // mono
            appendLe(wav, 8000, 4); // sample rate
            appendLe(wav, 16000, 4); // byte rate
            appendLe(wav, 2, 2); // block align
            appendLe(wav, 16, 2); // bits per sample
            wav += "data";
            appendLe(wav, static_cast<std::uint32_t>(samples * 2), 4);
            for (std::size_t i = 0; i < samples; ++i)
                appendLe(wav, static_cast<std::uint32_t>(i * 137), 2);
            return wav;
        }

        TEST(MWSoundWarmQueueTest, warmsStreamedSound)
        {
            TestingOpenMW::VFSTestFile file(makeWav(4000));
            const auto vfs = TestingOpenMW::createTestVFS({ { sStreamed, &file } });
            HeadCache cache(*vfs, 4 * 1024 * 1024);

            WarmQueue queue(*vfs, cache);
            queue.enqueue(VFS::Path::Normalized(sStreamed));

            for (int i = 0; i < 500 && !cache.contains(sStreamed); ++i)
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            EXPECT_TRUE(cache.contains(sStreamed));
        }
    }
}
