#include "scalinglayer.hpp"

#include <MyGUI_RenderManager.h>
#include <algorithm>

namespace MyGUIPlatform
{

    /// @brief the ProxyRenderTarget allows to adjust the pixel scale and offset for a "source" render target.
    class ProxyRenderTarget : public MyGUI::IRenderTarget
    {
    public:
        /// @param target The target to render to.
        /// @param viewSize The size of the underlying layer node to render.
        /// @param hoffset The horizontal rendering offset, specified as an offset from the left screen edge in range
        /// 0-1.
        /// @param voffset The vertical rendering offset, specified as an offset from the top screen edge in range 0-1.
        ProxyRenderTarget(MyGUI::IRenderTarget* target, MyGUI::IntSize viewSize, float hoffset, float voffset)
            : mTarget(target)
            , mViewSize(viewSize)
            , mHOffset(hoffset)
            , mVOffset(voffset)
        {
        }

        void begin() override { mTarget->begin(); }

        void end() override { mTarget->end(); }

        void doRender(MyGUI::IVertexBuffer* buffer, MyGUI::ITexture* texture, size_t count) override
        {
            mTarget->doRender(buffer, texture, count);
        }

        const MyGUI::RenderTargetInfo& getInfo() const override
        {
            mInfo = mTarget->getInfo();
            mInfo.hOffset = mHOffset;
            mInfo.vOffset = mVOffset;
            mInfo.pixScaleX = 1.f / mViewSize.width;
            mInfo.pixScaleY = 1.f / mViewSize.height;
            return mInfo;
        }

    private:
        MyGUI::IRenderTarget* mTarget;
        MyGUI::IntSize mViewSize;
        float mHOffset, mVOffset;
        mutable MyGUI::RenderTargetInfo mInfo;
    };

    MyGUI::ILayerItem* ScalingLayer::getLayerItemByPoint(int left, int top) const
    {
        screenToLayerCoords(left, top);

        return OverlappedLayer::getLayerItemByPoint(left, top);
    }

    void ScalingLayer::screenToLayerCoords(int& left, int& top) const
    {
        float scale = getScaleFactor(mViewSize);
        if (scale <= 0.f)
            return;

        MyGUI::IntSize globalViewSize = MyGUI::RenderManager::getInstance().getViewSize();

        left -= globalViewSize.width / 2;
        top -= globalViewSize.height / 2;

        left = static_cast<int>(left / scale);
        top = static_cast<int>(top / scale);

        left += mViewSize.width / 2;
        top += mViewSize.height / 2;
    }

    float ScalingLayer::getScaleFactor(const MyGUI::IntSize& layerViewSize)
    {
        MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        float w = static_cast<float>(viewSize.width);
        float h = static_cast<float>(viewSize.height);

        float heightScale = (h / layerViewSize.height);
        float widthScale = (w / layerViewSize.width);
        return std::min(widthScale, heightScale);
    }

    MyGUI::IntPoint ScalingLayer::getPosition(int left, int top) const
    {
        screenToLayerCoords(left, top);
        return MyGUI::IntPoint(left, top);
    }

    void ScalingLayer::renderToTarget(MyGUI::IRenderTarget* target, bool update)
    {
        MyGUI::IntSize globalViewSize = MyGUI::RenderManager::getInstance().getViewSize();
        MyGUI::IntSize viewSize = globalViewSize;
        float scale = getScaleFactor(mViewSize);
        viewSize.width = static_cast<int>(viewSize.width / scale);
        viewSize.height = static_cast<int>(viewSize.height / scale);

        float hoffset = (globalViewSize.width - mViewSize.width * getScaleFactor(mViewSize)) / 2.f
            / static_cast<float>(globalViewSize.width);
        float voffset = (globalViewSize.height - mViewSize.height * getScaleFactor(mViewSize)) / 2.f
            / static_cast<float>(globalViewSize.height);

        ProxyRenderTarget proxy(target, viewSize, hoffset, voffset);

        MyGUI::OverlappedLayer::renderToTarget(&proxy, update);
    }

    void ScalingLayer::resizeView(const MyGUI::IntSize& /*viewSize*/)
    {
        // do nothing
    }

    void ScalingLayer::deserialization(MyGUI::xml::ElementPtr node, MyGUI::Version version)
    {
        MyGUI::OverlappedLayer::deserialization(node, version);

        MyGUI::xml::ElementEnumerator info = node->getElementEnumerator();
        while (info.next())
        {
            if (info->getName() == "Property")
            {
                auto key = info->findAttribute("key");
                auto value = info->findAttribute("value");

                if (key == "Size")
                {
                    mViewSize = MyGUI::IntSize::parse(value);
                }
            }
        }
    }

}
