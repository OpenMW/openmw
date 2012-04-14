
#ifndef MWGUI_TOOLTIPS_H
#define MWGUI_TOOLTIPS_H

#include <openengine/gui/layout.hpp>

namespace MWGui
{
    class ToolTips : public OEngine::GUI::Layout
    {
    public:
        ToolTips();

        void onFrame(float frameDuration);

        void enterGameMode();
        void enterGuiMode();

        void adjustScreen(int screenWidth, int screenHeight);

    private:
        MyGUI::EditBox* mTextToolTip;
        MyGUI::Widget* mTextToolTipBox;

        MyGUI::Widget* mDynamicToolTipBox;

        bool mGameMode;
    };
}
#endif
