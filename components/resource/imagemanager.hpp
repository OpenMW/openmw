#ifndef OPENMW_COMPONENTS_RESOURCE_IMAGEMANAGER_H
#define OPENMW_COMPONENTS_RESOURCE_IMAGEMANAGER_H

#include <string>
#include <map>

#include <osg/ref_ptr>
#include <osg/Image>
#include <osg/Texture2D>

namespace osgViewer
{
    class Viewer;
}

namespace VFS
{
    class Manager;
}

namespace osgDB
{
    class Options;
}

namespace Resource
{

    /// @brief Handles loading/caching of Images.
    class ImageManager
    {
    public:
        ImageManager(const VFS::Manager* vfs);
        ~ImageManager();

        /// Create or retrieve an Image
        /// Returns the dummy image if the given image is not found.
        osg::ref_ptr<osg::Image> getImage(const std::string& filename);

        const VFS::Manager* getVFS() { return mVFS; }

        osg::Image* getWarningImage();

    private:
        const VFS::Manager* mVFS;

        // TODO: use ObjectCache
        std::map<std::string, osg::ref_ptr<osg::Image> > mImages;

        osg::ref_ptr<osg::Image> mWarningImage;
        osg::ref_ptr<osgDB::Options> mOptions;

        ImageManager(const ImageManager&);
        void operator = (const ImageManager&);
    };

}

#endif
