#ifndef MWGUI_FORMATTING_H
#define MWGUI_FORMATTING_H

#include <MyGUI.h>

namespace MWGui
{
    namespace Formatting
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

        class Paginator
        {
            public:
                typedef std::pair<int, int> Page;
                typedef std::vector<Page> Pages;

                Paginator(int pageWidth, int pageHeight)
                    : mStartTop(0), mCurrentTop(0),
                      mPageWidth(pageWidth), mPageHeight(pageHeight)
                {
                }

                int getStartTop() const { return mStartTop; }
                int getCurrentTop() const { return mCurrentTop; }
                int getPageWidth() const { return mPageWidth; }
                int getPageHeight() const { return mPageHeight; }
                Pages getPages() const { return mPages; }

                void setStartTop(int top) { mStartTop = top; }
                void setCurrentTop(int top) { mCurrentTop = top; }
                void modifyStartTop(int mod) { mStartTop += mod; }
                void modifyCurrentTop(int mod) { mCurrentTop += mod; }

                Paginator & operator<<(const Page & page)
                {
                    mPages.push_back(page);
                    return *this;
                }

            private:
                int mStartTop, mCurrentTop;
                int mPageWidth, mPageHeight;
                Pages mPages;
        };

        /// \brief utilities for parsing book/scroll text as mygui widgets
        class BookFormatter
        {
            public:
                Paginator::Pages markupToWidget(MyGUI::Widget * parent, std::string utf8Text, const int pageWidth, const int pageHeight);
                Paginator::Pages markupToWidget(MyGUI::Widget * parent, std::string utf8Text);

            protected:
                void parseDiv(std::string tag);
                void parseFont(std::string tag);
            private:
                TextStyle mTextStyle;
        };

        class GraphicElement
        {
            public:
                GraphicElement(MyGUI::Widget * parent, Paginator & pag, const TextStyle & style);
                virtual int getHeight() = 0;
                virtual void paginate();
                virtual int pageSplit();

            protected:
                int currentFontHeight() const;
                float widthForCharGlyph(MyGUI::Char unicodeChar) const;

                MyGUI::Widget * mParent;
                Paginator & mPaginator;
                TextStyle mStyle;
        };

        class TextElement : public GraphicElement
        {
            public:
                TextElement(MyGUI::Widget * parent, Paginator & pag, const TextStyle & style, const std::string & text);
                virtual int getHeight();
                virtual int pageSplit();
            private:
                MyGUI::EditBox * mEditBox;
        };

        class ImageElement : public GraphicElement
        {
            public:
                ImageElement(MyGUI::Widget * parent, Paginator & pag, const TextStyle & style, const std::string & tag);
                virtual int getHeight();
                virtual int pageSplit();

            private:
                int mImageHeight;
                MyGUI::ImageBox * mImageBox;
        };
    }
}

#endif
