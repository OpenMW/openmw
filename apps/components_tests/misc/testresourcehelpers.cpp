#include <components/esm/gameformat.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/testing/util.hpp>

#include <gtest/gtest.h>

namespace Misc::ResourceHelpers
{
    namespace
    {
        using namespace ::testing;

        constexpr VFS::Path::NormalizedView sound("sound");
        constexpr VFS::Path::NormalizedView textures("textures");
        constexpr VFS::Path::NormalizedView bookart("bookart");
        constexpr VFS::Path::ExtensionView mp3("mp3");
        constexpr VFS::Path::ExtensionView b("b");

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
            constexpr VFS::Path::NormalizedView wavPath("sound/foo.wav");
            constexpr VFS::Path::NormalizedView mp3Path("sound/foo.mp3");
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({
                { wavPath, nullptr },
                { mp3Path, nullptr },
            });
            EXPECT_EQ(correctSoundPath(wavPath, *vfs), "sound/foo.wav");
        }

        TEST(MiscResourceHelpersCorrectResourcePath, shouldFallbackToGivenExtensionIfDoesNotExistInVfs)
        {
            constexpr VFS::Path::NormalizedView path("sound/foo.wav");
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({});
            EXPECT_EQ(correctResourcePath({ { sound } }, path, *vfs, mp3), "sound/foo.mp3");
        }

        TEST(MiscResourceHelpersCorrectResourcePath, shouldFallbackToGivenExtensionIfBothExistInVfs)
        {
            constexpr VFS::Path::NormalizedView wavPath("sound/foo.wav");
            constexpr VFS::Path::NormalizedView mp3Path("sound/foo.mp3");
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({
                { wavPath, nullptr },
                { mp3Path, nullptr },
            });
            EXPECT_EQ(correctResourcePath({ { sound } }, wavPath, *vfs, mp3), "sound/foo.mp3");
        }

        TEST(MiscResourceHelpersCorrectResourcePath, shouldKeepExtensionIfExistInVfs)
        {
            constexpr VFS::Path::NormalizedView wavPath("sound/foo.wav");
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({
                { wavPath, nullptr },
            });
            EXPECT_EQ(correctResourcePath({ { sound } }, wavPath, *vfs, mp3), "sound/foo.wav");
        }

        TEST(MiscResourceHelpersCorrectResourcePath, shouldPrefixWithGivenTopDirectory)
        {
            constexpr VFS::Path::NormalizedView path("foo.mp3");
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({});
            EXPECT_EQ(correctResourcePath({ { sound } }, path, *vfs, mp3), "sound/foo.mp3");
        }

        TEST(MiscResourceHelpersCorrectResourcePath, shouldChangeTopDirectoryAndKeepExtensionIfOriginalExistInVfs)
        {
            constexpr VFS::Path::NormalizedView path("bookart/foo.a");
            constexpr VFS::Path::NormalizedView aPath("textures/foo.a");
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({
                { aPath, nullptr },
            });
            EXPECT_EQ(correctResourcePath({ { textures, bookart } }, path, *vfs, b), "textures/foo.a");
        }

        TEST(MiscResourceHelpersCorrectResourcePath, shouldChangeTopDirectoryAndChangeExtensionIfFallbackExistInVfs)
        {
            constexpr VFS::Path::NormalizedView path("bookart/foo.a");
            constexpr VFS::Path::NormalizedView bPath("textures/foo.b");
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({
                { bPath, nullptr },
            });
            EXPECT_EQ(correctResourcePath({ { textures, bookart } }, path, *vfs, b), "textures/foo.b");
        }

        TEST(MiscResourceHelpersCorrectResourcePath, shouldHandlePathEqualToDirectory)
        {
            constexpr VFS::Path::NormalizedView path("sound");
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({});
            EXPECT_EQ(correctResourcePath({ { sound } }, path, *vfs, mp3), "sound/sound");
        }

        struct MiscResourceHelpersCorrectResourcePathShouldRemoveExtraPrefix : TestWithParam<VFS::Path::NormalizedView>
        {
        };

        TEST_P(MiscResourceHelpersCorrectResourcePathShouldRemoveExtraPrefix, shouldMatchExpected)
        {
            const std::unique_ptr<const VFS::Manager> vfs = TestingOpenMW::createTestVFS({});
            EXPECT_EQ(correctResourcePath({ { sound } }, GetParam(), *vfs, mp3), "sound/foo.mp3");
        }

        const std::vector<VFS::Path::NormalizedView> pathsWithPrefix = {
            VFS::Path::NormalizedView("data/sound/foo.mp3"),
            VFS::Path::NormalizedView("data/notsound/sound/foo.mp3"),
            VFS::Path::NormalizedView("data/soundnot/sound/foo.mp3"),
            VFS::Path::NormalizedView("data/notsoundnot/sound/foo.mp3"),
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

        struct MiscResourceHelpersGetLODMeshName : Test
        {
            const std::unique_ptr<const VFS::Manager> mVFS = TestingOpenMW::createTestVFS({
                { VFS::Path::NormalizedView("meshes/foo_dist.nif"), nullptr },
                { VFS::Path::NormalizedView("meshes/foo_far.nif"), nullptr },
                { VFS::Path::NormalizedView("meshes/foo_lod.nif"), nullptr },
                { VFS::Path::NormalizedView("meshes/foo_lod_1.nif"), nullptr },
            });
            const VFS::Path::NormalizedView mPath{ "meshes/foo.nif" };
        };

        TEST_F(MiscResourceHelpersGetLODMeshName, shouldUseGamePattern)
        {
            EXPECT_EQ(getLODMeshName(ESM::GameFormat::TES3, mPath, *mVFS), "meshes/foo_dist.nif");
            EXPECT_EQ(getLODMeshName(ESM::GameFormat::TES4, mPath, *mVFS), "meshes/foo_far.nif");
            EXPECT_EQ(getLODMeshName(ESM::GameFormat::FONV, mPath, *mVFS), "meshes/foo_lod.nif");
            EXPECT_EQ(getLODMeshName(ESM::GameFormat::FO4, mPath, *mVFS), "meshes/foo_lod.nif");
        }

        TEST_F(MiscResourceHelpersGetLODMeshName, shouldPreferNumberedLevelAtOrBelowRequested)
        {
            EXPECT_EQ(getLODMeshName(ESM::GameFormat::TES5, mPath, *mVFS, 2), "meshes/foo_lod_1.nif");
            EXPECT_EQ(getLODMeshName(ESM::GameFormat::TES5, mPath, *mVFS, 0), "meshes/foo_lod.nif");
        }

        TEST_F(MiscResourceHelpersGetLODMeshName, shouldKeepPathForUnknownFormat)
        {
            EXPECT_EQ(getLODMeshName(ESM::GameFormat::Unknown, mPath, *mVFS), "meshes/foo.nif");
        }
    }
}
