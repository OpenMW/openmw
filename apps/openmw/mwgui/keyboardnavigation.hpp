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
        bool injectKeyPress(MyGUI::KeyCode key, unsigned int text);

        void saveFocus(int mode);
        void restoreFocus(int mode);

        void _unlinkWidget(MyGUI::Widget* widget);

        void onFrame();

    private:
        bool switchFocus(int direction, bool wrap);

        /// Send button press event to focused button
        bool accept();

        std::map<int, MyGUI::Widget*> mKeyFocus;

        MyGUI::Widget* mCurrentFocus;
    };

}

#endif
