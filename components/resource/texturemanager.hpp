#ifndef OPENMW_COMPONENTS_RESOURCE_TEXTUREMANAGER_H
#define OPENMW_COMPONENTS_RESOURCE_TEXTUREMANAGER_H

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
    class TextureManager
    {
    public:
        TextureManager(const VFS::Manager* vfs);
        ~TextureManager();

        /// Create or retrieve an Image
        /// Returns the dummy image if the given image is not found.
        osg::ref_ptr<osg::Image> getImage(const std::string& filename);

        const VFS::Manager* getVFS() { return mVFS; }

        osg::Texture2D* getWarningTexture();

    private:
        const VFS::Manager* mVFS;

        // TODO: use ObjectCache
        std::map<std::string, osg::ref_ptr<osg::Image> > mImages;

        osg::ref_ptr<osg::Texture2D> mWarningTexture;
        osg::ref_ptr<osg::Image> mWarningImage;
        osg::ref_ptr<osgDB::Options> mOptions;

        TextureManager(const TextureManager&);
        void operator = (const TextureManager&);
    };

}

#endif
