#include "ba2dx10file.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <format>
#include <istream>

#include <zlib.h>

#include <components/esm/fourcc.hpp>
#include <components/files/constrainedfilestream.hpp>
#include <components/files/utils.hpp>
#include <components/vfs/pathutil.hpp>

#include "ba2file.hpp"
#include "memorystream.hpp"

namespace Bsa
{
    BA2DX10File::BA2DX10File() {}

    BA2DX10File::~BA2DX10File() = default;

    void BA2DX10File::loadFiles(uint32_t fileCount, std::istream& in)
    {
        mFiles.clear();
        mFiles.reserve(fileCount);
        for (uint32_t i = 0; i < fileCount; ++i)
        {
            uint32_t nameHash, extHash, dirHash;
            in.read(reinterpret_cast<char*>(&nameHash), sizeof(uint32_t));
            in.read(reinterpret_cast<char*>(&extHash), sizeof(uint32_t));
            in.read(reinterpret_cast<char*>(&dirHash), sizeof(uint32_t));

            FileRecord file;
            uint8_t unknown;
            in.read(reinterpret_cast<char*>(&unknown), sizeof(uint8_t));

            uint8_t nbChunks;
            in.read(reinterpret_cast<char*>(&nbChunks), sizeof(uint8_t));

            file.mTextureChunks.resize(nbChunks);

            uint16_t chunkHeaderSize;
            in.read(reinterpret_cast<char*>(&chunkHeaderSize), sizeof(uint16_t));
            if (chunkHeaderSize != 24)
                fail("Corrupted BSA");

            in.read(reinterpret_cast<char*>(&file.mHeight), sizeof(uint16_t));
            in.read(reinterpret_cast<char*>(&file.mWidth), sizeof(uint16_t));
            in.read(reinterpret_cast<char*>(&file.mNumMips), sizeof(uint8_t));
            in.read(reinterpret_cast<char*>(&file.mDXGIFormat), sizeof(uint8_t));
            in.read(reinterpret_cast<char*>(&file.mCubeMaps), sizeof(uint16_t));
            for (auto& texture : file.mTextureChunks)
            {
                in.read(reinterpret_cast<char*>(&texture.mOffset), sizeof(int64_t));
                in.read(reinterpret_cast<char*>(&texture.mPackedSize), sizeof(uint32_t));
                in.read(reinterpret_cast<char*>(&texture.mSize), sizeof(uint32_t));
                in.read(reinterpret_cast<char*>(&texture.mStartMip), sizeof(uint16_t));
                in.read(reinterpret_cast<char*>(&texture.mEndMip), sizeof(uint16_t));
                uint32_t baadfood;
                in.read(reinterpret_cast<char*>(&baadfood), sizeof(uint32_t));
                if (baadfood != 0xBAADF00D)
                    fail("Corrupted BSA");
            }

            mFolders[dirHash][{ nameHash, extHash }] = std::move(file);

            FileStruct fileStruct{};
            mFiles.push_back(fileStruct);
        }
    }

    /// Read header information from the input source
    void BA2DX10File::readHeader(std::istream& input)
    {
        assert(!mIsLoaded);

        const std::streamsize fsize = Files::getStreamSizeLeft(input);

        if (fsize < 24) // header is 24 bytes
            fail("File too small to be a valid BSA archive");

        // Get essential header numbers
        uint32_t type, fileCount;
        uint64_t fileTableOffset;
        {
            uint32_t header[4];
            input.read(reinterpret_cast<char*>(header), 16);
            input.read(reinterpret_cast<char*>(&fileTableOffset), 8);

            if (header[0] != ESM::fourCC("BTDX"))
                fail("Unrecognized BA2 signature");
            mVersion = header[1];
            switch (static_cast<BA2Version>(mVersion))
            {
                case BA2Version::Fallout4:
                case BA2Version::Fallout4NextGen_v7:
                case BA2Version::Fallout4NextGen_v8:
                    break;
                case BA2Version::StarfieldDDS:
                    uint64_t dummy;
                    input.read(reinterpret_cast<char*>(&dummy), 8);
                    uint32_t compressionMethod;
                    input.read(reinterpret_cast<char*>(&compressionMethod), 4);
                    if (compressionMethod == 3)
                        fail("Unsupported LZ4-compressed DDS BA2");
                    break;
                default:
                    fail("Unrecognized DDS BA2 version");
            }

            type = header[2];
            fileCount = header[3];
        }

        if (type == ESM::fourCC("DX10"))
            loadFiles(fileCount, input);
        else
            fail("Unrecognized ba2 version type");

        // Read the string table
        input.seekg(fileTableOffset);
        for (uint32_t i = 0; i < fileCount; ++i)
        {
            std::vector<char> fileName;
            uint16_t fileNameSize;
            input.read(reinterpret_cast<char*>(&fileNameSize), sizeof(uint16_t));
            fileName.resize(fileNameSize + 1);
            input.read(fileName.data(), fileNameSize);
            mFileNames.push_back(std::move(fileName));
            mFiles[i].mNameOffset = 0;
            mFiles[i].mNameSize = fileNameSize;
            mFiles[i].mNamesBuffer = &mFileNames.back();
        }
    }

    std::optional<BA2DX10File::FileRecord> BA2DX10File::getFileRecord(std::string_view str) const
    {
        for (const auto c : str)
        {
            if (((static_cast<unsigned>(c) >> 7U) & 1U) != 0U)
            {
                fail(std::format("File record {} contains unicode characters, refusing to load.", str));
            }
        }

        const VFS::Path::Normalized path(str);

        const std::string_view fileName = path.stem();
        const std::string_view folder = path.parent().value();

        uint32_t folderHash = generateHash(folder);
        auto it = mFolders.find(folderHash);
        if (it == mFolders.end())
            return std::nullopt; // folder not found

        uint32_t fileHash = generateHash(fileName);
        uint32_t extHash = generateExtensionHash(path.extension().value());
        auto iter = it->second.find({ fileHash, extHash });
        if (iter == it->second.end())
            return std::nullopt; // file not found
        return iter->second;
    }

#pragma pack(push)
#pragma pack(1)
    struct DDSHeader
    {
        uint32_t mSize = 0;
        uint32_t mFlags = 0;
        uint32_t mHeight = 0;
        uint32_t mWidth = 0;
        uint32_t mPitchOrLinearSize = 0;
        uint32_t mDepth = 0;
        uint32_t mMipMapCount = 0;
        uint32_t mReserved1[11] = {};
        struct
        {
            uint32_t mSize = 0;
            uint32_t mFlags = 0;
            uint32_t mFourCC = 0;
            uint32_t mRGBBitCount = 0;
            uint32_t mRBitMask = 0;
            uint32_t mGBitMask = 0;
            uint32_t mBBitMask = 0;
            uint32_t mABitMask = 0;
        } mPixelFormat;
        uint32_t mCaps = 0;
        uint32_t mCaps2 = 0;
        uint32_t mCaps3 = 0;
        uint32_t mCaps4 = 0;
        uint32_t mReserved2 = 0;
    };

    struct DDSHeaderDX10 : DDSHeader
    {
        int32_t mDXGIFormat = 0;
        uint32_t mResourceDimension = 0;
        uint32_t mMiscFlags = 0;
        uint32_t mArraySize = 0;
        uint32_t mMiscFlags2 = 0;
    };
#pragma pack(pop)

    Files::IStreamPtr BA2DX10File::getFile(const FileStruct* file)
    {
        if (auto fileRec = getFileRecord(file->name()); fileRec)
            return getFile(*fileRec);
        fail("File not found: " + std::string(file->name()));
    }

    void BA2DX10File::addFile(const std::string& filename, std::istream& file)
    {
        assert(false); // not implemented yet
        fail("Add file is not implemented for compressed BSA: " + filename);
    }

    constexpr const uint32_t DDSD_CAPS = 0x00000001;
    constexpr const uint32_t DDSD_HEIGHT = 0x00000002;
    constexpr const uint32_t DDSD_WIDTH = 0x00000004;
    constexpr const uint32_t DDSD_PITCH = 0x00000008;
    constexpr const uint32_t DDSD_PIXELFORMAT = 0x00001000;
    constexpr const uint32_t DDSD_MIPMAPCOUNT = 0x00020000;
    constexpr const uint32_t DDSD_LINEARSIZE = 0x00080000;

    constexpr const uint32_t DDSCAPS_COMPLEX = 0x00000008;
    constexpr const uint32_t DDSCAPS_TEXTURE = 0x00001000;
    constexpr const uint32_t DDSCAPS_MIPMAP = 0x00400000;

    constexpr const uint32_t DDSCAPS2_CUBEMAP = 0x00000200;
    constexpr const uint32_t DDSCAPS2_POSITIVEX = 0x00000400;
    constexpr const uint32_t DDSCAPS2_NEGATIVEX = 0x00000800;
    constexpr const uint32_t DDSCAPS2_POSITIVEY = 0x00001000;
    constexpr const uint32_t DDSCAPS2_NEGATIVEY = 0x00002000;
    constexpr const uint32_t DDSCAPS2_POSITIVEZ = 0x00004000;
    constexpr const uint32_t DDSCAPS2_NEGATIVEZ = 0x00008000;

    constexpr const uint32_t DDPF_ALPHAPIXELS = 0x00000001;
    constexpr const uint32_t DDPF_ALPHA = 0x00000002;
    constexpr const uint32_t DDPF_FOURCC = 0x00000004;
    constexpr const uint32_t DDPF_RGB = 0x00000040;
    constexpr const uint32_t DDPF_LUMINANCE = 0x00020000;

    constexpr const uint32_t DDS_DIMENSION_TEXTURE2D = 0x00000003;
    constexpr const uint32_t DDS_RESOURCE_MISC_TEXTURECUBE = 0x00000004;

    enum DXGI : uint8_t
    {
        DXGI_FORMAT_UNKNOWN = 0,
        DXGI_FORMAT_R32G32B32A32_TYPELESS,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_UINT,
        DXGI_FORMAT_R32G32B32A32_SINT,
        DXGI_FORMAT_R32G32B32_TYPELESS,
        DXGI_FORMAT_R32G32B32_FLOAT,
        DXGI_FORMAT_R32G32B32_UINT,
        DXGI_FORMAT_R32G32B32_SINT,
        DXGI_FORMAT_R16G16B16A16_TYPELESS,
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        DXGI_FORMAT_R16G16B16A16_UNORM,
        DXGI_FORMAT_R16G16B16A16_UINT,
        DXGI_FORMAT_R16G16B16A16_SNORM,
        DXGI_FORMAT_R16G16B16A16_SINT,
        DXGI_FORMAT_R32G32_TYPELESS,
        DXGI_FORMAT_R32G32_FLOAT,
        DXGI_FORMAT_R32G32_UINT,
        DXGI_FORMAT_R32G32_SINT,
        DXGI_FORMAT_R32G8X24_TYPELESS,
        DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
        DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,
        DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,
        DXGI_FORMAT_R10G10B10A2_TYPELESS,
        DXGI_FORMAT_R10G10B10A2_UNORM,
        DXGI_FORMAT_R10G10B10A2_UINT,
        DXGI_FORMAT_R11G11B10_FLOAT,
        DXGI_FORMAT_R8G8B8A8_TYPELESS,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        DXGI_FORMAT_R8G8B8A8_UINT,
        DXGI_FORMAT_R8G8B8A8_SNORM,
        DXGI_FORMAT_R8G8B8A8_SINT,
        DXGI_FORMAT_R16G16_TYPELESS,
        DXGI_FORMAT_R16G16_FLOAT,
        DXGI_FORMAT_R16G16_UNORM,
        DXGI_FORMAT_R16G16_UINT,
        DXGI_FORMAT_R16G16_SNORM,
        DXGI_FORMAT_R16G16_SINT,
        DXGI_FORMAT_R32_TYPELESS,
        DXGI_FORMAT_D32_FLOAT,
        DXGI_FORMAT_R32_FLOAT,
        DXGI_FORMAT_R32_UINT,
        DXGI_FORMAT_R32_SINT,
        DXGI_FORMAT_R24G8_TYPELESS,
        DXGI_FORMAT_D24_UNORM_S8_UINT,
        DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
        DXGI_FORMAT_X24_TYPELESS_G8_UINT,
        DXGI_FORMAT_R8G8_TYPELESS,
        DXGI_FORMAT_R8G8_UNORM,
        DXGI_FORMAT_R8G8_UINT,
        DXGI_FORMAT_R8G8_SNORM,
        DXGI_FORMAT_R8G8_SINT,
        DXGI_FORMAT_R16_TYPELESS,
        DXGI_FORMAT_R16_FLOAT,
        DXGI_FORMAT_D16_UNORM,
        DXGI_FORMAT_R16_UNORM,
        DXGI_FORMAT_R16_UINT,
        DXGI_FORMAT_R16_SNORM,
        DXGI_FORMAT_R16_SINT,
        DXGI_FORMAT_R8_TYPELESS,
        DXGI_FORMAT_R8_UNORM,
        DXGI_FORMAT_R8_UINT,
        DXGI_FORMAT_R8_SNORM,
        DXGI_FORMAT_R8_SINT,
        DXGI_FORMAT_A8_UNORM,
        DXGI_FORMAT_R1_UNORM,
        DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
        DXGI_FORMAT_R8G8_B8G8_UNORM,
        DXGI_FORMAT_G8R8_G8B8_UNORM,
        DXGI_FORMAT_BC1_TYPELESS,
        DXGI_FORMAT_BC1_UNORM,
        DXGI_FORMAT_BC1_UNORM_SRGB,
        DXGI_FORMAT_BC2_TYPELESS,
        DXGI_FORMAT_BC2_UNORM,
        DXGI_FORMAT_BC2_UNORM_SRGB,
        DXGI_FORMAT_BC3_TYPELESS,
        DXGI_FORMAT_BC3_UNORM,
        DXGI_FORMAT_BC3_UNORM_SRGB,
        DXGI_FORMAT_BC4_TYPELESS,
        DXGI_FORMAT_BC4_UNORM,
        DXGI_FORMAT_BC4_SNORM,
        DXGI_FORMAT_BC5_TYPELESS,
        DXGI_FORMAT_BC5_UNORM,
        DXGI_FORMAT_BC5_SNORM,
        DXGI_FORMAT_B5G6R5_UNORM,
        DXGI_FORMAT_B5G5R5A1_UNORM,
        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_FORMAT_B8G8R8X8_UNORM,
        DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
        DXGI_FORMAT_B8G8R8A8_TYPELESS,
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
        DXGI_FORMAT_B8G8R8X8_TYPELESS,
        DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,
        DXGI_FORMAT_BC6H_TYPELESS,
        DXGI_FORMAT_BC6H_UF16,
        DXGI_FORMAT_BC6H_SF16,
        DXGI_FORMAT_BC7_TYPELESS,
        DXGI_FORMAT_BC7_UNORM,
        DXGI_FORMAT_BC7_UNORM_SRGB,
        DXGI_FORMAT_AYUV,
        DXGI_FORMAT_Y410,
        DXGI_FORMAT_Y416,
        DXGI_FORMAT_NV12,
        DXGI_FORMAT_P010,
        DXGI_FORMAT_P016,
        DXGI_FORMAT_420_OPAQUE,
        DXGI_FORMAT_YUY2,
        DXGI_FORMAT_Y210,
        DXGI_FORMAT_Y216,
        DXGI_FORMAT_NV11,
        DXGI_FORMAT_AI44,
        DXGI_FORMAT_IA44,
        DXGI_FORMAT_P8,
        DXGI_FORMAT_A8P8,
        DXGI_FORMAT_B4G4R4A4_UNORM,
        DXGI_FORMAT_P208,
        DXGI_FORMAT_V208,
        DXGI_FORMAT_V408
    };

    Files::IStreamPtr BA2DX10File::getFile(const FileRecord& fileRecord)
    {
        DDSHeaderDX10 header;
        header.mSize = sizeof(DDSHeader);
        header.mWidth = fileRecord.mWidth;
        header.mHeight = fileRecord.mHeight;
        header.mFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_MIPMAPCOUNT;
        header.mCaps = DDSCAPS_TEXTURE;
        header.mMipMapCount = fileRecord.mNumMips;
        if (header.mMipMapCount > 1)
            header.mCaps = header.mCaps | DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
        header.mDepth = 0;

        header.mResourceDimension = DDS_DIMENSION_TEXTURE2D;
        header.mArraySize = 1;

        if (fileRecord.mCubeMaps == 2049)
        {
            header.mCaps = header.mCaps | DDSCAPS_COMPLEX;
            header.mCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_POSITIVEX | DDSCAPS2_NEGATIVEX | DDSCAPS2_POSITIVEY
                | DDSCAPS2_NEGATIVEY | DDSCAPS2_POSITIVEZ | DDSCAPS2_NEGATIVEZ;
            header.mMiscFlags = DDS_RESOURCE_MISC_TEXTURECUBE;
        }
        header.mPixelFormat.mSize = sizeof(header.mPixelFormat);
        switch (DXGI(fileRecord.mDXGIFormat))
        {
            case DXGI_FORMAT_BC1_UNORM:
            {
                header.mFlags = header.mFlags | DDSD_LINEARSIZE;
                header.mPixelFormat.mFlags = DDPF_FOURCC;
                header.mPixelFormat.mFourCC = ESM::fourCC("DXT1");
                header.mPitchOrLinearSize = fileRecord.mWidth * fileRecord.mHeight / 2;
                break;
            }
            case DXGI_FORMAT_BC2_UNORM:
            {
                header.mFlags = header.mFlags | DDSD_LINEARSIZE;
                header.mPixelFormat.mFlags = DDPF_FOURCC;
                header.mPixelFormat.mFourCC = ESM::fourCC("DXT3");
                header.mPitchOrLinearSize = fileRecord.mWidth * fileRecord.mHeight;
                break;
            }
            case DXGI_FORMAT_BC3_UNORM:
            {
                header.mFlags = header.mFlags | DDSD_LINEARSIZE;
                header.mPixelFormat.mFlags = DDPF_FOURCC;
                header.mPixelFormat.mFourCC = ESM::fourCC("DXT5");
                header.mPitchOrLinearSize = fileRecord.mWidth * fileRecord.mHeight;
                break;
            }
            case DXGI_FORMAT_BC4_SNORM:
            {
                header.mFlags = header.mFlags | DDSD_LINEARSIZE;
                header.mPixelFormat.mFlags = DDPF_FOURCC;
                header.mPixelFormat.mFourCC = ESM::fourCC("BC4S");
                header.mPitchOrLinearSize = fileRecord.mWidth * fileRecord.mHeight / 2;
                break;
            }
            case DXGI_FORMAT_BC4_UNORM:
            {
                header.mFlags = header.mFlags | DDSD_LINEARSIZE;
                header.mPixelFormat.mFlags = DDPF_FOURCC;
                header.mPixelFormat.mFourCC = ESM::fourCC("BC4U");
                header.mPitchOrLinearSize = fileRecord.mWidth * fileRecord.mHeight / 2;
                break;
            }
            case DXGI_FORMAT_BC5_SNORM:
            {
                header.mFlags = header.mFlags | DDSD_LINEARSIZE;
                header.mPixelFormat.mFlags = DDPF_FOURCC;
                header.mPixelFormat.mFourCC = ESM::fourCC("BC5S");
                header.mPitchOrLinearSize = fileRecord.mWidth * fileRecord.mHeight;
                break;
            }
            case DXGI_FORMAT_BC5_UNORM:
            {
                header.mFlags = header.mFlags | DDSD_LINEARSIZE;
                header.mPixelFormat.mFlags = DDPF_FOURCC;
                header.mPixelFormat.mFourCC = ESM::fourCC("BC5U");
                header.mPitchOrLinearSize = fileRecord.mWidth * fileRecord.mHeight;
                break;
            }
            case DXGI_FORMAT_BC1_UNORM_SRGB:
            {
                header.mFlags = header.mFlags | DDSD_LINEARSIZE;
                header.mPixelFormat.mFlags = DDPF_FOURCC;
                header.mPixelFormat.mFourCC = ESM::fourCC("DX10");
                header.mDXGIFormat = int32_t(fileRecord.mDXGIFormat);
                header.mPitchOrLinearSize = fileRecord.mWidth * fileRecord.mHeight / 2;
                break;
            }
            case DXGI_FORMAT_BC2_UNORM_SRGB:
            case DXGI_FORMAT_BC3_UNORM_SRGB:
            case DXGI_FORMAT_BC6H_UF16:
            case DXGI_FORMAT_BC6H_SF16:
            case DXGI_FORMAT_BC7_UNORM:
            case DXGI_FORMAT_BC7_UNORM_SRGB:
            {
                header.mFlags = header.mFlags | DDSD_LINEARSIZE;
                header.mPixelFormat.mFlags = DDPF_FOURCC;
                header.mPixelFormat.mFourCC = ESM::fourCC("DX10");
                header.mDXGIFormat = int32_t(fileRecord.mDXGIFormat);
                header.mPitchOrLinearSize = fileRecord.mWidth * fileRecord.mHeight;
                break;
            }
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            case DXGI_FORMAT_R8G8B8A8_SINT:
            case DXGI_FORMAT_R8G8B8A8_UINT:
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            {
                header.mFlags = header.mFlags | DDSD_PITCH;
                header.mPixelFormat.mFlags = DDPF_FOURCC;
                header.mPixelFormat.mFourCC = ESM::fourCC("DX10");
                header.mDXGIFormat = int32_t(fileRecord.mDXGIFormat);
                header.mPitchOrLinearSize = fileRecord.mWidth * 4;
                break;
            }
            case DXGI_FORMAT_R8G8_SINT:
            case DXGI_FORMAT_R8G8_UINT:
            {
                header.mFlags = header.mFlags | DDSD_PITCH;
                header.mPixelFormat.mFlags = DDPF_FOURCC;
                header.mPixelFormat.mFourCC = ESM::fourCC("DX10");
                header.mDXGIFormat = int32_t(fileRecord.mDXGIFormat);
                header.mPitchOrLinearSize = fileRecord.mWidth * 2;
                break;
            }
            case DXGI_FORMAT_R8_SINT:
            case DXGI_FORMAT_R8_SNORM:
            case DXGI_FORMAT_R8_UINT:
            {
                header.mFlags = header.mFlags | DDSD_PITCH;
                header.mPixelFormat.mFlags = DDPF_FOURCC;
                header.mPixelFormat.mFourCC = ESM::fourCC("DX10");
                header.mDXGIFormat = int32_t(fileRecord.mDXGIFormat);
                header.mPitchOrLinearSize = fileRecord.mWidth;
                break;
            }
            case DXGI_FORMAT_R8G8B8A8_UNORM:
            {
                header.mFlags = header.mFlags | DDSD_PITCH;
                header.mPixelFormat.mFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
                header.mPixelFormat.mRGBBitCount = 32;
                header.mPixelFormat.mRBitMask = 0x000000FF;
                header.mPixelFormat.mGBitMask = 0x0000FF00;
                header.mPixelFormat.mBBitMask = 0x00FF0000;
                header.mPixelFormat.mABitMask = 0xFF000000;
                header.mPitchOrLinearSize = fileRecord.mWidth * 4;
                break;
            }
            case DXGI_FORMAT_B8G8R8A8_UNORM:
            {
                header.mFlags = header.mFlags | DDSD_PITCH;
                header.mPixelFormat.mFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
                header.mPixelFormat.mRGBBitCount = 32;
                header.mPixelFormat.mRBitMask = 0x00FF0000;
                header.mPixelFormat.mGBitMask = 0x0000FF00;
                header.mPixelFormat.mBBitMask = 0x000000FF;
                header.mPixelFormat.mABitMask = 0xFF000000;
                header.mPitchOrLinearSize = fileRecord.mWidth * 4;
                break;
            }
            case DXGI_FORMAT_B8G8R8X8_UNORM:
            {
                header.mFlags = header.mFlags | DDSD_PITCH;
                header.mPixelFormat.mFlags = DDPF_RGB;
                header.mPixelFormat.mRGBBitCount = 32;
                header.mPixelFormat.mRBitMask = 0x00FF0000;
                header.mPixelFormat.mGBitMask = 0x0000FF00;
                header.mPixelFormat.mBBitMask = 0x000000FF;
                header.mPitchOrLinearSize = fileRecord.mWidth * 4;
                break;
            }
            case DXGI_FORMAT_B5G6R5_UNORM:
            {
                header.mFlags = header.mFlags | DDSD_PITCH;
                header.mPixelFormat.mFlags = DDPF_RGB;
                header.mPixelFormat.mRGBBitCount = 16;
                header.mPixelFormat.mRBitMask = 0x0000F800;
                header.mPixelFormat.mGBitMask = 0x000007E0;
                header.mPixelFormat.mBBitMask = 0x0000001F;
                header.mPitchOrLinearSize = fileRecord.mWidth * 2;
                break;
            }
            case DXGI_FORMAT_B5G5R5A1_UNORM:
            {
                header.mFlags = header.mFlags | DDSD_PITCH;
                header.mPixelFormat.mFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
                header.mPixelFormat.mRGBBitCount = 16;
                header.mPixelFormat.mRBitMask = 0x00007C00;
                header.mPixelFormat.mGBitMask = 0x000003E0;
                header.mPixelFormat.mBBitMask = 0x0000001F;
                header.mPixelFormat.mABitMask = 0x00008000;
                header.mPitchOrLinearSize = fileRecord.mWidth * 2;
                break;
            }
            case DXGI_FORMAT_R8G8_UNORM:
            {
                header.mFlags = header.mFlags | DDSD_PITCH;
                header.mPixelFormat.mFlags = DDPF_LUMINANCE | DDPF_ALPHAPIXELS;
                header.mPixelFormat.mRGBBitCount = 16;
                header.mPixelFormat.mRBitMask = 0x000000FF;
                header.mPixelFormat.mABitMask = 0x0000FF00;
                header.mPitchOrLinearSize = fileRecord.mWidth * 2;
                break;
            }
            case DXGI_FORMAT_A8_UNORM:
            {
                header.mFlags = header.mFlags | DDSD_PITCH;
                header.mPixelFormat.mFlags = DDPF_ALPHA;
                header.mPixelFormat.mRGBBitCount = 8;
                header.mPixelFormat.mABitMask = 0x000000FF;
                header.mPitchOrLinearSize = fileRecord.mWidth;
                break;
            }
            case DXGI_FORMAT_R8_UNORM:
            {
                header.mFlags = header.mFlags | DDSD_PITCH;
                header.mPixelFormat.mFlags = DDPF_LUMINANCE;
                header.mPixelFormat.mRGBBitCount = 8;
                header.mPixelFormat.mRBitMask = 0x000000FF;
                header.mPitchOrLinearSize = fileRecord.mWidth;
                break;
            }
            default:
                break;
        }

        const bool isDx10 = header.mPixelFormat.mFourCC == ESM::fourCC("DX10");
        const size_t headerSize = isDx10 ? sizeof(DDSHeaderDX10) : sizeof(DDSHeader);

        size_t textureSize = sizeof(uint32_t) + headerSize; //"DDS " + header
        uint32_t maxPackedChunkSize = 0;
        for (const auto& textureChunk : fileRecord.mTextureChunks)
        {
            textureSize += textureChunk.mSize;
            maxPackedChunkSize = std::max(textureChunk.mPackedSize, maxPackedChunkSize);
        }

        auto memoryStreamPtr = std::make_unique<MemoryInputStream>(textureSize);
        char* buff = memoryStreamPtr->getRawData();
        std::vector<char> inputBuffer(maxPackedChunkSize);

        uint32_t dds = ESM::fourCC("DDS ");
        buff = (char*)std::memcpy(buff, &dds, sizeof(uint32_t)) + sizeof(uint32_t);
        std::memcpy(buff, &header, headerSize);

        size_t offset = sizeof(uint32_t) + headerSize;
        // append chunks
        for (const auto& c : fileRecord.mTextureChunks)
        {
            const uint32_t inputSize = c.mPackedSize != 0 ? c.mPackedSize : c.mSize;
            Files::IStreamPtr streamPtr = Files::openConstrainedFileStream(mFilepath, c.mOffset, inputSize);
            if (c.mPackedSize != 0)
            {
                streamPtr->read(inputBuffer.data(), c.mPackedSize);
                uLongf destSize = static_cast<uLongf>(c.mSize);
                int ec = ::uncompress(reinterpret_cast<Bytef*>(memoryStreamPtr->getRawData() + offset), &destSize,
                    reinterpret_cast<Bytef*>(inputBuffer.data()), static_cast<uLong>(c.mPackedSize));

                if (ec != Z_OK)
                    fail("zlib uncompress failed: " + std::string(::zError(ec)));
            }
            // uncompressed chunk
            else
            {
                streamPtr->read(memoryStreamPtr->getRawData() + offset, c.mSize);
            }
            offset += c.mSize;
        }

        return std::make_unique<Files::StreamWithBuffer<MemoryInputStream>>(std::move(memoryStreamPtr));
    }

} // namespace Bsa
