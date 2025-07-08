#include <components/misc/resourcehelpers.hpp>
#include <components/testing/util.hpp>

#include <gtest/gtest.h>

namespace
{
    using namespace Misc::ResourceHelpers;
    TEST(CorrectSoundPath, wav_files_not_overridden_with_mp3_in_vfs_are_not_corrected)
    {
        constexpr VFS::Path::NormalizedView path("sound/bar.wav");
        std::unique_ptr<VFS::Manager> mVFS = TestingOpenMW::createTestVFS({ { path, nullptr } });
        EXPECT_EQ(correctSoundPath(path, *mVFS), "sound/bar.wav");
    }

    TEST(CorrectSoundPath, wav_files_overridden_with_mp3_in_vfs_are_corrected)
    {
        constexpr VFS::Path::NormalizedView mp3("sound/foo.mp3");
        std::unique_ptr<VFS::Manager> mVFS = TestingOpenMW::createTestVFS({ { mp3, nullptr } });
        constexpr VFS::Path::NormalizedView wav("sound/foo.wav");
        EXPECT_EQ(correctSoundPath(wav, *mVFS), "sound/foo.mp3");
    }

    TEST(CorrectSoundPath, corrected_path_does_not_check_existence_in_vfs)
    {
        std::unique_ptr<VFS::Manager> mVFS = TestingOpenMW::createTestVFS({});
        constexpr VFS::Path::NormalizedView path("sound/foo.wav");
        EXPECT_EQ(correctSoundPath(path, *mVFS), "sound/foo.mp3");

        auto correctESM4SoundPath = [](auto path, auto* vfs) {
            return Misc::ResourceHelpers::correctResourcePath({ { "sound" } }, path, vfs, ".mp3");
        };

        EXPECT_EQ(correctESM4SoundPath("foo.WAV", mVFS.get()), "sound\\foo.mp3");
        EXPECT_EQ(correctESM4SoundPath("SOUND/foo.WAV", mVFS.get()), "sound\\foo.mp3");
        EXPECT_EQ(correctESM4SoundPath("DATA\\SOUND\\foo.WAV", mVFS.get()), "sound\\foo.mp3");
        EXPECT_EQ(correctESM4SoundPath("\\Data/Sound\\foo.WAV", mVFS.get()), "sound\\foo.mp3");
    }

    namespace
    {
        std::string checkChangeExtensionToDds(std::string path)
        {
            changeExtensionToDds(path);
            return path;
        }
    }

    TEST(ChangeExtensionToDds, original_extension_with_same_size_as_dds)
    {
        EXPECT_EQ(checkChangeExtensionToDds("texture/bar.tga"), "texture/bar.dds");
    }

    TEST(ChangeExtensionToDds, original_extension_greater_than_dds)
    {
        EXPECT_EQ(checkChangeExtensionToDds("texture/bar.jpeg"), "texture/bar.dds");
    }

    TEST(ChangeExtensionToDds, original_extension_smaller_than_dds)
    {
        EXPECT_EQ(checkChangeExtensionToDds("texture/bar.xx"), "texture/bar.dds");
    }

    TEST(ChangeExtensionToDds, does_not_change_dds_extension)
    {
        std::string path = "texture/bar.dds";
        EXPECT_FALSE(changeExtensionToDds(path));
    }

    TEST(ChangeExtensionToDds, does_not_change_when_no_extension)
    {
        std::string path = "texture/bar";
        EXPECT_FALSE(changeExtensionToDds(path));
    }

    TEST(ChangeExtensionToDds, change_when_there_is_an_extension)
    {
        std::string path = "texture/bar.jpeg";
        EXPECT_TRUE(changeExtensionToDds(path));
    }
}
