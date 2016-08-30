#include "textsizeutil.hpp"

#include <MyGUI_Widget.h>
#include <MyGUI_FontManager.h>

namespace MWGui
{
    int TextSizeUtil::sLineHeight = 0;

    int TextSizeUtil::getLineHeight()
    {
        if(sLineHeight == 0)
            sLineHeight = getHeightOfDefaultFont();

        return sLineHeight;
    }

    void TextSizeUtil::resizeWidgetToLineHeight(MyGUI::Widget *widget)
    {
        if (widget == NULL) return;
        const MyGUI::IntCoord &baseCoordinates = widget->getCoord();

        widget->setCoord(baseCoordinates.left, baseCoordinates.top, baseCoordinates.width, getLineHeight());
    }

    void TextSizeUtil::moveWidgetToLine(MyGUI::Widget *widget, int lineNumber, int padding)
    {
        if (widget == NULL) return;

        int top = 0;
        const MyGUI::IntCoord &baseCoordinates = widget->getCoord();
        top = (lineNumber*getLineHeight()) + padding;

        widget->setCoord(baseCoordinates.left, top, baseCoordinates.width, baseCoordinates.height);
    }

    int TextSizeUtil::getHeightOfDefaultFont()
    {
        MyGUI::FontManager& fontMgr = MyGUI::FontManager::getInstance();
        int result = fontMgr.getByName(fontMgr.getDefaultFont())->getDefaultHeight();
        std::cout << "Using font height: " << result << std::endl;
        return result;
    }

    TextSizeUtil::TextSizeUtil()
    {
        
    }
    TextSizeUtil::~TextSizeUtil()
    {
        
    }
}

