#include "resourcehelpers.hpp"

#include <components/misc/stringops.hpp>

#include <OgreMaterialManager.h>

std::string Misc::ResourceHelpers::correctResourcePath(const std::string &topLevelDirectory, const std::string &filename)
{
    /* Bethesda at some point converted all their BSA
     * textures from tga to dds for increased load speed, but all
     * texture file name references were kept as .tga.
     */
    std::string path = topLevelDirectory + '\\';
    std::string path2 = topLevelDirectory + '/';

    std::string texname = filename;
    Misc::StringUtils::toLower(texname);

    // Apparently, leading separators are allowed
    while (texname.size() && (texname[0] == '/' || texname[0] == '\\'))
        texname.erase(0, 1);

    if(texname.compare(0, path.size()-1, path.data()) != 0 &&
       texname.compare(0, path2.size()-1, path2.data()) != 0)
        texname = path + texname;

    Ogre::String::size_type pos = texname.rfind('.');
    if(pos != Ogre::String::npos && texname.compare(pos, texname.length() - pos, ".dds") != 0)
    {
        // since we know all (GOTY edition or less) textures end
        // in .dds, we change the extension
        texname.replace(pos, texname.length(), ".dds");

        // if it turns out that the above wasn't true in all cases (not for vanilla, but maybe mods)
        // verify, and revert if false (this call succeeds quickly, but fails slowly)
        if(!Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(texname))
        {
            texname = filename;
            Misc::StringUtils::toLower(texname);
            if(texname.compare(0, path.size()-1, path.data()) != 0 &&
               texname.compare(0, path2.size()-1, path2.data()) != 0)
                texname = path + texname;
        }
    }

    return texname;
}

std::string Misc::ResourceHelpers::correctTexturePath(const std::string &filename)
{
    static const std::string dir = "textures";
    return correctResourcePath(dir, filename);
}

std::string Misc::ResourceHelpers::correctIconPath(const std::string &filename)
{
    static const std::string dir = "icons";
    return correctResourcePath(dir, filename);
}
