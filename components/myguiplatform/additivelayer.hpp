#ifndef OPENMW_COMPONENTS_MYGUIPLATFORM_ADDITIVELAYER
#define OPENMW_COMPONENTS_MYGUIPLATFORM_ADDITIVELAYER

#include <MyGUI_OverlappedLayer.h>

#include <osg/ref_ptr>

namespace osg
{
    class StateSet;
}

namespace MyGUIPlatform
{

    /// @brief A Layer rendering with additive blend mode.
    class AdditiveLayer final : public MyGUI::OverlappedLayer
    {
    public:
        MYGUI_RTTI_DERIVED(AdditiveLayer)

        AdditiveLayer();
        ~AdditiveLayer() override;

        void renderToTarget(MyGUI::IRenderTarget* target, bool update) override;

    private:
        osg::ref_ptr<osg::StateSet> mStateSet;
    };

}

#endif
