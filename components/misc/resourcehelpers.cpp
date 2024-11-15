#include "resourcehelpers.hpp"

#include <algorithm>
#include <sstream>
#include <string_view>

#include <components/esm/common.hpp>
#include <components/esm/refid.hpp>

#include <components/misc/pathhelpers.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/lower.hpp>

#include <components/vfs/manager.hpp>
#include <components/vfs/pathutil.hpp>

namespace
{
    bool changeExtension(std::string& path, std::string_view ext)
    {
        std::string::size_type pos = path.rfind('.');
        if (pos != std::string::npos && path.compare(pos, path.length() - pos, ext) != 0)
        {
            path.replace(pos, path.length(), ext);
            return true;
        }
        return false;
    }
}

bool Misc::ResourceHelpers::changeExtensionToDds(std::string& path)
{
    return changeExtension(path, ".dds");
}

std::string Misc::ResourceHelpers::correctResourcePath(
    std::span<const std::string_view> topLevelDirectories, std::string_view resPath, const VFS::Manager* vfs)
{
    /* Bethesda at some point converted all their BSA
     * textures from tga to dds for increased load speed, but all
     * texture file name references were kept as .tga.
     */

    std::string correctedPath = Misc::StringUtils::lowerCase(resPath);

    // Flatten slashes
    std::replace(correctedPath.begin(), correctedPath.end(), '/', '\\');
    auto bothSeparators = [](char a, char b) { return a == '\\' && b == '\\'; };
    correctedPath.erase(std::unique(correctedPath.begin(), correctedPath.end(), bothSeparators), correctedPath.end());

    // Remove leading separator
    if (!correctedPath.empty() && correctedPath[0] == '\\')
        correctedPath.erase(0, 1);

    // Handle top level directory
    bool needsPrefix = true;
    for (std::string_view potentialTopLevelDirectory : topLevelDirectories)
    {
        if (correctedPath.starts_with(potentialTopLevelDirectory)
            && correctedPath.size() > potentialTopLevelDirectory.size()
            && correctedPath[potentialTopLevelDirectory.size()] == '\\')
        {
            needsPrefix = false;
            break;
        }
        else
        {
            std::string topLevelPrefix = std::string{ potentialTopLevelDirectory } + '\\';
            size_t topLevelPos = correctedPath.find('\\' + topLevelPrefix);
            if (topLevelPos != std::string::npos)
            {
                correctedPath.erase(0, topLevelPos + 1);
                needsPrefix = false;
                break;
            }
        }
    }
    if (needsPrefix)
        correctedPath = std::string{ topLevelDirectories.front() } + '\\' + correctedPath;

    std::string origExt = correctedPath;

    // since we know all (GOTY edition or less) textures end
    // in .dds, we change the extension
    bool changedToDds = changeExtensionToDds(correctedPath);
    if (vfs->exists(correctedPath))
        return correctedPath;
    // if it turns out that the above wasn't true in all cases (not for vanilla, but maybe mods)
    // verify, and revert if false (this call succeeds quickly, but fails slowly)
    if (changedToDds && vfs->exists(origExt))
        return origExt;

    // fall back to a resource in the top level directory if it exists
    std::string fallback{ topLevelDirectories.front() };
    fallback += '\\';
    fallback += Misc::getFileName(correctedPath);

    if (vfs->exists(fallback))
        return fallback;

    if (changedToDds)
    {
        fallback = topLevelDirectories.front();
        fallback += '\\';
        fallback += Misc::getFileName(origExt);
        if (vfs->exists(fallback))
            return fallback;
    }

    return correctedPath;
}

std::string Misc::ResourceHelpers::correctTexturePath(std::string_view resPath, const VFS::Manager* vfs)
{
    return correctResourcePath({ { "textures", "bookart" } }, resPath, vfs);
}

std::string Misc::ResourceHelpers::correctIconPath(std::string_view resPath, const VFS::Manager* vfs)
{
    return correctResourcePath({ { "icons" } }, resPath, vfs);
}

std::string Misc::ResourceHelpers::correctBookartPath(std::string_view resPath, const VFS::Manager* vfs)
{
    return correctResourcePath({ { "bookart", "textures" } }, resPath, vfs);
}

std::string Misc::ResourceHelpers::correctBookartPath(
    std::string_view resPath, int width, int height, const VFS::Manager* vfs)
{
    std::string image = correctBookartPath(resPath, vfs);

    // Apparently a bug with some morrowind versions, they reference the image without the size suffix.
    // So if the image isn't found, try appending the size.
    if (!vfs->exists(image))
    {
        std::stringstream str;
        str << image.substr(0, image.rfind('.')) << "_" << width << "_" << height << image.substr(image.rfind('.'));
        image = Misc::ResourceHelpers::correctBookartPath(str.str(), vfs);
    }

    return image;
}

VFS::Path::Normalized Misc::ResourceHelpers::correctActorModelPath(
    VFS::Path::NormalizedView resPath, const VFS::Manager* vfs)
{
    std::string mdlname(resPath.value());
    std::string::size_type p = mdlname.find_last_of('/');
    if (p != std::string::npos)
        mdlname.insert(mdlname.begin() + static_cast<std::string::difference_type>(p) + 1, 'x');
    else
        mdlname.insert(mdlname.begin(), 'x');

    VFS::Path::Normalized kfname(mdlname);
    if (Misc::getFileExtension(mdlname) == "nif")
        kfname.changeExtension("kf");

    if (!vfs->exists(kfname))
        return VFS::Path::Normalized(resPath);

    return mdlname;
}

std::string Misc::ResourceHelpers::correctMaterialPath(std::string_view resPath, const VFS::Manager* vfs)
{
    return correctResourcePath({ { "materials" } }, resPath, vfs);
}

std::string Misc::ResourceHelpers::correctMeshPath(std::string_view resPath)
{
    std::string res = "meshes\\";
    res.append(resPath);
    return res;
}

VFS::Path::Normalized Misc::ResourceHelpers::correctSoundPath(VFS::Path::NormalizedView resPath)
{
    static constexpr VFS::Path::NormalizedView prefix("sound");
    return prefix / resPath;
}

VFS::Path::Normalized Misc::ResourceHelpers::correctMusicPath(VFS::Path::NormalizedView resPath)
{
    static constexpr VFS::Path::NormalizedView prefix("music");
    return prefix / resPath;
}

std::string_view Misc::ResourceHelpers::meshPathForESM3(std::string_view resPath)
{
    constexpr std::string_view prefix = "meshes";
    if (resPath.length() < prefix.size() + 1 || !Misc::StringUtils::ciStartsWith(resPath, prefix)
        || (resPath[prefix.size()] != '/' && resPath[prefix.size()] != '\\'))
    {
        throw std::runtime_error("Path should start with 'meshes\\'");
    }
    return resPath.substr(prefix.size() + 1);
}

VFS::Path::Normalized Misc::ResourceHelpers::correctSoundPath(
    VFS::Path::NormalizedView resPath, const VFS::Manager& vfs)
{
    // Workaround: Bethesda at some point converted some of the files to mp3, but the references were kept as .wav.
    if (!vfs.exists(resPath))
    {
        VFS::Path::Normalized sound(resPath);
        sound.changeExtension("mp3");
        return sound;
    }
    return VFS::Path::Normalized(resPath);
}

bool Misc::ResourceHelpers::isHiddenMarker(const ESM::RefId& id)
{
    return id == "prisonmarker" || id == "divinemarker" || id == "templemarker" || id == "northmarker";
}

namespace
{
    VFS::Path::Normalized getLODMeshNameImpl(VFS::Path::NormalizedView resPath, std::string_view pattern)
    {
        const std::string_view::size_type position = Misc::findExtension(resPath.value());
        if (position == std::string::npos)
            return VFS::Path::Normalized(resPath);
        std::string withPattern(resPath.value());
        withPattern.insert(position, pattern);
        return VFS::Path::Normalized(std::move(withPattern));
    }

    VFS::Path::Normalized getBestLODMeshName(
        VFS::Path::NormalizedView resPath, const VFS::Manager& vfs, std::string_view pattern)
    {
        if (VFS::Path::Normalized result = getLODMeshNameImpl(resPath, pattern); vfs.exists(result))
            return result;
        return VFS::Path::Normalized(resPath);
    }

    std::string_view getDistantMeshPattern(int esmVersion)
    {
        static constexpr std::string_view dist = "_dist";
        static constexpr std::string_view far = "_far";
        static constexpr std::string_view lod = "_lod";

        switch (esmVersion)
        {
            case ESM::VER_120:
            case ESM::VER_130:
                return dist;
            case ESM::VER_080:
            case ESM::VER_100:
                return far;
            case ESM::VER_094:
            case ESM::VER_170:
                return lod;
            default:
                return std::string_view();
        }
    }
}

VFS::Path::Normalized Misc::ResourceHelpers::getLODMeshName(
    int esmVersion, VFS::Path::NormalizedView resPath, const VFS::Manager& vfs, unsigned char lod)
{
    const std::string_view distantMeshPattern = getDistantMeshPattern(esmVersion);
    for (int l = lod; l >= 0; --l)
    {
        std::stringstream patern;
        patern << distantMeshPattern << "_" << l;
        const VFS::Path::Normalized meshName = getBestLODMeshName(resPath, vfs, patern.view());
        if (meshName != resPath)
            return meshName;
    }
    return getBestLODMeshName(resPath, vfs, distantMeshPattern);
}
