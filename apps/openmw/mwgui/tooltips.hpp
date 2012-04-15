
#ifndef MWGUI_TOOLTIPS_H
#define MWGUI_TOOLTIPS_H

#include <openengine/gui/layout.hpp>
#include "../mwworld/ptr.hpp"

namespace MWGui
{
    class ToolTips : public OEngine::GUI::Layout
    {
    public:
        ToolTips();

        void onFrame(float frameDuration);

        void enterGameMode();
        void enterGuiMode();

        void setFocusObject(const MWWorld::Ptr& focus);

        void adjustScreen(int screenWidth, int screenHeight);

    private:
        MyGUI::EditBox* mTextToolTip;
        MyGUI::Widget* mTextToolTipBox;

        MyGUI::Widget* mDynamicToolTipBox;

        MWWorld::Ptr mFocusObject;
        bool mFocusChanged;

        bool mGameMode;
    };
}
#endif
