#include "operators.hpp"

#include <components/bsa/compressedbsafile.hpp>
#include <components/files/memorystream.hpp>
#include <components/testing/util.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>

namespace Bsa
{
    namespace
    {
        using namespace ::testing;

        struct Header
        {
            uint32_t mFormat;
            uint32_t mDirSize;
            uint32_t mFileCount;
        };

        struct Archive
        {
            Header mHeader;
            std::vector<std::uint32_t> mOffsets;
            std::vector<char> mStringBuffer;
            std::vector<BSAFile::Hash> mHashes;
            std::size_t mTailSize;
        };

        struct TestBSAFile final : public BSAFile
        {
            void readHeader(std::istream& input) override { BSAFile::readHeader(input); }

            void writeHeader() override { throw std::logic_error("TestBSAFile::writeHeader is not implemented"); }
        };

        void writeArchive(const Archive& value, std::ostream& stream)
        {
            stream.write(reinterpret_cast<const char*>(&value.mHeader), sizeof(value.mHeader));

            if (!value.mOffsets.empty())
                stream.write(reinterpret_cast<const char*>(value.mOffsets.data()),
                    value.mOffsets.size() * sizeof(std::uint32_t));

            if (!value.mStringBuffer.empty())
                stream.write(reinterpret_cast<const char*>(value.mStringBuffer.data()), value.mStringBuffer.size());

            for (const BSAFile::Hash& hash : value.mHashes)
                stream.write(reinterpret_cast<const char*>(&hash), sizeof(BSAFile::Hash));

            const std::size_t chunkSize = 4096;
            std::vector<char> chunk(chunkSize);
            for (std::size_t i = 0; i < value.mTailSize; i += chunkSize)
                stream.write(reinterpret_cast<const char*>(chunk.data()), std::min(chunk.size(), value.mTailSize - i));
        }

        std::filesystem::path makeOutputPath()
        {
            const auto testInfo = UnitTest::GetInstance()->current_test_info();
            return TestingOpenMW::outputFilePath(
                std::format("{}.{}.bsa", testInfo->test_suite_name(), testInfo->name()));
        }

        std::string makeBsaBuffer(std::uint32_t fileSize, std::uint32_t fileOffset)
        {
            std::string buffer;

            buffer.reserve(static_cast<std::size_t>(fileSize) + static_cast<std::size_t>(fileOffset) + 34);

            std::ostringstream stream(std::move(buffer));

            const Header header{
                .mFormat = static_cast<std::uint32_t>(BsaVersion::Uncompressed),
                .mDirSize = 14,
                .mFileCount = 1,
            };

            const BSAFile::Hash hash{
                .mLow = 0xaaaabbbb,
                .mHigh = 0xccccdddd,
            };

            const Archive archive{
                .mHeader = header,
                .mOffsets = { fileSize, fileOffset, 0 },
                .mStringBuffer = { 'a', '\0' },
                .mHashes = { hash },
                .mTailSize = 0,
            };

            writeArchive(archive, stream);

            return std::move(stream).str();
        }

        TEST(BSAFileTest, shouldHandleEmpty)
        {
            const std::filesystem::path path = makeOutputPath();

            {
                std::ofstream stream;
                stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
                stream.open(path, std::ios::binary);
            }

            BSAFile file;
            EXPECT_THROW(file.open(path), std::runtime_error);

            EXPECT_THAT(file.getList(), IsEmpty());
        }

        TEST(BSAFileTest, shouldHandleZeroFiles)
        {
            const std::filesystem::path path = makeOutputPath();

            {
                std::ofstream stream;
                stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

                stream.open(path, std::ios::binary);

                const Header header{
                    .mFormat = static_cast<std::uint32_t>(BsaVersion::Uncompressed),
                    .mDirSize = 0,
                    .mFileCount = 0,
                };

                const Archive archive{
                    .mHeader = header,
                    .mOffsets = {},
                    .mStringBuffer = {},
                    .mHashes = {},
                    .mTailSize = 0,
                };

                writeArchive(archive, stream);
            }

            BSAFile file;
            file.open(path);

            EXPECT_THAT(file.getList(), IsEmpty());
        }

        TEST(BSAFileTest, shouldHandleSingleFile)
        {
            const std::filesystem::path path = makeOutputPath();

            {
                std::ofstream stream;
                stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

                stream.open(path, std::ios::binary);

                const Header header{
                    .mFormat = static_cast<std::uint32_t>(BsaVersion::Uncompressed),
                    .mDirSize = 14,
                    .mFileCount = 1,
                };

                const BSAFile::Hash hash{
                    .mLow = 0xaaaabbbb,
                    .mHigh = 0xccccdddd,
                };

                const Archive archive{
                    .mHeader = header,
                    .mOffsets = { 42, 0, 0 },
                    .mStringBuffer = { 'a', '\0' },
                    .mHashes = { hash },
                    .mTailSize = 42,
                };

                writeArchive(archive, stream);
            }

            BSAFile file;
            file.open(path);

            std::vector<char> namesBuffer = { 'a', '\0' };

            EXPECT_THAT(file.getList(),
                ElementsAre(BSAFile::FileStruct{
                    .mFileSize = 42,
                    .mOffset = 34,
                    .mHash = BSAFile::Hash{ .mLow = 0xaaaabbbb, .mHigh = 0xccccdddd },
                    .mNameOffset = 0,
                    .mNameSize = 1,
                    .mNamesBuffer = &namesBuffer,
                }));
        }

        TEST(BSAFileTest, shouldHandleTwoFiles)
        {
            const std::filesystem::path path = makeOutputPath();

            {
                std::ofstream stream;
                stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

                stream.open(path, std::ios::binary);

                const std::uint32_t fileSize1 = 42;
                const std::uint32_t fileSize2 = 13;

                const Header header{
                    .mFormat = static_cast<std::uint32_t>(BsaVersion::Uncompressed),
                    .mDirSize = 28,
                    .mFileCount = 2,
                };

                const BSAFile::Hash hash1{
                    .mLow = 0xaaaabbbb,
                    .mHigh = 0xccccdddd,
                };

                const BSAFile::Hash hash2{
                    .mLow = 0x11112222,
                    .mHigh = 0x33334444,
                };

                const Archive archive{
                    .mHeader = header,
                    .mOffsets = { fileSize1, 0, fileSize2, fileSize1, 0, 2 },
                    .mStringBuffer = { 'a', '\0', 'b', '\0' },
                    .mHashes = { hash1, hash2 },
                    .mTailSize = fileSize1 + fileSize2,
                };

                writeArchive(archive, stream);
            }

            BSAFile file;
            file.open(path);

            std::vector<char> namesBuffer = { 'a', '\0', 'b', '\0' };

            EXPECT_THAT(file.getList(),
                ElementsAre(
                    BSAFile::FileStruct{
                        .mFileSize = 42,
                        .mOffset = 56,
                        .mHash = BSAFile::Hash{ .mLow = 0xaaaabbbb, .mHigh = 0xccccdddd },
                        .mNameOffset = 0,
                        .mNameSize = 1,
                        .mNamesBuffer = &namesBuffer,
                    },
                    BSAFile::FileStruct{
                        .mFileSize = 13,
                        .mOffset = 98,
                        .mHash = BSAFile::Hash{ .mLow = 0x11112222, .mHigh = 0x33334444 },
                        .mNameOffset = 2,
                        .mNameSize = 1,
                        .mNamesBuffer = &namesBuffer,
                    }));
        }

        TEST(BSAFileTest, shouldHandleSingleFileAtTheEndOfLargeFile)
        {
            constexpr std::uint32_t maxUInt32 = std::numeric_limits<uint32_t>::max();
            const std::string buffer = makeBsaBuffer(maxUInt32, maxUInt32 - 34);

            TestBSAFile file;
            // Use capacity assuming we never read beyond small header.
            Files::IMemStream stream(buffer.data(), buffer.capacity());
            file.readHeader(stream);

            std::vector<char> namesBuffer = { 'a', '\0' };

            EXPECT_THAT(file.getList(),
                ElementsAre(BSAFile::FileStruct{
                    .mFileSize = maxUInt32,
                    .mOffset = maxUInt32,
                    .mHash = BSAFile::Hash{ .mLow = 0xaaaabbbb, .mHigh = 0xccccdddd },
                    .mNameOffset = 0,
                    .mNameSize = 1,
                    .mNamesBuffer = &namesBuffer,
                }));
        }

        TEST(BSAFileTest, shouldThrowExceptionOnTooBigAbsoluteOffset)
        {
            constexpr std::uint32_t maxUInt32 = std::numeric_limits<uint32_t>::max();
            const std::string buffer = makeBsaBuffer(maxUInt32, maxUInt32 - 34 + 1);

            TestBSAFile file;
            // Use capacity assuming we never read beyond small header.
            Files::IMemStream stream(buffer.data(), buffer.capacity());
            EXPECT_THROW(file.readHeader(stream), std::runtime_error);

            EXPECT_THAT(file.getList(), IsEmpty());
        }
    }
}
