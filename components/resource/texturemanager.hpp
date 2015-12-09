#ifndef OPENMW_COMPONENTS_RESOURCE_TEXTUREMANAGER_H
#define OPENMW_COMPONENTS_RESOURCE_TEXTUREMANAGER_H

#include <string>
#include <map>

#include <osg/ref_ptr>
#include <osg/Image>
#include <osg/Texture2D>

namespace VFS
{
    class Manager;
}

namespace Resource
{

    /// @brief Handles loading/caching of Images and Texture StateAttributes.
    class TextureManager
    {
    public:
        TextureManager(const VFS::Manager* vfs);
        ~TextureManager();

        /// @warning It is unsafe to call this function when a draw thread is using the textures. Call stopThreading() first!
        void setFilterSettings(osg::Texture::FilterMode minFilter, osg::Texture::FilterMode maxFilter, int maxAnisotropy);

        /// Keep a copy of the texture data around in system memory? This is needed when using multiple graphics contexts,
        /// otherwise should be disabled to reduce memory usage.
        void setUnRefImageDataAfterApply(bool unref);

        /// Store the paths to images in the osg::Image object?  Required for some situations involving serializing
        /// objects to disk, but consumes otherwise unnecessary memory.  Disabled by default.
        inline void setStoreImageFilenames(bool b) { mStoreImageFilenames = b; }

        /// Create or retrieve a Texture2D using the specified image filename, and wrap parameters.
        /// Returns the dummy texture if the given texture is not found.
        osg::ref_ptr<osg::Texture2D> getTexture2D(const std::string& filename, osg::Texture::WrapMode wrapS, osg::Texture::WrapMode wrapT);

        /// Create or retrieve an Image
        osg::ref_ptr<osg::Image> getImage(const std::string& filename);

        const VFS::Manager* getVFS() { return mVFS; }

        osg::Texture2D* getWarningTexture();

    private:
        const VFS::Manager* mVFS;

        osg::Texture::FilterMode mMinFilter;
        osg::Texture::FilterMode mMagFilter;
        int mMaxAnisotropy;

        typedef std::pair<std::pair<int, int>, std::string> MapKey;

        std::map<std::string, osg::ref_ptr<osg::Image> > mImages;

        std::map<MapKey, osg::ref_ptr<osg::Texture2D> > mTextures;

        osg::ref_ptr<osg::Texture2D> mWarningTexture;

        bool mUnRefImageDataAfterApply;
        bool mStoreImageFilenames;

        TextureManager(const TextureManager&);
        void operator = (const TextureManager&);
    };

}

#endif
