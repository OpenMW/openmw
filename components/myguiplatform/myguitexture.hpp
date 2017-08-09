#ifndef OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUITEXTURE_H
#define OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUITEXTURE_H

#include <MyGUI_ITexture.h>

#include <osg/ref_ptr>

namespace osg
{
    class Image;
    class Texture2D;
}

namespace Resource
{
    class ImageManager;
}

namespace osgMyGUI
{

    class OSGTexture : public MyGUI::ITexture {
        std::string mName;
        Resource::ImageManager* mImageManager;

        osg::ref_ptr<osg::Image> mLockedImage;
        osg::ref_ptr<osg::Texture2D> mTexture;
        MyGUI::PixelFormat mFormat;
        MyGUI::TextureUsage mUsage;
        size_t mNumElemBytes;

        int mWidth;
        int mHeight;

    public:
        OSGTexture(const std::string &name, Resource::ImageManager* imageManager);
        OSGTexture(osg::Texture2D* texture);
        virtual ~OSGTexture();

        virtual const std::string& getName() const { return mName; }

        virtual void createManual(int width, int height, MyGUI::TextureUsage usage, MyGUI::PixelFormat format);
        virtual void loadFromFile(const std::string &fname);
        virtual void saveToFile(const std::string &fname);

        virtual void destroy();

        virtual void* lock(MyGUI::TextureUsage access);
        virtual void unlock();
        virtual bool isLocked();

        virtual int getWidth();
        virtual int getHeight();

        virtual MyGUI::PixelFormat getFormat() { return mFormat; }
        virtual MyGUI::TextureUsage getUsage() { return mUsage; }
        virtual size_t getNumElemBytes() { return mNumElemBytes; }

        virtual MyGUI::IRenderTarget *getRenderTarget();

    /*internal:*/
        osg::Texture2D *getTexture() const { return mTexture.get(); }
    };

}

#endif
