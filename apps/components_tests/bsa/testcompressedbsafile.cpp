#include "operators.hpp"

#include <components/bsa/compressedbsafile.hpp>
#include <components/testing/util.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>

namespace Bsa
{
    namespace
    {
        using namespace ::testing;

        struct FileRecord
        {
            std::uint64_t mHash;
            std::uint32_t mSize;
            std::uint32_t mOffset;
            std::string mName;
        };

        struct NonSSEFolderRecord
        {
            std::uint64_t mHash;
            std::uint32_t mCount;
            std::int32_t mOffset;
            std::string mName;
            std::vector<FileRecord> mFiles;
        };

        struct Archive
        {
            CompressedBSAFile::Header mHeader;
            std::vector<NonSSEFolderRecord> mFolders;
        };

        void writeArchive(const Archive& value, std::ostream& stream)
        {
            stream.write(reinterpret_cast<const char*>(&value.mHeader), sizeof(value.mHeader));

            for (const NonSSEFolderRecord& folder : value.mFolders)
            {
                stream.write(reinterpret_cast<const char*>(&folder.mHash), sizeof(folder.mHash));
                stream.write(reinterpret_cast<const char*>(&folder.mCount), sizeof(folder.mCount));
                stream.write(reinterpret_cast<const char*>(&folder.mOffset), sizeof(folder.mOffset));
            }

            for (const NonSSEFolderRecord& folder : value.mFolders)
            {
                const std::uint8_t folderNameSize = static_cast<std::uint8_t>(folder.mName.size() + 1);

                stream.write(reinterpret_cast<const char*>(&folderNameSize), sizeof(folderNameSize));
                stream.write(reinterpret_cast<const char*>(folder.mName.data()), folder.mName.size());
                stream.put('\0');

                for (const FileRecord& file : folder.mFiles)
                {
                    stream.write(reinterpret_cast<const char*>(&file.mHash), sizeof(file.mHash));
                    stream.write(reinterpret_cast<const char*>(&file.mSize), sizeof(file.mSize));
                    stream.write(reinterpret_cast<const char*>(&file.mOffset), sizeof(file.mOffset));
                }
            }

            for (const NonSSEFolderRecord& folder : value.mFolders)
            {
                for (const FileRecord& file : folder.mFiles)
                {
                    stream.write(reinterpret_cast<const char*>(file.mName.data()), file.mName.size());
                    stream.put('\0');
                }
            }
        }

        std::filesystem::path makeOutputPath()
        {
            const auto testInfo = UnitTest::GetInstance()->current_test_info();
            return TestingOpenMW::outputFilePath(
                std::format("{}.{}.bsa", testInfo->test_suite_name(), testInfo->name()));
        }

        TEST(CompressedBSAFileTest, shouldHandleEmpty)
        {
            const std::filesystem::path path = makeOutputPath();

            {
                std::ofstream stream;
                stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
                stream.open(path, std::ios::binary);
            }

            CompressedBSAFile file;
            EXPECT_THROW(file.open(path), std::runtime_error);

            EXPECT_THAT(file.getList(), IsEmpty());
        }

        TEST(CompressedBSAFileTest, shouldHandleSingleFile)
        {
            const std::filesystem::path path = makeOutputPath();

            {
                std::ofstream stream;
                stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

                stream.open(path, std::ios::binary);

                const CompressedBSAFile::Header header{
                    .mFormat = static_cast<std::uint32_t>(BsaVersion::Compressed),
                    .mVersion = CompressedBSAFile::Version_TES4,
                    .mFoldersOffset = sizeof(CompressedBSAFile::Header),
                    .mFlags = CompressedBSAFile::ArchiveFlag_FolderNames | CompressedBSAFile::ArchiveFlag_FileNames,
                    .mFolderCount = 1,
                    .mFileCount = 1,
                    .mFolderNamesLength = 7,
                    .mFileNamesLength = 9,
                    .mFileFlags = 0,
                };

                const FileRecord file{
                    .mHash = 0xfedcba9876543210,
                    .mSize = 42,
                    .mOffset = 0,
                    .mName = "filename",
                };

                const NonSSEFolderRecord folder{
                    .mHash = 0xfedcba9876543210,
                    .mCount = 1,
                    .mOffset = 0,
                    .mName = "folder",
                    .mFiles = { file },
                };

                const Archive archive{
                    .mHeader = header,
                    .mFolders = { folder },
                };

                writeArchive(archive, stream);
            }

            CompressedBSAFile file;
            file.open(path);

            std::vector<char> namesBuffer;
            constexpr std::string_view filePath = "folder\\filename";
            namesBuffer.assign(filePath.begin(), filePath.end());
            namesBuffer.push_back('\0');

            EXPECT_THAT(file.getList(),
                ElementsAre(BSAFile::FileStruct{
                    .mFileSize = 42,
                    .mOffset = 0,
                    .mHash = BSAFile::Hash{ .mLow = 0, .mHigh = 0 },
                    .mNameOffset = 0,
                    .mNameSize = 15,
                    .mNamesBuffer = &namesBuffer,
                }));
        }

        TEST(CompressedBSAFileTest, shouldHandleEmptyFileName)
        {
            const std::filesystem::path path = makeOutputPath();

            {
                std::ofstream stream;
                stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

                stream.open(path, std::ios::binary);

                const CompressedBSAFile::Header header{
                    .mFormat = static_cast<std::uint32_t>(BsaVersion::Compressed),
                    .mVersion = CompressedBSAFile::Version_TES4,
                    .mFoldersOffset = sizeof(CompressedBSAFile::Header),
                    .mFlags = CompressedBSAFile::ArchiveFlag_FolderNames | CompressedBSAFile::ArchiveFlag_FileNames,
                    .mFolderCount = 1,
                    .mFileCount = 1,
                    .mFolderNamesLength = 7,
                    .mFileNamesLength = 1,
                    .mFileFlags = 0,
                };

                const FileRecord file{
                    .mHash = 0xfedcba9876543210,
                    .mSize = 42,
                    .mOffset = 0,
                    .mName = "",
                };

                const NonSSEFolderRecord folder{
                    .mHash = 0xfedcba9876543210,
                    .mCount = 1,
                    .mOffset = 0,
                    .mName = "folder",
                    .mFiles = { file },
                };

                const Archive archive{
                    .mHeader = header,
                    .mFolders = { folder },
                };

                writeArchive(archive, stream);
            }

            CompressedBSAFile file;
            EXPECT_THROW(file.open(path), std::runtime_error);
        }

        TEST(CompressedBSAFileTest, shouldHandleFoldersWithDuplicateHash)
        {
            const std::filesystem::path path = makeOutputPath();

            {
                std::ofstream stream;
                stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

                stream.open(path, std::ios::binary);

                const CompressedBSAFile::Header header{
                    .mFormat = static_cast<std::uint32_t>(BsaVersion::Compressed),
                    .mVersion = CompressedBSAFile::Version_TES4,
                    .mFoldersOffset = sizeof(CompressedBSAFile::Header),
                    .mFlags = CompressedBSAFile::ArchiveFlag_FolderNames | CompressedBSAFile::ArchiveFlag_FileNames,
                    .mFolderCount = 2,
                    .mFileCount = 2,
                    .mFolderNamesLength = 16,
                    .mFileNamesLength = 18,
                    .mFileFlags = 0,
                };

                const FileRecord file{
                    .mHash = 0xfedcba9876543210,
                    .mSize = 42,
                    .mOffset = 0,
                    .mName = "filename",
                };

                const NonSSEFolderRecord folder1{
                    .mHash = 0xfedcba9876543210,
                    .mCount = 1,
                    .mOffset = 0,
                    .mName = "folder1",
                    .mFiles = { file },
                };

                const NonSSEFolderRecord folder2{
                    .mHash = 0xfedcba9876543210,
                    .mCount = 1,
                    .mOffset = 0,
                    .mName = "folder2",
                    .mFiles = { file },
                };

                const Archive archive{
                    .mHeader = header,
                    .mFolders = { folder1, folder2 },
                };

                writeArchive(archive, stream);
            }

            CompressedBSAFile file;
            file.open(path);

            std::vector<char> namesBuffer;
            constexpr std::string_view filePath = "folder2\\filename";
            namesBuffer.assign(filePath.begin(), filePath.end());
            namesBuffer.push_back('\0');

            EXPECT_THAT(file.getList(),
                ElementsAre(BSAFile::FileStruct{
                    .mFileSize = 42,
                    .mOffset = 0,
                    .mHash = BSAFile::Hash{ .mLow = 0, .mHigh = 0 },
                    .mNameOffset = 0,
                    .mNameSize = 16,
                    .mNamesBuffer = &namesBuffer,
                }));
        }

        TEST(CompressedBSAFileTest, shouldHandleFilesWithDuplicateHash)
        {
            const std::filesystem::path path = makeOutputPath();

            {
                std::ofstream stream;
                stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

                stream.open(path, std::ios::binary);

                const CompressedBSAFile::Header header{
                    .mFormat = static_cast<std::uint32_t>(BsaVersion::Compressed),
                    .mVersion = CompressedBSAFile::Version_TES4,
                    .mFoldersOffset = sizeof(CompressedBSAFile::Header),
                    .mFlags = CompressedBSAFile::ArchiveFlag_FolderNames | CompressedBSAFile::ArchiveFlag_FileNames,
                    .mFolderCount = 1,
                    .mFileCount = 2,
                    .mFolderNamesLength = 9,
                    .mFileNamesLength = 18,
                    .mFileFlags = 0,
                };

                const FileRecord file1{
                    .mHash = 0xfedcba9876543210,
                    .mSize = 42,
                    .mOffset = 0,
                    .mName = "filename1",
                };

                const FileRecord file2{
                    .mHash = 0xfedcba9876543210,
                    .mSize = 13,
                    .mOffset = 0,
                    .mName = "filename2",
                };

                const NonSSEFolderRecord folder{
                    .mHash = 0xfedcba9876543210,
                    .mCount = 2,
                    .mOffset = 0,
                    .mName = "folder",
                    .mFiles = { file1, file2 },
                };

                const Archive archive{
                    .mHeader = header,
                    .mFolders = { folder },
                };

                writeArchive(archive, stream);
            }

            CompressedBSAFile file;
            file.open(path);

            std::vector<char> namesBuffer;
            constexpr std::string_view filePath = "folder\\filename2";
            namesBuffer.assign(filePath.begin(), filePath.end());
            namesBuffer.push_back('\0');

            EXPECT_THAT(file.getList(),
                ElementsAre(BSAFile::FileStruct{
                    .mFileSize = 13,
                    .mOffset = 0,
                    .mHash = BSAFile::Hash{ .mLow = 0, .mHigh = 0 },
                    .mNameOffset = 0,
                    .mNameSize = 16,
                    .mNamesBuffer = &namesBuffer,
                }));
        }
    }
}
