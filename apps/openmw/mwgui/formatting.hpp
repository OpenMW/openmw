#ifndef MWGUI_FORMATTING_H
#define MWGUI_FORMATTING_H

#include <MyGUI.h>

#include <boost/property_tree/ptree.hpp>

namespace MWGui
{
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
            /**
             * @param text to parse
             * @param text size (-1 means default)
             * @param text align
             * @return size of the created widgets
             */
            MyGUI::IntSize parseSubText(std::string text, int textSize, MyGUI::Align textAlign);

        private:
            MyGUI::Widget* mParent;
            int mWidth;
    };
}

#endif
