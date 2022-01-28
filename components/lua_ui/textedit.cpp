#include "textedit.hpp"

namespace LuaUi
{
    void LuaTextEdit::updateProperties()
    {
        setCaption(propertyValue("caption", std::string()));

        WidgetExtension::updateProperties();
    }
}
