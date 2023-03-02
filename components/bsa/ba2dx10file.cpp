#include "ba2dx10file.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>

#include <lz4frame.h>

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4706)
#include <boost/iostreams/filter/zlib.hpp>
#pragma warning(pop)
#else
#include <boost/iostreams/filter/zlib.hpp>
#endif

#include <boost/iostreams/device/array.hpp>
#include <components/bsa/memorystream.hpp>
#include <components/esm/fourcc.hpp>
#include <components/files/constrainedfilestream.hpp>
#include <components/files/conversion.hpp>
#include <components/misc/strings/lower.hpp>

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
            uint32_t unknown;
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

            mFolders[dirHash][{ nameHash, extHash }] = file;

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

            if (header[0] == 0x00415342) /*"BSA\x00"*/
                fail("Unrecognized compressed BSA format");
            mVersion = header[1];
            if (mVersion != 0x01 /*F04*/)
                fail("Unrecognized compressed BSA version");

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
            fileName.resize(fileNameSize);
            input.read(fileName.data(), fileName.size());
            fileName.push_back('\0');
            mFileNames.push_back(fileName);
            mFiles[i].setNameInfos(0, &mFileNames.back());
        }

        mIsLoaded = true;
    }

    BA2DX10File::FileRecord BA2DX10File::getFileRecord(const std::string& str) const
    {
        for (const auto c : str)
        {
            if (((static_cast<unsigned>(c) >> 7U) & 1U) != 0U)
            {
                fail("File record " + str + " contains unicode characters, refusing to load.");
            }
        }

#ifdef _WIN32
        const auto& path = str;
#else
        // Force-convert the path into something UNIX can handle first
        // to make sure std::filesystem::path doesn't think the entire path is the filename on Linux
        // and subsequently purge it to determine the file folder.
        std::string path = str;
        std::replace(path.begin(), path.end(), '\\', '/');
#endif

        const auto p = std::filesystem::path{ path }; // Purposefully damage Unicode strings.
        const auto fileName = Misc::StringUtils::lowerCase(p.stem().string());
        const auto ext = Misc::StringUtils::lowerCase(p.extension().string()); // Purposefully damage Unicode strings.
        const auto folder = Misc::StringUtils::lowerCase(p.parent_path().string());

        uint32_t folderHash = generateHash(folder);
        auto it = mFolders.find(folderHash);
        if (it == mFolders.end())
            return FileRecord(); // folder not found, return default which has offset of sInvalidOffset

        uint32_t fileHash = generateHash(fileName);
        uint32_t extHash = *reinterpret_cast<const uint32_t*>(ext.data() + 1);
        auto iter = it->second.find({ fileHash, extHash });
        if (iter == it->second.end())
            return FileRecord(); // file not found, return default which has offset of sInvalidOffset
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
        FileRecord fileRec = getFileRecord(file->name());
        return getFile(fileRec);
    }

    void BA2DX10File::addFile(const std::string& filename, std::istream& file)
    {
        assert(false); // not implemented yet
        fail("Add file is not implemented for compressed BSA: " + filename);
    }

    Files::IStreamPtr BA2DX10File::getFile(const char* file)
    {
        FileRecord fileRec = getFileRecord(file);
        return getFile(fileRec);
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
        header.depth = 1;

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
        for (const auto& textureChunk : fileRecord.texturesChunks)
            textureSize += textureChunk.size;

        auto memoryStreamPtr = std::make_unique<MemoryInputStream>(textureSize);
        char* buff = memoryStreamPtr->getRawData();

        uint32_t dds = ESM::fourCC("DDS ");
        buff = (char*)std::memcpy(buff, &dds, sizeof(uint32_t)) + sizeof(uint32_t);
        std::memcpy(buff, &header, headerSize);

        size_t offset = headerSize;
        // append chunks
        for (const auto& c : fileRecord.texturesChunks)
        {
            if (c.packedSize != 0)
            {
                Files::IStreamPtr streamPtr = Files::openConstrainedFileStream(mFilepath, c.offset, c.packedSize);
                std::istream* fileStream = streamPtr.get();

                boost::iostreams::filtering_streambuf<boost::iostreams::input> inputStreamBuf;
                inputStreamBuf.push(boost::iostreams::zlib_decompressor());
                inputStreamBuf.push(*fileStream);

                boost::iostreams::basic_array_sink<char> sr(memoryStreamPtr->getRawData() + offset, c.size);
                boost::iostreams::copy(inputStreamBuf, sr);
            }
            // uncompressed chunk
            else
            {
                Files::IStreamPtr streamPtr = Files::openConstrainedFileStream(mFilepath, c.offset, c.packedSize);
                std::istream* fileStream = streamPtr.get();
                fileStream->read(memoryStreamPtr->getRawData(), c.size);
            }
            offset += c.size;
        }

        return std::make_unique<Files::StreamWithBuffer<MemoryInputStream>>(std::move(memoryStreamPtr));
    }

    constexpr const uint32_t crc32table[256] = { 0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
        0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07,
        0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
        0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5, 0x3b6e20c8,
        0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
        0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a, 0xc8d75180,
        0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
        0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589,
        0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
        0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1,
        0xf50fc457, 0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
        0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a,
        0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
        0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f, 0x5edef90e, 0x29d9c998, 0xb0d09822,
        0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
        0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b,
        0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
        0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43,
        0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
        0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79, 0xcb61b38c,
        0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
        0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c,
        0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
        0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd,
        0xf6b9265b, 0x6fb077e1, 0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
        0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d,
        0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
        0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e,
        0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d };

    uint32_t BA2DX10File::generateHash(const std::string& name)
    {
        uint32_t result = 0;
        for (auto c : name)
        {
            if (uint8_t(c) > 127)
                continue;
            if (c == '/')
                c = '\\';
            result = (result >> 8) ^ crc32table[(result ^ (unsigned)(c)) & 0xFF];
        }
        return result;
    }

} // namespace Bsa
