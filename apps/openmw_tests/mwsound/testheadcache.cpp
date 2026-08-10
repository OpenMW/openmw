#include "apps/openmw/mwsound/headcache.hpp"

#include <cstddef>
#include <filesystem>
#include <ios>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include <components/files/istreamptr.hpp>
#include <components/testing/util.hpp>
#include <components/vfs/file.hpp>
#include <components/vfs/manager.hpp>
#include <components/vfs/pathutil.hpp>

namespace MWSound
{
    namespace
    {
        constexpr VFS::Path::NormalizedView sFile("sound/a.wav");
        constexpr VFS::Path::NormalizedView sOtherFile("sound/b.wav");

        // Counts opens, so a read served from memory can be told from one that
        // reached the file.
        class CountingFile final : public VFS::File
        {
        public:
            explicit CountingFile(std::string content)
                : mContent(std::move(content))
            {
            }

            Files::IStreamPtr open() override
            {
                ++mOpens;
                return std::make_unique<std::stringstream>(mContent, std::ios_base::in);
            }

            std::filesystem::file_time_type getLastModified() const override { return {}; }

            std::string getStem() const override { return "TestFile"; }

            int mOpens = 0;

        private:
            const std::string mContent;
        };

        std::string makeContent(std::size_t size)
        {
            std::string content(size, '\0');
            for (std::size_t i = 0; i < size; ++i)
                content[i] = static_cast<char>('a' + i % 26);
            return content;
        }

        std::string read(std::istream& stream, std::size_t size)
        {
            std::string out(size, '\0');
            stream.read(out.data(), static_cast<std::streamsize>(size));
            out.resize(static_cast<std::size_t>(stream.gcount()));
            return out;
        }

        // Fills the cache the way a first play does: stream init reads the
        // first bytes of the file, then the decoder hands the stream back.
        void warm(HeadCache& cache, VFS::Path::NormalizedView name, std::size_t initBytes, const VFS::Manager& vfs)
        {
            const Files::IStreamPtr stream = makeRecordingStream(vfs.get(name));
            read(*stream, initBytes);
            cache.insert(name, *stream);
        }

        TEST(MWSoundHeadCacheTest, shouldNotFindAnEntryForAFileNeverPlayed)
        {
            CountingFile file(makeContent(64));
            const auto vfs = TestingOpenMW::createTestVFS({ { sFile, &file } });
            HeadCache cache(*vfs, 1024);

            EXPECT_EQ(cache.lookup(sFile), nullptr);
        }

        TEST(MWSoundHeadCacheTest, shouldCacheTheHeadOfARecordingStream)
        {
            CountingFile file(makeContent(64));
            const auto vfs = TestingOpenMW::createTestVFS({ { sFile, &file } });
            HeadCache cache(*vfs, 1024);
            const Files::IStreamPtr stream = makeRecordingStream(vfs->get(sFile));
            read(*stream, 16);

            cache.insert(sFile, *stream);

            EXPECT_NE(cache.lookup(sFile), nullptr);
        }

        TEST(MWSoundHeadCacheTest, shouldServeTheHeadOfAReplayFromMemory)
        {
            const std::string content = makeContent(64);
            CountingFile file(content);
            const auto vfs = TestingOpenMW::createTestVFS({ { sFile, &file } });
            HeadCache cache(*vfs, 1024);
            warm(cache, sFile, 16, *vfs);
            const int opens = file.mOpens;

            std::shared_ptr<const HeadBuffer> buffer = cache.lookup(sFile);
            ASSERT_NE(buffer, nullptr);
            const Files::IStreamPtr stream = makeHeadStream(std::move(buffer), *vfs);

            EXPECT_EQ(read(*stream, 16), content.substr(0, 16));
            EXPECT_EQ(file.mOpens, opens);
        }

        TEST(MWSoundHeadCacheTest, shouldNotOpenTheFileWhenTheRangesCoverIt)
        {
            const std::string content = makeContent(64);
            CountingFile file(content);
            const auto vfs = TestingOpenMW::createTestVFS({ { sFile, &file } });
            HeadCache cache(*vfs, 1024);
            warm(cache, sFile, 64, *vfs);
            const int opens = file.mOpens;

            std::shared_ptr<const HeadBuffer> buffer = cache.lookup(sFile);
            ASSERT_NE(buffer, nullptr);
            const Files::IStreamPtr stream = makeHeadStream(std::move(buffer), *vfs);

            EXPECT_EQ(read(*stream, 64), content);
            EXPECT_EQ(file.mOpens, opens);
        }

        TEST(MWSoundHeadCacheTest, shouldReadPastTheHeadFromTheFile)
        {
            const std::string content = makeContent(64);
            CountingFile file(content);
            const auto vfs = TestingOpenMW::createTestVFS({ { sFile, &file } });
            HeadCache cache(*vfs, 1024);
            warm(cache, sFile, 16, *vfs);
            const int opens = file.mOpens;

            std::shared_ptr<const HeadBuffer> buffer = cache.lookup(sFile);
            ASSERT_NE(buffer, nullptr);
            const Files::IStreamPtr stream = makeHeadStream(std::move(buffer), *vfs);

            EXPECT_EQ(read(*stream, 64), content);
            EXPECT_EQ(file.mOpens, opens + 1);
        }

        TEST(MWSoundHeadCacheTest, shouldCacheTheSuffixRangeReadByInit)
        {
            const std::string content = makeContent(64);
            CountingFile file(content);
            const auto vfs = TestingOpenMW::createTestVFS({ { sFile, &file } });
            HeadCache cache(*vfs, 1024);
            {
                const Files::IStreamPtr init = makeRecordingStream(vfs->get(sFile));
                read(*init, 8);
                init->seekg(-8, std::ios_base::end);
                read(*init, 8);
                cache.insert(sFile, *init);
            }
            const int opens = file.mOpens;

            std::shared_ptr<const HeadBuffer> buffer = cache.lookup(sFile);
            ASSERT_NE(buffer, nullptr);
            const Files::IStreamPtr stream = makeHeadStream(std::move(buffer), *vfs);

            EXPECT_EQ(read(*stream, 8), content.substr(0, 8));
            stream->seekg(-8, std::ios_base::end);
            EXPECT_EQ(read(*stream, 8), content.substr(56, 8));
            EXPECT_EQ(file.mOpens, opens);
        }

        TEST(MWSoundHeadCacheTest, shouldAnswerSeeksFromTheRecordedFileSize)
        {
            const std::string content = makeContent(64);
            CountingFile file(content);
            const auto vfs = TestingOpenMW::createTestVFS({ { sFile, &file } });
            HeadCache cache(*vfs, 1024);
            warm(cache, sFile, 64, *vfs);

            std::shared_ptr<const HeadBuffer> buffer = cache.lookup(sFile);
            ASSERT_NE(buffer, nullptr);
            const Files::IStreamPtr stream = makeHeadStream(std::move(buffer), *vfs);

            stream->seekg(0, std::ios_base::end);
            EXPECT_EQ(stream->tellg(), std::streampos(64));
            stream->seekg(65);
            EXPECT_TRUE(stream->fail());
        }

        TEST(MWSoundHeadCacheTest, shouldEvictTheLeastRecentlyUsedEntry)
        {
            const std::string content = makeContent(32);
            CountingFile file(content);
            CountingFile otherFile(content);
            const auto vfs = TestingOpenMW::createTestVFS({ { sFile, &file }, { sOtherFile, &otherFile } });
            HeadCache cache(*vfs, 40);
            warm(cache, sFile, 32, *vfs);

            warm(cache, sOtherFile, 32, *vfs);

            EXPECT_EQ(cache.lookup(sFile), nullptr);
            EXPECT_NE(cache.lookup(sOtherFile), nullptr);
        }

        TEST(MWSoundHeadCacheTest, shouldKeepServingAnOpenStreamAfterItsEntryIsEvicted)
        {
            const std::string content = makeContent(32);
            CountingFile file(content);
            CountingFile otherFile(content);
            const auto vfs = TestingOpenMW::createTestVFS({ { sFile, &file }, { sOtherFile, &otherFile } });
            HeadCache cache(*vfs, 40);
            warm(cache, sFile, 32, *vfs);
            std::shared_ptr<const HeadBuffer> buffer = cache.lookup(sFile);
            ASSERT_NE(buffer, nullptr);
            const Files::IStreamPtr stream = makeHeadStream(std::move(buffer), *vfs);
            warm(cache, sOtherFile, 32, *vfs);
            const int opens = file.mOpens;

            EXPECT_EQ(read(*stream, 32), content);
            EXPECT_EQ(file.mOpens, opens);
        }

        TEST(MWSoundHeadCacheTest, shouldCacheNothingWithAZeroBudget)
        {
            const std::string content = makeContent(32);
            CountingFile file(content);
            const auto vfs = TestingOpenMW::createTestVFS({ { sFile, &file } });
            HeadCache cache(*vfs, 0);

            warm(cache, sFile, 32, *vfs);

            EXPECT_EQ(cache.lookup(sFile), nullptr);
        }

        TEST(MWSoundHeadCacheTest, shouldRejectAStreamItDidNotRecord)
        {
            CountingFile file(makeContent(64));
            const auto vfs = TestingOpenMW::createTestVFS({ { sFile, &file } });
            HeadCache cache(*vfs, 1024);
            const Files::IStreamPtr stream = vfs->get(sFile);

            EXPECT_THROW(cache.insert(sFile, *stream), std::invalid_argument);
        }

        TEST(MWSoundHeadCacheTest, shouldNotCacheAHeadOverTheCeiling)
        {
            const std::string content = makeContent(300 * 1024);
            CountingFile file(content);
            const auto vfs = TestingOpenMW::createTestVFS({ { sFile, &file } });
            HeadCache cache(*vfs, 1024 * 1024);

            warm(cache, sFile, content.size(), *vfs);

            EXPECT_EQ(cache.lookup(sFile), nullptr);
        }
    }
}
