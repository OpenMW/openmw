#include "formatting.hpp"

#include <components/interpreter/defines.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/misc/stringops.hpp>

#include "../mwscript/interpretercontext.hpp"

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

#include <OgreUTFString.h>
#include <OgreResourceGroupManager.h>

#include <MyGUI_EditText.h>

namespace MWGui
{
    namespace Formatting
    {
        /* BookTextParser */
        BookTextParser::BookTextParser(const std::string & text)
            : mIndex(0), mText(text), mIgnoreNewlineTags(true), mIgnoreLineEndings(true)
        {
            MWScript::InterpreterContext interpreterContext(NULL, MWWorld::Ptr()); // empty arguments, because there is no locals or actor
            mText = Interpreter::fixDefinesBook(mText, interpreterContext);

            boost::algorithm::replace_all(mText, "\r", "");

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

        std::string BookTextParser::getReadyText()
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
                            mIgnoreLineEndings = false;
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

        void BookTextParser::parseTag(std::string tag)
        {
            size_t tagNameEndPos = tag.find(' ');
            mTag = tag.substr(0, tagNameEndPos);
            Misc::StringUtils::toLower(mTag);
            mAttributes.clear();
            if (tagNameEndPos == std::string::npos)
                return;
            tag.erase(0, tagNameEndPos+1);

            while (!tag.empty())
            {
                size_t sepPos = tag.find('=');
                if (sepPos == std::string::npos)
                    return;

                std::string key = tag.substr(0, sepPos);
                Misc::StringUtils::toLower(key);
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

            MyGUI::Widget * paper = parent->createWidget<MyGUI::Widget>("Widget", MyGUI::IntCoord(0, 0, pag.getPageWidth(), pag.getPageHeight()), MyGUI::Align::Left | MyGUI::Align::Top);
            paper->setNeedMouseFocus(false);

            BookTextParser parser(markup);
            for (;;)
            {
                BookTextParser::Events event = parser.next();
                if (event == BookTextParser::Event_BrTag || event == BookTextParser::Event_PTag)
                    continue;

                std::string plainText = parser.getReadyText();
                if (!plainText.empty())
                {
                    // if there's a newline at the end of the box caption, remove it
                    if (plainText[plainText.size()-1] == '\n')
                        plainText.erase(plainText.end()-1);

#if (MYGUI_VERSION < MYGUI_DEFINE_VERSION(3, 2, 2))
                    // splitting won't be fully functional until 3.2.2 (see TextElement::pageSplit())
                    // hack: prevent newlines at the end of the book possibly creating unnecessary pages
                    if (event == BookTextParser::Event_EOF)
                    {
                        while (plainText.size() && plainText[plainText.size()-1] == '\n')
                            plainText.erase(plainText.end()-1);
                    }
#endif

                    TextElement elem(paper, pag, mTextStyle, plainText);
                    elem.paginate();
                }

                if (event == BookTextParser::Event_EOF)
                    break;

                switch (event)
                {
                    case BookTextParser::Event_ImgTag:
                    {
                        const BookTextParser::Attributes & attr = parser.getAttributes();

                        if (attr.find("src") == attr.end() || attr.find("width") == attr.end() || attr.find("height") == attr.end())
                            continue;

                        std::string src = attr.at("src");
                        int width = boost::lexical_cast<int>(attr.at("width"));
                        int height = boost::lexical_cast<int>(attr.at("height"));

                        ImageElement elem(paper, pag, src, width, height);
                        elem.paginate();
                        break;
                    }
                    case BookTextParser::Event_FontTag:
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

        void BookFormatter::handleDiv(const BookTextParser::Attributes & attr)
        {
            if (attr.find("align") == attr.end())
                return;

            std::string align = attr.at("align");

            if (Misc::StringUtils::ciEqual(align, "center"))
                mTextStyle.mTextAlign = MyGUI::Align::HCenter;
            else if (Misc::StringUtils::ciEqual(align, "left"))
                mTextStyle.mTextAlign = MyGUI::Align::Left;
        }

        void BookFormatter::handleFont(const BookTextParser::Attributes & attr)
        {
            if (attr.find("color") != attr.end())
            {
                int color;
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

                if (face != "Magic Cards")
                    mTextStyle.mFont = face;
            }
            if (attr.find("size") != attr.end())
            {
                /// \todo
            }
        }

        /* GraphicElement */
        GraphicElement::GraphicElement(MyGUI::Widget * parent, Paginator & pag)
            : mParent(parent), mPaginator(pag)
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
        TextElement::TextElement(MyGUI::Widget * parent, Paginator & pag,
                                 const TextStyle & style, const std::string & text)
            : GraphicElement(parent, pag),
              mStyle(style)
        {
            MyGUI::EditBox* box = parent->createWidget<MyGUI::EditBox>("NormalText",
                MyGUI::IntCoord(0, pag.getCurrentTop(), pag.getPageWidth(), 0), MyGUI::Align::Left | MyGUI::Align::Top,
                parent->getName() + boost::lexical_cast<std::string>(parent->getChildCount()));
            box->setProperty("Static", "true");
            box->setProperty("MultiLine", "true");
            box->setProperty("WordWrap", "true");
            box->setProperty("NeedMouse", "false");
            box->setMaxTextLength(text.size());
            box->setTextAlign(mStyle.mTextAlign);
            box->setTextColour(mStyle.mColour);
            box->setFontName(mStyle.mFont);
            box->setCaption(MyGUI::TextIterator::toTagsString(text));
            box->setSize(box->getSize().width, box->getTextSize().height);
            mEditBox = box;
        }

        int TextElement::currentFontHeight() const
        {
            std::string fontName(mStyle.mFont == "Default" ? MyGUI::FontManager::getInstance().getDefaultFont() : mStyle.mFont);
            return MyGUI::FontManager::getInstance().getByName(fontName)->getDefaultHeight();
        }

        int TextElement::getHeight()
        {
            return mEditBox->getTextSize().height;
        }

        int TextElement::pageSplit()
        {
            // split lines
            const int lineHeight = currentFontHeight();
            unsigned int lastLine = (mPaginator.getStartTop() + mPaginator.getPageHeight() - mPaginator.getCurrentTop()) / lineHeight;
            int ret = mPaginator.getCurrentTop() + lastLine * lineHeight;

            // first empty lines that would go to the next page should be ignored
            // unfortunately, getLineInfo method won't be available until 3.2.2
#if (MYGUI_VERSION >= MYGUI_DEFINE_VERSION(3, 2, 2))
            const MyGUI::VectorLineInfo & lines = mEditBox->getSubWidgetText()->castType<MyGUI::EditText>()->getLineInfo();
            for (unsigned int i = lastLine; i < lines.size(); ++i)
            {
                if (lines[i].width == 0)
                    ret += lineHeight;
                else
                    break;
            }
#endif
            return ret;
        }

        /* ImageElement */
        ImageElement::ImageElement(MyGUI::Widget * parent, Paginator & pag,
                                   const std::string & src, int width, int height)
            : GraphicElement(parent, pag),
              mImageHeight(height)
        {
            mImageBox = parent->createWidget<MyGUI::ImageBox> ("ImageBox",
                MyGUI::IntCoord(0, pag.getCurrentTop(), width, mImageHeight), MyGUI::Align::Left | MyGUI::Align::Top,
                parent->getName() + boost::lexical_cast<std::string>(parent->getChildCount()));

            std::string image = Misc::ResourceHelpers::correctBookartPath(src, width, mImageHeight);
            mImageBox->setImageTexture(image);
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
