#ifndef OPENMW_MWGUI_KEYBOARDNAVIGATION_H
#define OPENMW_MWGUI_KEYBOARDNAVIGATION_H

#include <MyGUI_KeyCode.h>
#include <MyGUI_IUnlinkWidget.h>

namespace MWGui
{

    class KeyboardNavigation : public MyGUI::IUnlinkWidget
    {
    public:
        KeyboardNavigation();
        ~KeyboardNavigation();

        /// @return Was the key handled by this class?
        bool injectKeyPress(MyGUI::KeyCode key, unsigned int text, bool repeat);

        void saveFocus(int mode);
        void restoreFocus(int mode);

        void _unlinkWidget(MyGUI::Widget* widget) override;

        void onFrame();

        /// Set a key focus widget for this window, if one isn't already set.
        void setDefaultFocus(MyGUI::Widget* window, MyGUI::Widget* defaultFocus);

        void setModalWindow(MyGUI::Widget* window);

        void setEnabled(bool enabled);

    private:
        bool switchFocus(int direction, bool wrap);

        bool selectFirstWidget();

        /// Send button press event to focused button
        bool accept();

        std::map<int, MyGUI::Widget*> mKeyFocus;

        MyGUI::Widget* mCurrentFocus;
        MyGUI::Widget* mModalWindow;

        bool mEnabled;
    };

}

#endif
