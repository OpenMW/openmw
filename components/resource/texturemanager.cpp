#include "texturemanager.hpp"

#include <osgDB/Registry>

#include <stdexcept>

#include <components/vfs/manager.hpp>
#if ANDROID
#include <components/glesextensions/texturecompression.h>
#endif
namespace
{

    osg::ref_ptr<osg::Texture2D> createWarningTexture()
    {
        osg::ref_ptr<osg::Image> warningImage = new osg::Image;

        int width = 8, height = 8;
        warningImage->allocateImage(width, height, 1, GL_RGB, GL_UNSIGNED_BYTE);
        assert (warningImage->isDataContiguous());
        unsigned char* data = warningImage->data();
        for (int i=0;i<width*height;++i)
        {
            data[3*i] = (255);
            data[3*i+1] = (0);
            data[3*i+2] = (255);
        }

        osg::ref_ptr<osg::Texture2D> warningTexture = new osg::Texture2D;
        warningTexture->setImage(warningImage);
        return warningTexture;
    }

}

namespace Resource
{

    TextureManager::TextureManager(const VFS::Manager *vfs)
        : mVFS(vfs)
        , mMinFilter(osg::Texture::LINEAR_MIPMAP_LINEAR)
        , mMagFilter(osg::Texture::LINEAR)
        , mMaxAnisotropy(1)
        , mWarningTexture(createWarningTexture())
        , mUnRefImageDataAfterApply(false)
    {

    }

    TextureManager::~TextureManager()
    {

    }

    void TextureManager::setUnRefImageDataAfterApply(bool unref)
    {
        mUnRefImageDataAfterApply = unref;
    }

    void TextureManager::setFilterSettings(osg::Texture::FilterMode minFilter, osg::Texture::FilterMode magFilter, int maxAnisotropy)
    {
        mMinFilter = minFilter;
        mMagFilter = magFilter;
        mMaxAnisotropy = std::max(1, maxAnisotropy);

        for (std::map<MapKey, osg::ref_ptr<osg::Texture2D> >::iterator it = mTextures.begin(); it != mTextures.end(); ++it)
        {
            osg::ref_ptr<osg::Texture2D> tex = it->second;
            tex->setFilter(osg::Texture::MIN_FILTER, mMinFilter);
            tex->setFilter(osg::Texture::MAG_FILTER, mMagFilter);
            tex->setMaxAnisotropy(static_cast<float>(mMaxAnisotropy));
#if ANDROID
            tex-> setInternalFormatMode(GlesTextureCompression::getGLesOsgTextureCompression());
#endif
        }
    }

    /*
    osg::ref_ptr<osg::Image> TextureManager::getImage(const std::string &filename)
    {

    }
    */

    osg::ref_ptr<osg::Texture2D> TextureManager::getTexture2D(const std::string &filename, osg::Texture::WrapMode wrapS, osg::Texture::WrapMode wrapT)
    {
        std::string normalized = filename;
        mVFS->normalizeFilename(normalized);
        MapKey key = std::make_pair(std::make_pair(wrapS, wrapT), normalized);
        std::map<MapKey, osg::ref_ptr<osg::Texture2D> >::iterator found = mTextures.find(key);
        if (found != mTextures.end())
        {
            return found->second;
        }
        else
        {
            Files::IStreamPtr stream;
            try
            {
                stream = mVFS->get(normalized.c_str());
            }
            catch (std::exception& e)
            {
                std::cerr << "Failed to open texture: " << e.what() << std::endl;
                return mWarningTexture;
            }

            osg::ref_ptr<osgDB::Options> opts (new osgDB::Options);
            opts->setOptionString("dds_dxt1_detect_rgba"); // tx_creature_werewolf.dds isn't loading in the correct format without this option
            size_t extPos = normalized.find_last_of('.');
            std::string ext;
            if (extPos != std::string::npos && extPos+1 < normalized.size())
                ext = normalized.substr(extPos+1);
            osgDB::ReaderWriter* reader = osgDB::Registry::instance()->getReaderWriterForExtension(ext);
            if (!reader)
            {
                std::cerr << "Error loading " << filename << ": no readerwriter for '" << ext << "' found" << std::endl;
                return mWarningTexture;
            }

            osgDB::ReaderWriter::ReadResult result = reader->readImage(*stream, opts);
            if (!result.success())
            {
                std::cerr << "Error loading " << filename << ": " << result.message() << std::endl;
                return mWarningTexture;
            }

            osg::Image* image = result.getImage();

            // We need to flip images, because the Morrowind texture coordinates use the DirectX convention (top-left image origin),
            // but OpenGL uses bottom left as the image origin.
            // For some reason this doesn't concern DDS textures, which are already flipped when loaded.
            if (ext != "dds")
            {
                image->flipVertical();
            }

            osg::ref_ptr<osg::Texture2D> texture(new osg::Texture2D);
            texture->setImage(image);
            texture->setWrap(osg::Texture::WRAP_S, wrapS);
            texture->setWrap(osg::Texture::WRAP_T, wrapT);
            texture->setFilter(osg::Texture::MIN_FILTER, mMinFilter);
            texture->setFilter(osg::Texture::MAG_FILTER, mMagFilter);
            texture->setMaxAnisotropy(mMaxAnisotropy);
#if ANDROID
            texture-> setInternalFormatMode(GlesTextureCompression::getGLesOsgTextureCompression());
#endif
            texture->setUnRefImageDataAfterApply(mUnRefImageDataAfterApply);

            mTextures.insert(std::make_pair(key, texture));
            return texture;
        }
    }

    osg::Texture2D* TextureManager::getWarningTexture()
    {
        return mWarningTexture.get();
    }

}
