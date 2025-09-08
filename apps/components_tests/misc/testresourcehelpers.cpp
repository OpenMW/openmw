#include <components/misc/resourcehelpers.hpp>
#include <components/testing/util.hpp>

#include <gtest/gtest.h>

namespace Misc::ResourceHelpers
{
    namespace
    {
        using namespace ::testing;

        TEST(MiscResourceHelpersCorrectSoundPath, shouldKeepWavExtensionIfExistsInVfs)
        {
            constexpr VFS::Path::NormalizedView path("sound/foo.wav");
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({ { path, nullptr } });
            EXPECT_EQ(correctSoundPath(path, *vfs), "sound/foo.wav");
        }

        TEST(MiscResourceHelpersCorrectSoundPath, shouldFallbackToMp3IfWavDoesNotExistInVfs)
        {
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({});
            constexpr VFS::Path::NormalizedView path("sound/foo.wav");
            EXPECT_EQ(correctSoundPath(path, *vfs), "sound/foo.mp3");
        }

        TEST(MiscResourceHelpersCorrectSoundPath, shouldKeepWavExtensionIfBothExistsInVfs)
        {
            constexpr VFS::Path::NormalizedView wav("sound/foo.wav");
            constexpr VFS::Path::NormalizedView mp3("sound/foo.mp3");
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({
                { wav, nullptr },
                { mp3, nullptr },
            });
            EXPECT_EQ(correctSoundPath(wav, *vfs), "sound/foo.wav");
        }

        TEST(MiscResourceHelpersCorrectResourcePath, shouldFallbackToGivenExtentionIfDoesNotExistInVfs)
        {
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({});
            EXPECT_EQ(correctResourcePath({ { "sound" } }, "sound/foo.wav", vfs.get(), ".mp3"), "sound\\foo.mp3");
        }

        TEST(MiscResourceHelpersCorrectResourcePath, shouldFallbackToGivenExtentionIfBothExistInVfs)
        {
            constexpr VFS::Path::NormalizedView wav("sound/foo.wav");
            constexpr VFS::Path::NormalizedView mp3("sound/foo.mp3");
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({
                { wav, nullptr },
                { mp3, nullptr },
            });
            EXPECT_EQ(correctResourcePath({ { "sound" } }, wav.value(), vfs.get(), ".mp3"), "sound\\foo.mp3");
        }

        TEST(MiscResourceHelpersCorrectResourcePath, shouldKeepExtentionIfExistInVfs)
        {
            constexpr VFS::Path::NormalizedView wav("sound/foo.wav");
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({
                { wav, nullptr },
            });
            EXPECT_EQ(correctResourcePath({ { "sound" } }, wav.value(), vfs.get(), ".mp3"), "sound\\foo.wav");
        }

        TEST(MiscResourceHelpersCorrectResourcePath, shouldPrefixWithGivenTopDirectory)
        {
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({});
            EXPECT_EQ(correctResourcePath({ { "sound" } }, "foo.mp3", vfs.get(), ".mp3"), "sound\\foo.mp3");
        }

        TEST(MiscResourceHelpersCorrectResourcePath, shouldChangeTopDirectoryAndKeepExtensionIfOriginalExistInVfs)
        {
            constexpr VFS::Path::NormalizedView a("textures/foo.a");
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({
                { a, nullptr },
            });
            EXPECT_EQ(correctResourcePath({ { "textures", "bookart" } }, "bookart/foo.a", vfs.get(), ".b"),
                "textures\\foo.a");
        }

        TEST(MiscResourceHelpersCorrectResourcePath, shouldChangeTopDirectoryAndChangeExtensionIfFallbackExistInVfs)
        {
            constexpr VFS::Path::NormalizedView b("textures/foo.b");
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({
                { b, nullptr },
            });
            EXPECT_EQ(correctResourcePath({ { "textures", "bookart" } }, "bookart/foo.a", vfs.get(), ".b"),
                "textures\\foo.b");
        }

        TEST(MiscResourceHelpersCorrectResourcePath, shouldLowerCase)
        {
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({});
            EXPECT_EQ(correctResourcePath({ { "sound" } }, "SOUND\\Foo.MP3", vfs.get(), ".mp3"), "sound\\foo.mp3");
        }

        TEST(MiscResourceHelpersCorrectResourcePath, shouldRemoveLeadingSlash)
        {
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({});
            EXPECT_EQ(correctResourcePath({ { "sound" } }, "\\SOUND\\Foo.MP3", vfs.get(), ".mp3"), "sound\\foo.mp3");
        }

        TEST(MiscResourceHelpersCorrectResourcePath, shouldRemoveDuplicateSlashes)
        {
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({});
            EXPECT_EQ(
                correctResourcePath({ { "sound" } }, "\\\\SOUND\\\\Foo.MP3", vfs.get(), ".mp3"), "sound\\foo.mp3");
        }

        TEST(MiscResourceHelpersCorrectResourcePath, shouldConvertToBackSlash)
        {
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({});
            EXPECT_EQ(correctResourcePath({ { "sound" } }, "SOUND/Foo.MP3", vfs.get(), ".mp3"), "sound\\foo.mp3");
        }

        TEST(MiscResourceHelpersCorrectResourcePath, shouldHandlePathEqualToDirectory)
        {
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({});
            EXPECT_EQ(correctResourcePath({ { "sound" } }, "sound", vfs.get(), "mp3"), "sound\\sound");
        }

        struct MiscResourceHelpersCorrectResourcePathShouldRemoveExtraPrefix : TestWithParam<std::string>
        {
        };

        TEST_P(MiscResourceHelpersCorrectResourcePathShouldRemoveExtraPrefix, shouldMatchExpected)
        {
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({});
            EXPECT_EQ(correctResourcePath({ { "sound" } }, GetParam(), vfs.get(), ".mp3"), "sound\\foo.mp3");
        }

        const std::vector<std::string> pathsWithPrefix = {
            "data/sound/foo.mp3",
            "data/notsound/sound/foo.mp3",
            "data/soundnot/sound/foo.mp3",
            "data/notsoundnot/sound/foo.mp3",
        };

        INSTANTIATE_TEST_SUITE_P(
            PathsWithPrefix, MiscResourceHelpersCorrectResourcePathShouldRemoveExtraPrefix, ValuesIn(pathsWithPrefix));

        TEST(MiscResourceHelpersChangeExtensionToDds, original_extension_with_same_size_as_dds)
        {
            std::string path = "texture/bar.tga";
            ASSERT_TRUE(changeExtensionToDds(path));
            EXPECT_EQ(path, "texture/bar.dds");
        }

        TEST(MiscResourceHelpersChangeExtensionToDds, original_extension_greater_than_dds)
        {
            std::string path = "texture/bar.jpeg";
            ASSERT_TRUE(changeExtensionToDds(path));
            EXPECT_EQ(path, "texture/bar.dds");
        }

        TEST(MiscResourceHelpersChangeExtensionToDds, original_extension_smaller_than_dds)
        {
            std::string path = "texture/bar.xx";
            ASSERT_TRUE(changeExtensionToDds(path));
            EXPECT_EQ(path, "texture/bar.dds");
        }

        TEST(MiscResourceHelpersChangeExtensionToDds, does_not_change_dds_extension)
        {
            std::string path = "texture/bar.dds";
            EXPECT_FALSE(changeExtensionToDds(path));
            EXPECT_EQ(path, "texture/bar.dds");
        }

        TEST(MiscResourceHelpersChangeExtensionToDds, does_not_change_when_no_extension)
        {
            std::string path = "texture/bar";
            EXPECT_FALSE(changeExtensionToDds(path));
            EXPECT_EQ(path, "texture/bar");
        }
    }
}
