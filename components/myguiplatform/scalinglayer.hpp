#ifndef OPENMW_COMPONENTS_MYGUIPLATFORM_SCALINGLAYER
#define OPENMW_COMPONENTS_MYGUIPLATFORM_SCALINGLAYER

#include <MyGUI_OverlappedLayer.h>

namespace osgMyGUI
{

    ///@brief A Layer that lays out and renders widgets in screen-relative coordinates. The "Size" property determines the size of the virtual screen,
    /// which is then upscaled to the real screen size during rendering. The aspect ratio is kept intact, adding blanks to the sides when necessary.
    class ScalingLayer : public MyGUI::OverlappedLayer
    {
    public:
        MYGUI_RTTI_DERIVED(ScalingLayer)

        virtual void deserialization(MyGUI::xml::ElementPtr _node, MyGUI::Version _version);

        virtual MyGUI::ILayerItem* getLayerItemByPoint(int _left, int _top) const;
        virtual MyGUI::IntPoint getPosition(int _left, int _top) const;
        virtual void renderToTarget(MyGUI::IRenderTarget* _target, bool _update);

        virtual void resizeView(const MyGUI::IntSize& _viewSize);

    private:
        void screenToLayerCoords(int& _left, int& _top) const;
        float getScaleFactor() const;
    };

}

#endif
