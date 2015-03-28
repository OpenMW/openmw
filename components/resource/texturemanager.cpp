#include "texturemanager.hpp"

#include <osgDB/Registry>

#include <stdexcept>

#include <components/vfs/manager.hpp>

namespace Resource
{

    TextureManager::TextureManager(const VFS::Manager *vfs)
        : mVFS(vfs)
    {

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
            osg::ref_ptr<osgDB::Options> opts (new osgDB::Options);
            opts->setOptionString("dds_dxt1_detect_rgba"); // tx_creature_werewolf.dds isn't loading in the correct format without this option
            size_t extPos = normalized.find_last_of('.');
            std::string ext;
            if (extPos != std::string::npos && extPos+1 < normalized.size())
                ext = normalized.substr(extPos+1);
            osgDB::ReaderWriter* reader = osgDB::Registry::instance()->getReaderWriterForExtension(ext);
            osgDB::ReaderWriter::ReadResult result = reader->readImage(*mVFS->get(normalized.c_str()), opts);
            if (!result.success())
            {
                // TODO: use "notfound" default texture
                throw std::runtime_error("Error loading");
                //std::cerr << "Error loading " << filename << ": " << result.message() << std::endl;
            }

            osg::Image* image = result.getImage();
            osg::ref_ptr<osg::Texture2D> texture(new osg::Texture2D);
            texture->setImage(image);
            texture->setWrap(osg::Texture::WRAP_S, wrapS);
            texture->setWrap(osg::Texture::WRAP_T, wrapT);

            // Can be enabled for single-context, i.e. in openmw
            //texture->setUnRefImageDataAfterApply(true);

            mTextures.insert(std::make_pair(key, texture));
            return texture;
        }
    }

}
