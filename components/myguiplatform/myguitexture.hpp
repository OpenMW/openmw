#ifndef OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUITEXTURE_H
#define OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUITEXTURE_H

#include <MyGUI_ITexture.h>

#include <osg/ref_ptr>

namespace osg
{
    class Image;
    class Texture2D;
    class StateSet;
}

namespace Resource
{
    class ImageManager;
}

namespace MyGUIPlatform
{

    class OSGTexture final : public MyGUI::ITexture
    {
        std::string mName;
        Resource::ImageManager* mImageManager;

        osg::ref_ptr<osg::Image> mLockedImage;
        osg::ref_ptr<osg::Texture2D> mTexture;
        osg::ref_ptr<osg::StateSet> mInjectState;
        MyGUI::PixelFormat mFormat;
        MyGUI::TextureUsage mUsage;
        size_t mNumElemBytes;

        int mWidth;
        int mHeight;

    public:
        OSGTexture(const std::string& name, Resource::ImageManager* imageManager);
        OSGTexture(osg::Texture2D* texture, osg::StateSet* injectState = nullptr);
        ~OSGTexture() override;

        osg::StateSet* getInjectState() { return mInjectState; }

        const std::string& getName() const override { return mName; }

        void createManual(int width, int height, MyGUI::TextureUsage usage, MyGUI::PixelFormat format) override;
        void loadFromFile(const std::string& fname) override;
        void saveToFile(const std::string& fname) override;

        void destroy() override;

        void* lock(MyGUI::TextureUsage access) override;
        void unlock() override;
        bool isLocked() const override { return mLockedImage.valid(); }

        int getWidth() const override { return mWidth; }
        int getHeight() const override { return mHeight; }

        MyGUI::PixelFormat getFormat() const override { return mFormat; }
        MyGUI::TextureUsage getUsage() const override { return mUsage; }
        size_t getNumElemBytes() const override { return mNumElemBytes; }

        MyGUI::IRenderTarget* getRenderTarget() override;

        void setShader(const std::string& _shaderName) override;

        /*internal:*/
        osg::Texture2D* getTexture() const { return mTexture.get(); }
    };

}

#endif
