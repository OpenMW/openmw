#ifndef OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUITEXTURE_H
#define OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUITEXTURE_H

#include <MyGUI_ITexture.h>

#include <osg/ref_ptr>

#if MYGUI_VERSION > MYGUI_DEFINE_VERSION(3, 4, 0)
    #define OPENMW_MYGUI_CONST_GETTER_3_4_1 const
#else
    #define OPENMW_MYGUI_CONST_GETTER_3_4_1
#endif

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

        const std::string& getName() const override { return mName; }

        void createManual(int width, int height, MyGUI::TextureUsage usage, MyGUI::PixelFormat format) override;
        void loadFromFile(const std::string &fname) override;
        void saveToFile(const std::string &fname) override;

        void destroy() override;

        void* lock(MyGUI::TextureUsage access) override;
        void unlock() override;
        bool isLocked() OPENMW_MYGUI_CONST_GETTER_3_4_1 override { return mLockedImage.valid(); }

        int getWidth() OPENMW_MYGUI_CONST_GETTER_3_4_1 override { return mWidth; }
        int getHeight() OPENMW_MYGUI_CONST_GETTER_3_4_1 override { return mHeight; }

        MyGUI::PixelFormat getFormat() OPENMW_MYGUI_CONST_GETTER_3_4_1 override { return mFormat; }
        MyGUI::TextureUsage getUsage() OPENMW_MYGUI_CONST_GETTER_3_4_1 override { return mUsage; }
        size_t getNumElemBytes() OPENMW_MYGUI_CONST_GETTER_3_4_1 override { return mNumElemBytes; }

        MyGUI::IRenderTarget *getRenderTarget() override;

    // setShader() is a part of MyGUI::RenderManager interface since 3.4.1 release
#if MYGUI_VERSION > MYGUI_DEFINE_VERSION(3, 4, 0)
    void setShader(const std::string& _shaderName) override;
#endif

    /*internal:*/
        osg::Texture2D *getTexture() const { return mTexture.get(); }
    };

}

#endif
