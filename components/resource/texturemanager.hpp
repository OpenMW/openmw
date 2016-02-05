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

    /// @brief Handles loading/caching of Images and Texture StateAttributes.
    class TextureManager
    {
    public:
        TextureManager(const VFS::Manager* vfs);
        ~TextureManager();

        /// Create or retrieve a Texture2D using the specified image filename, and wrap parameters.
        /// Returns the dummy texture if the given texture is not found.
        osg::ref_ptr<osg::Texture2D> getTexture2D(const std::string& filename, osg::Texture::WrapMode wrapS, osg::Texture::WrapMode wrapT);

        /// Create or retrieve an Image
        osg::ref_ptr<osg::Image> getImage(const std::string& filename);

        const VFS::Manager* getVFS() { return mVFS; }

        osg::Texture2D* getWarningTexture();

    private:
        const VFS::Manager* mVFS;

        typedef std::pair<std::pair<int, int>, std::string> MapKey;

        std::map<std::string, osg::ref_ptr<osg::Image> > mImages;

        std::map<MapKey, osg::ref_ptr<osg::Texture2D> > mTextures;

        osg::ref_ptr<osg::Texture2D> mWarningTexture;
        osg::ref_ptr<osgDB::Options> mOptions;

        TextureManager(const TextureManager&);
        void operator = (const TextureManager&);
    };

}

#endif
