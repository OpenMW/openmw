#include "texturemanager.hpp"

#include <osgDB/Registry>
#include <osg/GLExtensions>
#include <osg/Version>

#include <stdexcept>

#include <components/vfs/manager.hpp>

#ifdef OSG_LIBRARY_STATIC
// This list of plugins should match with the list in the top-level CMakelists.txt.
USE_OSGPLUGIN(png)
USE_OSGPLUGIN(tga)
USE_OSGPLUGIN(dds)
USE_OSGPLUGIN(jpeg)
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
        , mWarningTexture(createWarningTexture())
        , mOptions(new osgDB::Options("dds_flip dds_dxt1_detect_rgba"))
    {
    }

    TextureManager::~TextureManager()
    {

    }

    /*
    osg::ref_ptr<osg::Image> TextureManager::getImage(const std::string &filename)
    {

    }
    */

    bool checkSupported(osg::Image* image, const std::string& filename)
    {
        switch(image->getPixelFormat())
        {
            case(GL_COMPRESSED_RGB_S3TC_DXT1_EXT):
            case(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT):
            case(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT):
            case(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT):
            {
#if OSG_VERSION_GREATER_OR_EQUAL(3,3,3)
                osg::GLExtensions* exts = osg::GLExtensions::Get(0, false);
                if (exts && !exts->isTextureCompressionS3TCSupported
                        // This one works too. Should it be included in isTextureCompressionS3TCSupported()? Submitted as a patch to OSG.
                        && !osg::isGLExtensionSupported(0, "GL_S3_s3tc"))
#else
                osg::Texture::Extensions* exts = osg::Texture::getExtensions(0, false);
                if (exts && !exts->isTextureCompressionS3TCSupported()
                        // This one works too. Should it be included in isTextureCompressionS3TCSupported()? Submitted as a patch to OSG.
                        && !osg::isGLExtensionSupported(0, "GL_S3_s3tc"))
#endif
                {
                    std::cerr << "Error loading " << filename << ": no S3TC texture compression support installed" << std::endl;
                    return false;
                }
                break;
            }
            // not bothering with checks for other compression formats right now, we are unlikely to ever use those anyway
            default:
                return true;
        }
        return true;
    }

    osg::ref_ptr<osg::Image> TextureManager::getImage(const std::string &filename)
    {
        std::string normalized = filename;
        mVFS->normalizeFilename(normalized);
        std::map<std::string, osg::ref_ptr<osg::Image> >::iterator found = mImages.find(normalized);
        if (found != mImages.end())
            return found->second;
        else
        {
            Files::IStreamPtr stream;
            try
            {
                stream = mVFS->get(normalized.c_str());
            }
            catch (std::exception& e)
            {
                std::cerr << "Failed to open image: " << e.what() << std::endl;
                return NULL;
            }

            size_t extPos = normalized.find_last_of('.');
            std::string ext;
            if (extPos != std::string::npos && extPos+1 < normalized.size())
                ext = normalized.substr(extPos+1);
            osgDB::ReaderWriter* reader = osgDB::Registry::instance()->getReaderWriterForExtension(ext);
            if (!reader)
            {
                std::cerr << "Error loading " << filename << ": no readerwriter for '" << ext << "' found" << std::endl;
                return NULL;
            }

            osgDB::ReaderWriter::ReadResult result = reader->readImage(*stream, mOptions);
            if (!result.success())
            {
                std::cerr << "Error loading " << filename << ": " << result.message() << " code " << result.status() << std::endl;
                return NULL;
            }

            osg::Image* image = result.getImage();
            if (!checkSupported(image, filename))
            {
                return NULL;
            }

            mImages.insert(std::make_pair(normalized, image));
            return image;
        }
    }

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

            osgDB::ReaderWriter::ReadResult result = reader->readImage(*stream, mOptions);
            if (!result.success())
            {
                std::cerr << "Error loading " << filename << ": " << result.message() << " code " << result.status() << std::endl;
                return mWarningTexture;
            }

            osg::Image* image = result.getImage();
            if (!checkSupported(image, filename))
            {
                return mWarningTexture;
            }

            osg::ref_ptr<osg::Texture2D> texture(new osg::Texture2D);
            texture->setImage(image);
            texture->setWrap(osg::Texture::WRAP_S, wrapS);
            texture->setWrap(osg::Texture::WRAP_T, wrapT);

            mTextures.insert(std::make_pair(key, texture));
            return texture;
        }
    }

    osg::Texture2D* TextureManager::getWarningTexture()
    {
        return mWarningTexture.get();
    }

}
