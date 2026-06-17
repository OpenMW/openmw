#ifndef OPENMW_COMPONENTS_MYGUIPLATFORM_SCALINGLAYER
#define OPENMW_COMPONENTS_MYGUIPLATFORM_SCALINGLAYER

#include <MyGUI_OverlappedLayer.h>

namespace MyGUIPlatform
{

    ///@brief A Layer that lays out and renders widgets in screen-relative coordinates. The "Size" property determines
    /// the size of the virtual screen,
    /// which is then upscaled to the real screen size during rendering. The aspect ratio is kept intact, adding blanks
    /// to the sides when necessary.
    class ScalingLayer final : public MyGUI::OverlappedLayer
    {
    public:
        MYGUI_RTTI_DERIVED(ScalingLayer)

        void deserialization(MyGUI::xml::ElementPtr node, MyGUI::Version version) override;

        MyGUI::ILayerItem* getLayerItemByPoint(int left, int top) const override;
        MyGUI::IntPoint getPosition(int left, int top) const override;
        void renderToTarget(MyGUI::IRenderTarget* target, bool update) override;

        void resizeView(const MyGUI::IntSize& viewSize) override;

        static float getScaleFactor(const MyGUI::IntSize& layerViewSize);

    private:
        void screenToLayerCoords(int& left, int& top) const;
    };

}

#endif
