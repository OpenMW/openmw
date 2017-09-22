#ifndef OPENMW_MWGUI_KEYBOARDNAVIGATION_H
#define OPENMW_MWGUI_KEYBOARDNAVIGATION_H

#include <MyGUI_KeyCode.h>

namespace MWGui
{

    class KeyboardNavigation
    {
    public:
        KeyboardNavigation();
        ~KeyboardNavigation();

        /// @return Was the key handled by this class?
        bool injectKeyPress(MyGUI::KeyCode key, unsigned int text);

    private:
        bool switchFocus(int direction, bool wrap);

        /// Send button press event to focused button
        bool accept();
    };

}

#endif
