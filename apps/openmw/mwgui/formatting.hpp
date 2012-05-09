#ifndef MWGUI_FORMATTING_H
#define MWGUI_FORMATTING_H

#include <MyGUI.h>

#include <boost/property_tree/ptree.hpp>

namespace MWGui
{
    struct TextStyle
    {
        TextStyle() :
            mColour(0,0,0)
            , mFont("Default")
            , mTextSize(16)
            , mTextAlign(MyGUI::Align::Left | MyGUI::Align::Top)
        {
        }

        MyGUI::Colour mColour;
        std::string mFont;
        int mTextSize;
        MyGUI::Align mTextAlign;
    };

    /// \brief utilities for parsing book/scroll text as mygui widgets
    class BookTextParser
    {
        public:
            /**
             * Parse markup as MyGUI widgets
             * @param markup to parse
             * @param parent for the created widgets
             * @param maximum width
             * @return size of the created widgets
             */
            MyGUI::IntSize parse(std::string text, MyGUI::Widget* parent, const int width);

        protected:
            void parseSubText(std::string text);

            void parseImage(std::string tag);
            void parseDiv(std::string tag);
            void parseFont(std::string tag);
        private:
            MyGUI::Widget* mParent;
            int mWidth; // maximum width
            int mHeight; // current height
            TextStyle mTextStyle;
    };
}

#endif
