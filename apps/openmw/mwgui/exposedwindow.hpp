#ifndef MWGUI_EXPOSEDWINDOW_H
#define MWGUI_EXPOSEDWINDOW_H

#include <MyGUI_Window.h>

namespace MWGui
{

    /**
     * @brief subclass to provide access to some Widget internals.
     */
    class ExposedWindow : public MyGUI::Window
    {
        MYGUI_RTTI_DERIVED(ExposedWindow)

    public:
        MyGUI::VectorWidgetPtr getSkinWidgetsByName (const std::string &name);

        MyGUI::Widget* getSkinWidget(const std::string & _name, bool _throw = true);
        ///< Get a widget defined in the inner skin of this window.
    };

}

#endif

