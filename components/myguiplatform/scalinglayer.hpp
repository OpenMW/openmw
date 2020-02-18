#ifndef OPENMW_COMPONENTS_MYGUIPLATFORM_SCALINGLAYER
#define OPENMW_COMPONENTS_MYGUIPLATFORM_SCALINGLAYER

#include <MyGUI_OverlappedLayer.h>

namespace osgMyGUI
{

    ///@brief A Layer that lays out and renders widgets in screen-relative coordinates. The "Size" property determines the size of the virtual screen,
    /// which is then upscaled to the real screen size during rendering. The aspect ratio is kept intact, adding blanks to the sides when necessary.
    class ScalingLayer final : public MyGUI::OverlappedLayer
    {
    public:
        MYGUI_RTTI_DERIVED(ScalingLayer)

        void deserialization(MyGUI::xml::ElementPtr _node, MyGUI::Version _version) final;

        MyGUI::ILayerItem* getLayerItemByPoint(int _left, int _top) const final;
        MyGUI::IntPoint getPosition(int _left, int _top) const final;
        void renderToTarget(MyGUI::IRenderTarget* _target, bool _update) final;

        void resizeView(const MyGUI::IntSize& _viewSize) final;

    private:
        void screenToLayerCoords(int& _left, int& _top) const;
        float getScaleFactor() const;
    };

}

#endif
