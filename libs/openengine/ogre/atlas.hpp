#ifndef OENGINE_OGRE_ATLAS_HPP
#define OENGINE_OGRE_ATLAS_HPP

#include <string>

namespace OEngine
{
namespace Render
{

    /// \brief Creates a texture atlas at runtime
    class Atlas
    {
    public:
        /**
         * @param absolute path to file that specifies layout of the texture (positions of the textures it contains)
         * @param name of the destination texture to save to (in memory)
         * @param texture directory prefix
         */
        static void createFromFile (const std::string& filename, const std::string& textureName, const std::string& texturePrefix="textures\\");
    };

}
}

#endif

