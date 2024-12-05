#include <array>

#include <osgUtil/RenderBin>

namespace osg
{
    class FrameBufferObject;
}

namespace MWRender
{
    class DistortionCallback : public osgUtil::RenderBin::DrawCallback
    {
    public:
        void drawImplementation(
            osgUtil::RenderBin* bin, osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous) override;

        void setFBO(const osg::ref_ptr<osg::FrameBufferObject>& fbo, size_t frameId) { mFBO[frameId] = fbo; }
        void setOriginalFBO(const osg::ref_ptr<osg::FrameBufferObject>& fbo, size_t frameId)
        {
            mOriginalFBO[frameId] = fbo;
        }

    private:
        std::array<osg::ref_ptr<osg::FrameBufferObject>, 2> mFBO;
        std::array<osg::ref_ptr<osg::FrameBufferObject>, 2> mOriginalFBO;
    };
}
