#include "resourcehelpers.hpp"

#include <components/misc/stringops.hpp>

#include <OgreResourceGroupManager.h>

bool Misc::ResourceHelpers::changeExtensionToDds(std::string &path)
{
    Ogre::String::size_type pos = path.rfind('.');
    if(pos != Ogre::String::npos && path.compare(pos, path.length() - pos, ".dds") != 0)
    {
        path.replace(pos, path.length(), ".dds");
        return true;
    }
    return false;
}

std::string Misc::ResourceHelpers::correctResourcePath(const std::string &topLevelDirectory, const std::string &resPath)
{
    /* Bethesda at some point converted all their BSA
     * textures from tga to dds for increased load speed, but all
     * texture file name references were kept as .tga.
     */

    std::string prefix1 = topLevelDirectory + '\\';
    std::string prefix2 = topLevelDirectory + '/';

    std::string correctedPath = resPath;
    Misc::StringUtils::toLower(correctedPath);

    // Apparently, leading separators are allowed
    while (correctedPath.size() && (correctedPath[0] == '/' || correctedPath[0] == '\\'))
        correctedPath.erase(0, 1);

    if(correctedPath.compare(0, prefix1.size(), prefix1.data()) != 0 &&
       correctedPath.compare(0, prefix2.size(), prefix2.data()) != 0)
        correctedPath = prefix1 + correctedPath;

    std::string origExt = correctedPath;

    // since we know all (GOTY edition or less) textures end
    // in .dds, we change the extension
    if (changeExtensionToDds(correctedPath))
    {
        // if it turns out that the above wasn't true in all cases (not for vanilla, but maybe mods)
        // verify, and revert if false (this call succeeds quickly, but fails slowly)
        if(!Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(correctedPath))
        {
            return origExt;
        }
    }

    return correctedPath;
}

std::string Misc::ResourceHelpers::correctTexturePath(const std::string &resPath)
{
    static const std::string dir = "textures";
    return correctResourcePath(dir, resPath);
}

std::string Misc::ResourceHelpers::correctIconPath(const std::string &resPath)
{
    static const std::string dir = "icons";
    return correctResourcePath(dir, resPath);
}

std::string Misc::ResourceHelpers::correctBookartPath(const std::string &resPath)
{
    static const std::string dir = "bookart";
    std::string image = correctResourcePath(dir, resPath);

    return image;
}

std::string Misc::ResourceHelpers::correctBookartPath(const std::string &resPath, int width, int height)
{
    std::string image = correctBookartPath(resPath);

    // Apparently a bug with some morrowind versions, they reference the image without the size suffix.
    // So if the image isn't found, try appending the size.
    if (!Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(image))
    {
        std::stringstream str;
        str << image.substr(0, image.rfind('.')) << "_" << width << "_" << height << image.substr(image.rfind('.'));
        image = Misc::ResourceHelpers::correctBookartPath(str.str());
    }

    return image;
}
