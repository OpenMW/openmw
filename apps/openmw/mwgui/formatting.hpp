#ifndef MWGUI_FORMATTING_H
#define MWGUI_FORMATTING_H

#include <MyGUI.h>
#include <map>

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

        class BookTextParser
        {
            public:
                typedef std::map<std::string, std::string> Attributes;
                enum Events
                {
                    Event_None = -2,
                    Event_EOF = -1,
                    Event_BrTag,
                    Event_PTag,
                    Event_ImgTag,
                    Event_DivTag,
                    Event_FontTag
                };

                BookTextParser(const std::string & text);
                void registerTag(const std::string & tag, Events type);
                std::string getReadyText();

                Events next();
                void flushBuffer();
                const Attributes & getAttributes() const;
                void parseTag(std::string tag);

            private:
                size_t mIndex;
                std::string mText;
                std::string mReadyText;

                bool mIgnoreNewlineTags;
                bool mIgnoreLineEndings;
                Attributes mAttributes;
                std::string mTag;
                std::map<std::string, Events> mTagTypes;
                std::string mBuffer;
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
                Paginator::Pages markupToWidget(MyGUI::Widget * parent, const std::string & markup, const int pageWidth, const int pageHeight);
                Paginator::Pages markupToWidget(MyGUI::Widget * parent, const std::string & markup);

            protected:
                void handleImg(const BookTextParser::Attributes & attr);
                void handleDiv(const BookTextParser::Attributes & attr);
                void handleFont(const BookTextParser::Attributes & attr);
            private:
                TextStyle mTextStyle;
        };

        class GraphicElement
        {
            public:
                GraphicElement(MyGUI::Widget * parent, Paginator & pag);
                virtual int getHeight() = 0;
                virtual void paginate();
                virtual int pageSplit();

            protected:
                MyGUI::Widget * mParent;
                Paginator & mPaginator;
        };

        class TextElement : public GraphicElement
        {
            public:
                TextElement(MyGUI::Widget * parent, Paginator & pag, const TextStyle & style, const std::string & text);
                virtual int getHeight();
                virtual int pageSplit();
            private:
                int currentFontHeight() const;
                TextStyle mStyle;
                MyGUI::EditBox * mEditBox;
        };

        class ImageElement : public GraphicElement
        {
            public:
                ImageElement(MyGUI::Widget * parent, Paginator & pag, const std::string & src, int width, int height);
                virtual int getHeight();
                virtual int pageSplit();

            private:
                int mImageHeight;
                MyGUI::ImageBox * mImageBox;
        };
    }
}

#endif
