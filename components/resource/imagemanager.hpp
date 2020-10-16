#ifndef OPENMW_COMPONENTS_RESOURCE_IMAGEMANAGER_H
#define OPENMW_COMPONENTS_RESOURCE_IMAGEMANAGER_H

#include <string>
#include <map>

#include <osg/ref_ptr>
#include <osg/Image>
#include <osg/Texture2D>

#include "resourcemanager.hpp"

namespace osgDB
{
    class Options;
}

namespace Resource
{

    /// @brief Handles loading/caching of Images.
    /// @note May be used from any thread.
    class ImageManager : public ResourceManager
    {
    public:
        ImageManager(const VFS::Manager* vfs);
        ~ImageManager();

        /// Create or retrieve an Image
        /// Returns the dummy image if the given image is not found.
        osg::ref_ptr<osg::Image> getImage(const std::string& filename);

        osg::Image* getWarningImage();

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const override;

    private:
        osg::ref_ptr<osg::Image> mWarningImage;
        osg::ref_ptr<osgDB::Options> mOptions;

        ImageManager(const ImageManager&);
        void operator = (const ImageManager&);
    };

}

#endif
