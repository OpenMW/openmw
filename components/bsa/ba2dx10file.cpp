#include "ba2dx10file.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>

#include <zlib.h>

#include <components/esm/fourcc.hpp>
#include <components/files/constrainedfilestream.hpp>
#include <components/files/conversion.hpp>
#include <components/misc/strings/lower.hpp>

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

            file.texturesChunks.resize(nbChunks);

            uint16_t chunkHeaderSize;
            in.read(reinterpret_cast<char*>(&chunkHeaderSize), sizeof(uint16_t));
            if (chunkHeaderSize != 24)
                fail("Corrupted BSA");

            in.read(reinterpret_cast<char*>(&file.height), sizeof(uint16_t));
            in.read(reinterpret_cast<char*>(&file.width), sizeof(uint16_t));
            in.read(reinterpret_cast<char*>(&file.numMips), sizeof(uint8_t));
            in.read(reinterpret_cast<char*>(&file.DXGIFormat), sizeof(uint8_t));
            in.read(reinterpret_cast<char*>(&file.cubeMaps), sizeof(uint16_t));
            for (auto& texture : file.texturesChunks)
            {
                in.read(reinterpret_cast<char*>(&texture.offset), sizeof(int64_t));
                in.read(reinterpret_cast<char*>(&texture.packedSize), sizeof(uint32_t));
                in.read(reinterpret_cast<char*>(&texture.size), sizeof(uint32_t));
                in.read(reinterpret_cast<char*>(&texture.startMip), sizeof(uint16_t));
                in.read(reinterpret_cast<char*>(&texture.endMip), sizeof(uint16_t));
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
    void BA2DX10File::readHeader()
    {
        assert(!mIsLoaded);

        std::ifstream input(mFilepath, std::ios_base::binary);

        // Total archive size
        std::streamoff fsize = 0;
        if (input.seekg(0, std::ios_base::end))
        {
            fsize = input.tellg();
            input.seekg(0);
        }

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

        mIsLoaded = true;
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

#ifdef _WIN32
        const auto& path = str;
#else
        // Force-convert the path into something UNIX can handle first
        // to make sure std::filesystem::path doesn't think the entire path is the filename on Linux
        // and subsequently purge it to determine the file folder.
        std::string path(str);
        std::replace(path.begin(), path.end(), '\\', '/');
#endif

        const auto p = std::filesystem::path{ path }; // Purposefully damage Unicode strings.
        const auto fileName = Misc::StringUtils::lowerCase(p.stem().string());
        const auto ext = Misc::StringUtils::lowerCase(p.extension().string()); // Purposefully damage Unicode strings.
        const auto folder = Misc::StringUtils::lowerCase(p.parent_path().string());

        uint32_t folderHash = generateHash(folder);
        auto it = mFolders.find(folderHash);
        if (it == mFolders.end())
            return std::nullopt; // folder not found

        uint32_t fileHash = generateHash(fileName);
        uint32_t extHash = generateExtensionHash(ext);
        auto iter = it->second.find({ fileHash, extHash });
        if (iter == it->second.end())
            return std::nullopt; // file not found
        return iter->second;
    }

#pragma pack(push)
#pragma pack(1)
    struct DDSHeader
    {
        uint32_t size = 0;
        uint32_t flags = 0;
        uint32_t height = 0;
        uint32_t width = 0;
        uint32_t pitchOrLinearSize = 0;
        uint32_t depth = 0;
        uint32_t mipMapCount = 0;
        uint32_t reserved1[11] = {};
        struct
        {
            uint32_t size = 0;
            uint32_t flags = 0;
            uint32_t fourCC = 0;
            uint32_t RGBBitCount = 0;
            uint32_t RBitMask = 0;
            uint32_t GBitMask = 0;
            uint32_t BBitMask = 0;
            uint32_t ABitMask = 0;
        } ddspf;
        uint32_t caps = 0;
        uint32_t caps2 = 0;
        uint32_t caps3 = 0;
        uint32_t caps4 = 0;
        uint32_t reserved2 = 0;
    };

    struct DDSHeaderDX10 : DDSHeader
    {
        int32_t dxgiFormat = 0;
        uint32_t resourceDimension = 0;
        uint32_t miscFlags = 0;
        uint32_t arraySize = 0;
        uint32_t miscFlags2 = 0;
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

    Files::IStreamPtr BA2DX10File::getFile(const char* file)
    {
        if (auto fileRec = getFileRecord(file); fileRec)
            return getFile(*fileRec);
        fail("File not found: " + std::string(file));
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
        header.size = sizeof(DDSHeader);
        header.width = fileRecord.width;
        header.height = fileRecord.height;
        header.flags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_MIPMAPCOUNT;
        header.caps = DDSCAPS_TEXTURE;
        header.mipMapCount = fileRecord.numMips;
        if (header.mipMapCount > 1)
            header.caps = header.caps | DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
        header.depth = 0;

        header.resourceDimension = DDS_DIMENSION_TEXTURE2D;
        header.arraySize = 1;

        if (fileRecord.cubeMaps == 2049)
        {
            header.caps = header.caps | DDSCAPS_COMPLEX;
            header.caps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_POSITIVEX | DDSCAPS2_NEGATIVEX | DDSCAPS2_POSITIVEY
                | DDSCAPS2_NEGATIVEY | DDSCAPS2_POSITIVEZ | DDSCAPS2_NEGATIVEZ;
            header.miscFlags = DDS_RESOURCE_MISC_TEXTURECUBE;
        }
        header.ddspf.size = sizeof(header.ddspf);
        switch (DXGI(fileRecord.DXGIFormat))
        {
            case DXGI_FORMAT_BC1_UNORM:
            {
                header.flags = header.flags | DDSD_LINEARSIZE;
                header.ddspf.flags = DDPF_FOURCC;
                header.ddspf.fourCC = ESM::fourCC("DXT1");
                header.pitchOrLinearSize = fileRecord.width * fileRecord.height / 2;
                break;
            }
            case DXGI_FORMAT_BC2_UNORM:
            {
                header.flags = header.flags | DDSD_LINEARSIZE;
                header.ddspf.flags = DDPF_FOURCC;
                header.ddspf.fourCC = ESM::fourCC("DXT3");
                header.pitchOrLinearSize = fileRecord.width * fileRecord.height;
                break;
            }
            case DXGI_FORMAT_BC3_UNORM:
            {
                header.flags = header.flags | DDSD_LINEARSIZE;
                header.ddspf.flags = DDPF_FOURCC;
                header.ddspf.fourCC = ESM::fourCC("DXT5");
                header.pitchOrLinearSize = fileRecord.width * fileRecord.height;
                break;
            }
            case DXGI_FORMAT_BC4_SNORM:
            {
                header.flags = header.flags | DDSD_LINEARSIZE;
                header.ddspf.flags = DDPF_FOURCC;
                header.ddspf.fourCC = ESM::fourCC("BC4S");
                header.pitchOrLinearSize = fileRecord.width * fileRecord.height / 2;
                break;
            }
            case DXGI_FORMAT_BC4_UNORM:
            {
                header.flags = header.flags | DDSD_LINEARSIZE;
                header.ddspf.flags = DDPF_FOURCC;
                header.ddspf.fourCC = ESM::fourCC("BC4U");
                header.pitchOrLinearSize = fileRecord.width * fileRecord.height / 2;
                break;
            }
            case DXGI_FORMAT_BC5_SNORM:
            {
                header.flags = header.flags | DDSD_LINEARSIZE;
                header.ddspf.flags = DDPF_FOURCC;
                header.ddspf.fourCC = ESM::fourCC("BC5S");
                header.pitchOrLinearSize = fileRecord.width * fileRecord.height;
                break;
            }
            case DXGI_FORMAT_BC5_UNORM:
            {
                header.flags = header.flags | DDSD_LINEARSIZE;
                header.ddspf.flags = DDPF_FOURCC;
                header.ddspf.fourCC = ESM::fourCC("BC5U");
                header.pitchOrLinearSize = fileRecord.width * fileRecord.height;
                break;
            }
            case DXGI_FORMAT_BC1_UNORM_SRGB:
            {
                header.flags = header.flags | DDSD_LINEARSIZE;
                header.ddspf.flags = DDPF_FOURCC;
                header.ddspf.fourCC = ESM::fourCC("DX10");
                header.dxgiFormat = int32_t(fileRecord.DXGIFormat);
                header.pitchOrLinearSize = fileRecord.width * fileRecord.height / 2;
                break;
            }
            case DXGI_FORMAT_BC2_UNORM_SRGB:
            case DXGI_FORMAT_BC3_UNORM_SRGB:
            case DXGI_FORMAT_BC6H_UF16:
            case DXGI_FORMAT_BC6H_SF16:
            case DXGI_FORMAT_BC7_UNORM:
            case DXGI_FORMAT_BC7_UNORM_SRGB:
            {
                header.flags = header.flags | DDSD_LINEARSIZE;
                header.ddspf.flags = DDPF_FOURCC;
                header.ddspf.fourCC = ESM::fourCC("DX10");
                header.dxgiFormat = int32_t(fileRecord.DXGIFormat);
                header.pitchOrLinearSize = fileRecord.width * fileRecord.height;
                break;
            }
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            case DXGI_FORMAT_R8G8B8A8_SINT:
            case DXGI_FORMAT_R8G8B8A8_UINT:
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            {
                header.flags = header.flags | DDSD_PITCH;
                header.ddspf.flags = DDPF_FOURCC;
                header.ddspf.fourCC = ESM::fourCC("DX10");
                header.dxgiFormat = int32_t(fileRecord.DXGIFormat);
                header.pitchOrLinearSize = fileRecord.width * 4;
                break;
            }
            case DXGI_FORMAT_R8G8_SINT:
            case DXGI_FORMAT_R8G8_UINT:
            {
                header.flags = header.flags | DDSD_PITCH;
                header.ddspf.flags = DDPF_FOURCC;
                header.ddspf.fourCC = ESM::fourCC("DX10");
                header.dxgiFormat = int32_t(fileRecord.DXGIFormat);
                header.pitchOrLinearSize = fileRecord.width * 2;
                break;
            }
            case DXGI_FORMAT_R8_SINT:
            case DXGI_FORMAT_R8_SNORM:
            case DXGI_FORMAT_R8_UINT:
            {
                header.flags = header.flags | DDSD_PITCH;
                header.ddspf.flags = DDPF_FOURCC;
                header.ddspf.fourCC = ESM::fourCC("DX10");
                header.dxgiFormat = int32_t(fileRecord.DXGIFormat);
                header.pitchOrLinearSize = fileRecord.width;
                break;
            }
            case DXGI_FORMAT_R8G8B8A8_UNORM:
            {
                header.flags = header.flags | DDSD_PITCH;
                header.ddspf.flags = DDPF_RGB | DDPF_ALPHAPIXELS;
                header.ddspf.RGBBitCount = 32;
                header.ddspf.RBitMask = 0x000000FF;
                header.ddspf.GBitMask = 0x0000FF00;
                header.ddspf.BBitMask = 0x00FF0000;
                header.ddspf.ABitMask = 0xFF000000;
                header.pitchOrLinearSize = fileRecord.width * 4;
                break;
            }
            case DXGI_FORMAT_B8G8R8A8_UNORM:
            {
                header.flags = header.flags | DDSD_PITCH;
                header.ddspf.flags = DDPF_RGB | DDPF_ALPHAPIXELS;
                header.ddspf.RGBBitCount = 32;
                header.ddspf.RBitMask = 0x00FF0000;
                header.ddspf.GBitMask = 0x0000FF00;
                header.ddspf.BBitMask = 0x000000FF;
                header.ddspf.ABitMask = 0xFF000000;
                header.pitchOrLinearSize = fileRecord.width * 4;
                break;
            }
            case DXGI_FORMAT_B8G8R8X8_UNORM:
            {
                header.flags = header.flags | DDSD_PITCH;
                header.ddspf.flags = DDPF_RGB;
                header.ddspf.RGBBitCount = 32;
                header.ddspf.RBitMask = 0x00FF0000;
                header.ddspf.GBitMask = 0x0000FF00;
                header.ddspf.BBitMask = 0x000000FF;
                header.pitchOrLinearSize = fileRecord.width * 4;
                break;
            }
            case DXGI_FORMAT_B5G6R5_UNORM:
            {
                header.flags = header.flags | DDSD_PITCH;
                header.ddspf.flags = DDPF_RGB;
                header.ddspf.RGBBitCount = 16;
                header.ddspf.RBitMask = 0x0000F800;
                header.ddspf.GBitMask = 0x000007E0;
                header.ddspf.BBitMask = 0x0000001F;
                header.pitchOrLinearSize = fileRecord.width * 2;
                break;
            }
            case DXGI_FORMAT_B5G5R5A1_UNORM:
            {
                header.flags = header.flags | DDSD_PITCH;
                header.ddspf.flags = DDPF_RGB | DDPF_ALPHAPIXELS;
                header.ddspf.RGBBitCount = 16;
                header.ddspf.RBitMask = 0x00007C00;
                header.ddspf.GBitMask = 0x000003E0;
                header.ddspf.BBitMask = 0x0000001F;
                header.ddspf.ABitMask = 0x00008000;
                header.pitchOrLinearSize = fileRecord.width * 2;
                break;
            }
            case DXGI_FORMAT_R8G8_UNORM:
            {
                header.flags = header.flags | DDSD_PITCH;
                header.ddspf.flags = DDPF_LUMINANCE | DDPF_ALPHAPIXELS;
                header.ddspf.RGBBitCount = 16;
                header.ddspf.RBitMask = 0x000000FF;
                header.ddspf.ABitMask = 0x0000FF00;
                header.pitchOrLinearSize = fileRecord.width * 2;
                break;
            }
            case DXGI_FORMAT_A8_UNORM:
            {
                header.flags = header.flags | DDSD_PITCH;
                header.ddspf.flags = DDPF_ALPHA;
                header.ddspf.RGBBitCount = 8;
                header.ddspf.ABitMask = 0x000000FF;
                header.pitchOrLinearSize = fileRecord.width;
                break;
            }
            case DXGI_FORMAT_R8_UNORM:
            {
                header.flags = header.flags | DDSD_PITCH;
                header.ddspf.flags = DDPF_LUMINANCE;
                header.ddspf.RGBBitCount = 8;
                header.ddspf.RBitMask = 0x000000FF;
                header.pitchOrLinearSize = fileRecord.width;
                break;
            }
            default:
                break;
        }

        size_t headerSize = (header.ddspf.fourCC == ESM::fourCC("DX10") ? sizeof(DDSHeaderDX10) : sizeof(DDSHeader));

        size_t textureSize = sizeof(uint32_t) + headerSize; //"DDS " + header
        uint32_t maxPackedChunkSize = 0;
        for (const auto& textureChunk : fileRecord.texturesChunks)
        {
            textureSize += textureChunk.size;
            maxPackedChunkSize = std::max(textureChunk.packedSize, maxPackedChunkSize);
        }

        auto memoryStreamPtr = std::make_unique<MemoryInputStream>(textureSize);
        char* buff = memoryStreamPtr->getRawData();
        std::vector<char> inputBuffer(maxPackedChunkSize);

        uint32_t dds = ESM::fourCC("DDS ");
        buff = (char*)std::memcpy(buff, &dds, sizeof(uint32_t)) + sizeof(uint32_t);
        std::memcpy(buff, &header, headerSize);

        size_t offset = sizeof(uint32_t) + headerSize;
        // append chunks
        for (const auto& c : fileRecord.texturesChunks)
        {
            const uint32_t inputSize = c.packedSize != 0 ? c.packedSize : c.size;
            Files::IStreamPtr streamPtr = Files::openConstrainedFileStream(mFilepath, c.offset, inputSize);
            if (c.packedSize != 0)
            {
                streamPtr->read(inputBuffer.data(), c.packedSize);
                uLongf destSize = static_cast<uLongf>(c.size);
                int ec = ::uncompress(reinterpret_cast<Bytef*>(memoryStreamPtr->getRawData() + offset), &destSize,
                    reinterpret_cast<Bytef*>(inputBuffer.data()), static_cast<uLong>(c.packedSize));

                if (ec != Z_OK)
                    fail("zlib uncompress failed: " + std::string(::zError(ec)));
            }
            // uncompressed chunk
            else
            {
                streamPtr->read(memoryStreamPtr->getRawData() + offset, c.size);
            }
            offset += c.size;
        }

        return std::make_unique<Files::StreamWithBuffer<MemoryInputStream>>(std::move(memoryStreamPtr));
    }

} // namespace Bsa
