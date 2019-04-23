#include "formatting.hpp"

#include <MyGUI_EditText.h>
#include <MyGUI_Gui.h>
#include <MyGUI_EditBox.h>
#include <MyGUI_ImageBox.h>

// correctBookartPath
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include <components/debug/debuglog.hpp>
#include <components/interpreter/defines.hpp>
#include <components/misc/stringops.hpp>

#include "../mwscript/interpretercontext.hpp"

namespace MWGui
{
    namespace Formatting
    {
        /* BookTextParser */
        BookTextParser::BookTextParser(const std::string & text)
            : mIndex(0), mText(text), mIgnoreNewlineTags(true), mIgnoreLineEndings(true), mClosingTag(false)
        {
            MWScript::InterpreterContext interpreterContext(nullptr, MWWorld::Ptr()); // empty arguments, because there is no locals or actor
            mText = Interpreter::fixDefinesBook(mText, interpreterContext);

            Misc::StringUtils::replaceAll(mText, "\r", "");

            // vanilla game does not show any text after the last EOL tag.
            const std::string lowerText = Misc::StringUtils::lowerCase(mText);
            int brIndex = lowerText.rfind("<br>");
            int pIndex = lowerText.rfind("<p>");
            if (brIndex == pIndex)
                mText = "";
            else if (brIndex > pIndex)
                mText = mText.substr(0, brIndex+4);
            else
                mText = mText.substr(0, pIndex+3);

            registerTag("br", Event_BrTag);
            registerTag("p", Event_PTag);
            registerTag("img", Event_ImgTag);
            registerTag("div", Event_DivTag);
            registerTag("font", Event_FontTag);
        }

        void BookTextParser::registerTag(const std::string & tag, BookTextParser::Events type)
        {
            mTagTypes[tag] = type;
        }

        std::string BookTextParser::getReadyText() const
        {
            return mReadyText;
        }

        BookTextParser::Events BookTextParser::next()
        {
            while (mIndex < mText.size())
            {
                char ch = mText[mIndex];
                if (ch == '<')
                {
                    const size_t tagStart = mIndex + 1;
                    const size_t tagEnd = mText.find('>', tagStart);
                    if (tagEnd == std::string::npos)
                        throw std::runtime_error("BookTextParser Error: Tag is not terminated");
                    parseTag(mText.substr(tagStart, tagEnd - tagStart));
                    mIndex = tagEnd;

                    if (mTagTypes.find(mTag) != mTagTypes.end())
                    {
                        Events type = mTagTypes.at(mTag);

                        if (type == Event_BrTag || type == Event_PTag)
                        {
                            if (!mIgnoreNewlineTags)
                            {
                                if (type == Event_BrTag)
                                    mBuffer.push_back('\n');
                                else
                                {
                                    mBuffer.append("\n\n");
                                }
                            }
                            mIgnoreLineEndings = true;
                        }
                        else
                            flushBuffer();

                        if (type == Event_ImgTag)
                        {
                            mIgnoreNewlineTags = false;
                        }

                        ++mIndex;
                        return type;
                    }
                }
                else
                {
                    if (!mIgnoreLineEndings || ch != '\n')
                    {
                        mBuffer.push_back(ch);
                        mIgnoreLineEndings = false;
                        mIgnoreNewlineTags = false;
                    }
                }

                ++mIndex;
            }

            flushBuffer();
            return Event_EOF;
        }

        void BookTextParser::flushBuffer()
        {
            mReadyText = mBuffer;
            mBuffer.clear();
        }

        const BookTextParser::Attributes & BookTextParser::getAttributes() const
        {
            return mAttributes;
        }

        bool BookTextParser::isClosingTag() const
        {
            return mClosingTag;
        }

        void BookTextParser::parseTag(std::string tag)
        {
            size_t tagNameEndPos = tag.find(' ');
            mAttributes.clear();
            mTag = tag.substr(0, tagNameEndPos);
            Misc::StringUtils::lowerCaseInPlace(mTag);
            if (mTag.empty())
                return;

            mClosingTag = (mTag[0] == '/');
            if (mClosingTag)
            {
                mTag.erase(mTag.begin());
                return;
            }

            if (tagNameEndPos == std::string::npos)
                return;
            tag.erase(0, tagNameEndPos+1);

            while (!tag.empty())
            {
                size_t sepPos = tag.find('=');
                if (sepPos == std::string::npos)
                    return;

                std::string key = tag.substr(0, sepPos);
                Misc::StringUtils::lowerCaseInPlace(key);
                tag.erase(0, sepPos+1);

                std::string value;

                if (tag.empty())
                    return;

                if (tag[0] == '"')
                {
                    size_t quoteEndPos = tag.find('"', 1);
                    if (quoteEndPos == std::string::npos)
                        throw std::runtime_error("BookTextParser Error: Missing end quote in tag");
                    value = tag.substr(1, quoteEndPos-1);
                    tag.erase(0, quoteEndPos+2);
                }
                else
                {
                    size_t valEndPos = tag.find(' ');
                    if (valEndPos == std::string::npos)
                    {
                        value = tag;
                        tag.erase();
                    }
                    else
                    {
                        value = tag.substr(0, valEndPos);
                        tag.erase(0, valEndPos+1);
                    }
                }

                mAttributes[key] = value;
            }
        }

        /* BookFormatter */
        Paginator::Pages BookFormatter::markupToWidget(MyGUI::Widget * parent, const std::string & markup, const int pageWidth, const int pageHeight)
        {
            Paginator pag(pageWidth, pageHeight);

            while (parent->getChildCount())
            {
                MyGUI::Gui::getInstance().destroyWidget(parent->getChildAt(0));
            }

            mTextStyle = TextStyle();
            mBlockStyle = BlockStyle();

            MyGUI::Widget * paper = parent->createWidget<MyGUI::Widget>("Widget", MyGUI::IntCoord(0, 0, pag.getPageWidth(), pag.getPageHeight()), MyGUI::Align::Left | MyGUI::Align::Top);
            paper->setNeedMouseFocus(false);

            BookTextParser parser(markup);

            bool brBeforeLastTag = false;
            bool isPrevImg = false;
            for (;;)
            {
                BookTextParser::Events event = parser.next();
                if (event == BookTextParser::Event_BrTag || event == BookTextParser::Event_PTag)
                    continue;

                std::string plainText = parser.getReadyText();

                // for cases when linebreaks are used to cause a shift to the next page
                // if the split text block ends in an empty line, proceeding text block(s) should have leading empty lines removed
                if (pag.getIgnoreLeadingEmptyLines())
                {
                    while (!plainText.empty())
                    {
                        if (plainText[0] == '\n')
                            plainText.erase(plainText.begin());
                        else
                        {
                            pag.setIgnoreLeadingEmptyLines(false);
                            break;
                        }
                    }
                }

                if (plainText.empty())
                    brBeforeLastTag = true;
                else
                {
                    // Each block of text (between two tags / boundary and tag) will be displayed in a separate editbox widget,
                    // which means an additional linebreak will be created between them.
                    // ^ This is not what vanilla MW assumes, so we must deal with line breaks around tags appropriately.
                    bool brAtStart = (plainText[0] == '\n');
                    bool brAtEnd = (plainText[plainText.size()-1] == '\n');

                    if (brAtStart && !brBeforeLastTag && !isPrevImg)
                        plainText.erase(plainText.begin());

                    if (plainText.size() && brAtEnd)
                        plainText.erase(plainText.end()-1);

                    if (!plainText.empty() || brBeforeLastTag || isPrevImg)
                    {
                        TextElement elem(paper, pag, mBlockStyle,
                                         mTextStyle, plainText);
                        elem.paginate();
                    }

                    brBeforeLastTag = brAtEnd;
                }

                if (event == BookTextParser::Event_EOF)
                    break;

                isPrevImg = (event == BookTextParser::Event_ImgTag);

                switch (event)
                {
                    case BookTextParser::Event_ImgTag:
                    {
                        const BookTextParser::Attributes & attr = parser.getAttributes();

                        if (attr.find("src") == attr.end() || attr.find("width") == attr.end() || attr.find("height") == attr.end())
                            continue;

                        std::string src = attr.at("src");
                        int width = MyGUI::utility::parseInt(attr.at("width"));
                        int height = MyGUI::utility::parseInt(attr.at("height"));

                        bool exists;
                        std::string correctedSrc = MWBase::Environment::get().getWindowManager()->correctBookartPath(src, width, height, &exists);

                        if (!exists)
                        {
                            Log(Debug::Warning) << "Warning: Could not find \"" << src << "\" referenced by an <img> tag.";
                            break;
                        }

                        pag.setIgnoreLeadingEmptyLines(false);

                        ImageElement elem(paper, pag, mBlockStyle,
                                          correctedSrc, width, height);
                        elem.paginate();
                        break;
                    }
                    case BookTextParser::Event_FontTag:
                        if (parser.isClosingTag())
                            resetFontProperties();
                        else
                            handleFont(parser.getAttributes());
                        break;
                    case BookTextParser::Event_DivTag:
                        handleDiv(parser.getAttributes());
                        break;
                    default:
                        break;
                }
            }

            // insert last page
            if (pag.getStartTop() != pag.getCurrentTop())
                pag << Paginator::Page(pag.getStartTop(), pag.getStartTop() + pag.getPageHeight());

            paper->setSize(paper->getWidth(), pag.getCurrentTop());

            return pag.getPages();
        }

        Paginator::Pages BookFormatter::markupToWidget(MyGUI::Widget * parent, const std::string & markup)
        {
            return markupToWidget(parent, markup, parent->getWidth(), parent->getHeight());
        }

        void BookFormatter::resetFontProperties()
        {
            mTextStyle = TextStyle();
        }

        void BookFormatter::handleDiv(const BookTextParser::Attributes & attr)
        {
            if (attr.find("align") == attr.end())
                return;

            std::string align = attr.at("align");

            if (Misc::StringUtils::ciEqual(align, "center"))
                mBlockStyle.mAlign = MyGUI::Align::HCenter;
            else if (Misc::StringUtils::ciEqual(align, "left"))
                mBlockStyle.mAlign = MyGUI::Align::Left;
            else if (Misc::StringUtils::ciEqual(align, "right"))
                mBlockStyle.mAlign = MyGUI::Align::Right;
        }

        void BookFormatter::handleFont(const BookTextParser::Attributes & attr)
        {
            if (attr.find("color") != attr.end())
            {
                unsigned int color;
                std::stringstream ss;
                ss << attr.at("color");
                ss >> std::hex >> color;

                mTextStyle.mColour = MyGUI::Colour(
                    (color>>16 & 0xFF) / 255.f,
                    (color>>8 & 0xFF) / 255.f,
                    (color & 0xFF) / 255.f);
            }
            if (attr.find("face") != attr.end())
            {
                std::string face = attr.at("face");
                mTextStyle.mFont = "Journalbook "+face;
            }
            if (attr.find("size") != attr.end())
            {
                /// \todo
            }
        }

        /* GraphicElement */
        GraphicElement::GraphicElement(MyGUI::Widget * parent, Paginator & pag, const BlockStyle & blockStyle)
            : mParent(parent), mPaginator(pag), mBlockStyle(blockStyle)
        {
        }

        void GraphicElement::paginate()
        {
            int newTop = mPaginator.getCurrentTop() + getHeight();
            while (newTop-mPaginator.getStartTop() > mPaginator.getPageHeight())
            {
                int newStartTop = pageSplit();
                mPaginator << Paginator::Page(mPaginator.getStartTop(), newStartTop);
                mPaginator.setStartTop(newStartTop);
            }

            mPaginator.setCurrentTop(newTop);
        }

        int GraphicElement::pageSplit()
        {
            return mPaginator.getStartTop() + mPaginator.getPageHeight();
        }

        /* TextElement */
        TextElement::TextElement(MyGUI::Widget * parent, Paginator & pag, const BlockStyle & blockStyle,
                                 const TextStyle & textStyle, const std::string & text)
            : GraphicElement(parent, pag, blockStyle),
              mTextStyle(textStyle)
        {
            Gui::EditBox* box = parent->createWidget<Gui::EditBox>("NormalText",
                MyGUI::IntCoord(0, pag.getCurrentTop(), pag.getPageWidth(), 0), MyGUI::Align::Left | MyGUI::Align::Top,
                parent->getName() + MyGUI::utility::toString(parent->getChildCount()));
            box->setEditStatic(true);
            box->setEditMultiLine(true);
            box->setEditWordWrap(true);
            box->setNeedMouseFocus(false);
            box->setNeedKeyFocus(false);
            box->setMaxTextLength(text.size());
            box->setTextAlign(mBlockStyle.mAlign);
            box->setTextColour(mTextStyle.mColour);
            box->setFontName(mTextStyle.mFont);
            box->setCaption(MyGUI::TextIterator::toTagsString(text));
            box->setSize(box->getSize().width, box->getTextSize().height);
            mEditBox = box;
        }

        int TextElement::getHeight()
        {
            return mEditBox->getTextSize().height;
        }

        int TextElement::pageSplit()
        {
            // split lines
            const int lineHeight = MWBase::Environment::get().getWindowManager()->getFontHeight();
            unsigned int lastLine = (mPaginator.getStartTop() + mPaginator.getPageHeight() - mPaginator.getCurrentTop());
            if (lineHeight > 0)
                lastLine /= lineHeight;
            int ret = mPaginator.getCurrentTop() + lastLine * lineHeight;

            // first empty lines that would go to the next page should be ignored
            mPaginator.setIgnoreLeadingEmptyLines(true);

            const MyGUI::VectorLineInfo & lines = mEditBox->getSubWidgetText()->castType<MyGUI::EditText>()->getLineInfo();
            for (unsigned int i = lastLine; i < lines.size(); ++i)
            {
                if (lines[i].width == 0)
                    ret += lineHeight;
                else
                {
                    mPaginator.setIgnoreLeadingEmptyLines(false);
                    break;
                }
            }
            return ret;
        }

        /* ImageElement */
        ImageElement::ImageElement(MyGUI::Widget * parent, Paginator & pag, const BlockStyle & blockStyle,
                                   const std::string & src, int width, int height)
            : GraphicElement(parent, pag, blockStyle),
              mImageHeight(height)
        {
            int left = 0;
            if (mBlockStyle.mAlign.isHCenter())
                left += (pag.getPageWidth() - width) / 2;
            else if (mBlockStyle.mAlign.isLeft())
                left = 0;
            else if (mBlockStyle.mAlign.isRight())
                left += pag.getPageWidth() - width;

            mImageBox = parent->createWidget<MyGUI::ImageBox> ("ImageBox",
                MyGUI::IntCoord(left, pag.getCurrentTop(), width, mImageHeight), MyGUI::Align::Left | MyGUI::Align::Top,
                parent->getName() + MyGUI::utility::toString(parent->getChildCount()));

            mImageBox->setImageTexture(src);
            mImageBox->setProperty("NeedMouse", "false");
        }

        int ImageElement::getHeight()
        {
            return mImageHeight;
        }

        int ImageElement::pageSplit()
        {
            // if the image is larger than the page, fall back to the default pageSplit implementation
            if (mImageHeight > mPaginator.getPageHeight())
                return GraphicElement::pageSplit();
            return mPaginator.getCurrentTop();
        }
    }
}
