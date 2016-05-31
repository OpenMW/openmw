#ifndef MWGUI_TEXTSIZEUTIL_H
#define MWGUI_TEXTSIZEUTIL_H
#pragma once
namespace MyGUI {
    class Widget;
}

namespace MWGui
{
    class TextSizeUtil
    {
    public:
        static int getLineHeight();
        static void resizeWidgetToLineHeight(MyGUI::Widget * widget);
        static void moveWidgetToLine(MyGUI::Widget * widget, int lineNumber, int padding);
    private:
        static int sLineHeight;
        static int getHeightOfDefaultFont();
        TextSizeUtil();
        ~TextSizeUtil();
    };
}

#endif