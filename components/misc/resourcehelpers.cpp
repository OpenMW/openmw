#include "resourcehelpers.hpp"

#include <sstream>

#include <components/misc/stringops.hpp>

#include <components/vfs/manager.hpp>

namespace
{


    struct MatchPathSeparator
    {
        bool operator()( char ch ) const
        {
            return ch == '\\' || ch == '/';
        }
    };

    std::string
    getBasename( std::string const& pathname )
    {
        return std::string(
            std::find_if( pathname.rbegin(), pathname.rend(),
                          MatchPathSeparator() ).base(),
            pathname.end() );
    }

}

bool Misc::ResourceHelpers::changeExtensionToDds(std::string &path)
{
    std::string::size_type pos = path.rfind('.');
    if(pos != std::string::npos && path.compare(pos, path.length() - pos, ".dds") != 0)
    {
        path.replace(pos, path.length(), ".dds");
        return true;
    }
    return false;
}

std::string Misc::ResourceHelpers::correctResourcePath(const std::string &topLevelDirectory, const std::string &resPath, const VFS::Manager* vfs)
{
    /* Bethesda at some point converted all their BSA
     * textures from tga to dds for increased load speed, but all
     * texture file name references were kept as .tga.
     */

    std::string prefix1 = topLevelDirectory + '\\';
    std::string prefix2 = topLevelDirectory + '/';

    std::string correctedPath = resPath;
    Misc::StringUtils::lowerCaseInPlace(correctedPath);

    // Apparently, leading separators are allowed
    while (correctedPath.size() && (correctedPath[0] == '/' || correctedPath[0] == '\\'))
        correctedPath.erase(0, 1);

    if(correctedPath.compare(0, prefix1.size(), prefix1.data()) != 0 &&
       correctedPath.compare(0, prefix2.size(), prefix2.data()) != 0)
        correctedPath = prefix1 + correctedPath;

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
    std::string fallback = topLevelDirectory + "\\" + getBasename(correctedPath);
    if (vfs->exists(fallback))
        return fallback;

    if (changedToDds)
    {
        fallback = topLevelDirectory + "\\" + getBasename(origExt);
        if (vfs->exists(fallback))
            return fallback;
    }

    return correctedPath;
}

std::string Misc::ResourceHelpers::correctTexturePath(const std::string &resPath, const VFS::Manager* vfs)
{
    static const std::string dir = "textures";
    return correctResourcePath(dir, resPath, vfs);
}

std::string Misc::ResourceHelpers::correctIconPath(const std::string &resPath, const VFS::Manager* vfs)
{
    static const std::string dir = "icons";
    return correctResourcePath(dir, resPath, vfs);
}

std::string Misc::ResourceHelpers::correctBookartPath(const std::string &resPath, const VFS::Manager* vfs)
{
    static const std::string dir = "bookart";
    std::string image = correctResourcePath(dir, resPath, vfs);

    return image;
}

std::string Misc::ResourceHelpers::correctBookartPath(const std::string &resPath, int width, int height, const VFS::Manager* vfs)
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

std::string Misc::ResourceHelpers::correctActorModelPath(const std::string &resPath, const VFS::Manager* vfs)
{
    std::string mdlname = resPath;
    std::string::size_type p = mdlname.find_last_of("/\\");
    if(p != std::string::npos)
        mdlname.insert(mdlname.begin()+p+1, 'x');
    else
        mdlname.insert(mdlname.begin(), 'x');
    if(!vfs->exists(mdlname))
    {
        return resPath;
    }
    return mdlname;
}
