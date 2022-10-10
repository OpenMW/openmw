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
            SelectColorFormatOperation()
                : GraphicsOperation("SelectColorFormatOperation", false)
            {
            }

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

#endif
