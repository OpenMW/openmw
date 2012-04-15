
#ifndef MWGUI_TOOLTIPS_H
#define MWGUI_TOOLTIPS_H

#include <openengine/gui/layout.hpp>
#include "../mwworld/ptr.hpp"

namespace MWGui
{
    class WindowManager;

    class ToolTips : public OEngine::GUI::Layout
    {
    public:
        ToolTips(WindowManager* windowManager);

        void onFrame(float frameDuration);

        void enterGameMode();
        void enterGuiMode();

        void setFocusObject(const MWWorld::Ptr& focus);

    private:
        MyGUI::Widget* mDynamicToolTipBox;

        WindowManager* mWindowManager;

        MWWorld::Ptr mFocusObject;

        void findImageExtension(std::string& image);

        MyGUI::IntSize getToolTipViaPtr ();
        ///< @return requested tooltip size

        MyGUI::IntSize createImageToolTip(const std::string& caption, const std::string& image, const std::string& text);
        ///< @return requested tooltip size

        MyGUI::IntSize createToolTip(const std::string& caption, const std::string& text);
        ///< @return requested tooltip size

        MyGUI::IntSize createToolTip(const std::string& text);
        ///< @return requested tooltip size

        std::string getValueString(const int value);
        ///< get "Value: X" string or "" if value is 0

        std::string toString(const float value);
        std::string toString(const int value);

        bool mGameMode;
    };
}
#endif
