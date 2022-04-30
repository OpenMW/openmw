#ifndef OPENMW_COMPONENTS_SCENEUTIL_COLOR_H
#define OPENMW_COMPONENTS_SCENEUTIL_COLOR_H

#include <osg/GraphicsThread>

namespace SceneUtil
{
    bool isColorFormat(GLenum format);
    bool isFloatingPointColorFormat(GLenum format);
    int getColorFormatChannelCount(GLenum format);
    void getColorFormatSourceFormatAndType(GLenum internalFormat, GLenum& sourceFormat, GLenum& sourceType);

    namespace Color
    {
        GLenum colorSourceFormat();
        GLenum colorSourceType();
        GLenum colorInternalFormat();

        class SelectColorFormatOperation final : public osg::GraphicsOperation
        {
        public:
            SelectColorFormatOperation() : GraphicsOperation("SelectColorFormatOperation", false)
            {}

            void operator()(osg::GraphicsContext* graphicsContext) override;

            void setSupportedFormats(const std::vector<GLenum>& supportedFormats)
            {
                mSupportedFormats = supportedFormats;
            }

        private:
            std::vector<GLenum> mSupportedFormats;
        };
    }
}

#ifndef GL_RGB
#define GL_RGB 0x1907
#endif

#ifndef GL_RGBA
#define GL_RGBA 0x1908
#endif

#ifndef GL_RGB4
#define GL_RGB4 0x804F
#endif

#ifndef GL_RGB5
#define GL_RGB5 0x8050
#endif

#ifndef GL_RGB8
#define GL_RGB8 0x8051
#endif

#ifndef GL_RGB8_SNORM
#define GL_RGB8_SNORM 0x8F96
#endif

#ifndef GL_RGB10
#define GL_RGB10 0x8052
#endif

#ifndef GL_RGB12
#define GL_RGB12 0x8053
#endif

#ifndef GL_RGB16
#define GL_RGB16 0x8054
#endif

#ifndef GL_RGB16_SNORM
#define GL_RGB16_SNORM 0x8F9A
#endif

#ifndef GL_RGBA2
#define GL_RGBA2 0x8055
#endif

#ifndef GL_RGBA4
#define GL_RGBA4 0x8056
#endif

#ifndef GL_RGB5_A1
#define GL_RGB5_A1 0x8057
#endif

#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif

#ifndef GL_RGBA8_SNORM
#define GL_RGBA8_SNORM 0x8F97
#endif

#ifndef GL_RGB10_A2
#define GL_RGB10_A2 0x906F
#endif

#ifndef GL_RGB10_A2UI 
#define GL_RGB10_A2UI  0x906F
#endif

#ifndef GL_RGBA12
#define GL_RGBA12 0x805A
#endif

#ifndef GL_RGBA16
#define GL_RGBA16 0x805B
#endif

#ifndef GL_RGBA16_SNORM
#define GL_RGBA16_SNORM 0x8F9B
#endif

#ifndef GL_SRGB
#define GL_SRGB 0x8C40
#endif

#ifndef GL_SRGB8
#define GL_SRGB8 0x8C41
#endif

#ifndef GL_SRGB_ALPHA8
#define GL_SRGB_ALPHA8 0x8C42
#endif

#ifndef GL_SRGB8_ALPHA8
#define GL_SRGB8_ALPHA8 0x8C43
#endif

#ifndef GL_RGB16F
#define GL_RGB16F 0x881B
#endif

#ifndef GL_RGBA16F
#define GL_RGBA16F 0x881A
#endif

#ifndef GL_RGB32F
#define GL_RGB32F 0x8815
#endif

#ifndef GL_RGBA32F
#define GL_RGBA32F 0x8814
#endif

#ifndef GL_R11F_G11F_B10F
#define GL_R11F_G11F_B10F 0x8C3A
#endif


#ifndef GL_RGB8I
#define GL_RGB8I 0x8D8F
#endif

#ifndef GL_RGB8UI
#define GL_RGB8UI 0x8D7D
#endif

#ifndef GL_RGB16I
#define GL_RGB16I 0x8D89
#endif

#ifndef GL_RGB16UI
#define GL_RGB16UI 0x8D77
#endif

#ifndef GL_RGB32I
#define GL_RGB32I 0x8D83
#endif

#ifndef GL_RGB32UI
#define GL_RGB32UI 0x8D71
#endif

#ifndef GL_RGBA8I
#define GL_RGBA8I 0x8D8E
#endif

#ifndef GL_RGBA8UI
#define GL_RGBA8UI 0x8D7C
#endif

#ifndef GL_RGBA16I
#define GL_RGBA16I 0x8D88
#endif

#ifndef GL_RGBA16UI
#define GL_RGBA16UI 0x8D76
#endif

#ifndef GL_RGBA32I
#define GL_RGBA32I 0x8D82
#endif

#ifndef GL_RGBA32UI
#define GL_RGBA32UI 0x8D70
#endif

#ifndef GL_RGB9_E5
#define GL_RGB9_E5 0x8C3D
#endif

#endif
