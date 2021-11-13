#include <gtest/gtest.h>
#include "components/misc/resourcehelpers.hpp"
#include "../lua/testing_util.hpp"

namespace
{
    using namespace Misc::ResourceHelpers;
    TEST(CorrectSoundPath, wav_files_not_overridden_with_mp3_in_vfs_are_not_corrected)
    {
        std::unique_ptr<VFS::Manager> mVFS = createTestVFS({
            {"sound/bar.wav", nullptr}
        });
        EXPECT_EQ(correctSoundPath("sound/bar.wav", mVFS.get()), "sound/bar.wav");
    }

    TEST(CorrectSoundPath, wav_files_overridden_with_mp3_in_vfs_are_corrected)
    {
        std::unique_ptr<VFS::Manager> mVFS = createTestVFS({
            {"sound/foo.mp3", nullptr}
        });
        EXPECT_EQ(correctSoundPath("sound/foo.wav", mVFS.get()), "sound/foo.mp3");
    }

    TEST(CorrectSoundPath, corrected_path_does_not_check_existence_in_vfs)
    {
        std::unique_ptr<VFS::Manager> mVFS = createTestVFS({
        });
        EXPECT_EQ(correctSoundPath("sound/foo.wav", mVFS.get()), "sound/foo.mp3");
    }

    TEST(CorrectSoundPath, correct_path_normalize_paths)
    {
        std::unique_ptr<VFS::Manager> mVFS = createTestVFS({
        });
        EXPECT_EQ(correctSoundPath("sound\\foo.wav", mVFS.get()), "sound/foo.mp3");
        EXPECT_EQ(correctSoundPath("SOUND\\foo.WAV", mVFS.get()), "SOUND/foo.mp3");
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
