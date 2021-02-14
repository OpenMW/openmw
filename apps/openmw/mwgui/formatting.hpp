#ifndef MWGUI_FORMATTING_H
#define MWGUI_FORMATTING_H

#include <MyGUI_Colour.h>
#include <map>

#include <components/widgets/box.hpp>

namespace MWGui
{
    namespace Formatting
    {
        struct TextStyle
        {
            TextStyle() :
                mColour(0,0,0)
                , mFont("Journalbook Magic Cards")
                , mTextSize(16)
            {
            }

            MyGUI::Colour mColour;
            std::string mFont;
            int mTextSize;
        };

        struct BlockStyle
        {
            BlockStyle() :
                mAlign(MyGUI::Align::Left | MyGUI::Align::Top)
            {
            }

            MyGUI::Align mAlign;
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

                Events next();

                const Attributes & getAttributes() const;
                std::string getReadyText() const;
                bool isClosingTag() const;

            private:
                void registerTag(const std::string & tag, Events type);
                void flushBuffer();
                void parseTag(std::string tag);

                size_t mIndex;
                std::string mText;
                std::string mReadyText;

                bool mIgnoreNewlineTags;
                bool mIgnoreLineEndings;
                Attributes mAttributes;
                std::string mTag;
                bool mClosingTag;
                std::map<std::string, Events> mTagTypes;
                std::string mBuffer;

                size_t mPlainTextEnd;
        };

        class Paginator
        {
            public:
                typedef std::pair<int, int> Page;
                typedef std::vector<Page> Pages;

                Paginator(int pageWidth, int pageHeight)
                    : mStartTop(0), mCurrentTop(0),
                      mPageWidth(pageWidth), mPageHeight(pageHeight),
                      mIgnoreLeadingEmptyLines(false)
                {
                }

                int getStartTop() const { return mStartTop; }
                int getCurrentTop() const { return mCurrentTop; }
                int getPageWidth() const { return mPageWidth; }
                int getPageHeight() const { return mPageHeight; }
                bool getIgnoreLeadingEmptyLines() const { return mIgnoreLeadingEmptyLines; }
                Pages getPages() const { return mPages; }

                void setStartTop(int top) { mStartTop = top; }
                void setCurrentTop(int top) { mCurrentTop = top; }
                void setIgnoreLeadingEmptyLines(bool ignore) { mIgnoreLeadingEmptyLines = ignore; }

                Paginator & operator<<(const Page & page)
                {
                    mPages.push_back(page);
                    return *this;
                }

            private:
                int mStartTop, mCurrentTop;
                int mPageWidth, mPageHeight;
                bool mIgnoreLeadingEmptyLines;
                Pages mPages;
        };

        /// \brief utilities for parsing book/scroll text as mygui widgets
        class BookFormatter
        {
            public:
                Paginator::Pages markupToWidget(MyGUI::Widget * parent, const std::string & markup, const int pageWidth, const int pageHeight);
                Paginator::Pages markupToWidget(MyGUI::Widget * parent, const std::string & markup);

            private:
                void resetFontProperties();

                void handleDiv(const BookTextParser::Attributes & attr);
                void handleFont(const BookTextParser::Attributes & attr);

                TextStyle mTextStyle;
                BlockStyle mBlockStyle;
        };

        class GraphicElement
        {
            public:
                GraphicElement(MyGUI::Widget * parent, Paginator & pag, const BlockStyle & blockStyle);
                virtual int getHeight() = 0;
                virtual void paginate();
                virtual int pageSplit();

            protected:
                virtual ~GraphicElement() {}
                MyGUI::Widget * mParent;
                Paginator & mPaginator;
                BlockStyle mBlockStyle;
        };

        class TextElement : public GraphicElement
        {
            public:
                TextElement(MyGUI::Widget * parent, Paginator & pag, const BlockStyle & blockStyle,
                            const TextStyle & textStyle, const std::string & text);
                int getHeight() override;
                int pageSplit() override;
            private:
                int currentFontHeight() const;
                TextStyle mTextStyle;
                Gui::EditBox * mEditBox;
        };

        class ImageElement : public GraphicElement
        {
            public:
                ImageElement(MyGUI::Widget * parent, Paginator & pag, const BlockStyle & blockStyle,
                             const std::string & src, int width, int height);
                int getHeight() override;
                int pageSplit() override;

            private:
                int mImageHeight;
                MyGUI::ImageBox * mImageBox;
        };
    }
}

#endif
