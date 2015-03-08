#ifndef OENGINE_OGRE_IMAGEROTATE_HPP
#define OENGINE_OGRE_IMAGEROTATE_HPP

#include <string>


namespace SFO
{

    /// Rotate an image by certain degrees and save as file, uses the GPU
    /// Make sure Ogre Root is initialised before calling
    class ImageRotate
    {
    public:
        /**
         * @param source image (file name - has to exist in an resource group)
         * @param name of the destination texture to save to (in memory)
         * @param angle in degrees to turn
         */
        static void rotate(const std::string& sourceImage, const std::string& destImage, const float angle);
    };

}

#endif
