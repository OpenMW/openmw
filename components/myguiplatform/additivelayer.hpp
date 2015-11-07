#ifndef OPENMW_COMPONENTS_MYGUIPLATFORM_ADDITIVELAYER
#define OPENMW_COMPONENTS_MYGUIPLATFORM_ADDITIVELAYER

#include <MyGUI_OverlappedLayer.h>

#include <osg/ref_ptr>

namespace osg
{
    class StateSet;
}

namespace osgMyGUI
{

    /// @brief A Layer rendering with additive blend mode.
    class AdditiveLayer : public MyGUI::OverlappedLayer
    {
    public:
        MYGUI_RTTI_DERIVED( AdditiveLayer )

        AdditiveLayer();
        ~AdditiveLayer();

        virtual void renderToTarget(MyGUI::IRenderTarget* _target, bool _update);

    private:
        osg::ref_ptr<osg::StateSet> mStateSet;
    };

}

#endif
