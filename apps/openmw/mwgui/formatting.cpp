#include "formatting.hpp"

#include <components/interpreter/defines.hpp>
#include <components/misc/resourcehelpers.hpp>

#include "../mwscript/interpretercontext.hpp"

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

#include <OgreUTFString.h>
#include <OgreResourceGroupManager.h>

#include <MyGUI_EditText.h>

namespace
{
    Ogre::UTFString::unicode_char unicodeCharFromChar(char ch)
    {
        std::string s;
        s += ch;
        Ogre::UTFString string(s);
        return string.getChar(0);
    }
}

namespace MWGui
{
    namespace Formatting
    {
        Paginator::Pages BookFormatter::markupToWidget(MyGUI::Widget * parent, std::string utf8Text, const int pageWidth, const int pageHeight)
        {
            using Ogre::UTFString;

            MWScript::InterpreterContext interpreterContext(NULL, MWWorld::Ptr()); // empty arguments, because there is no locals or actor
            utf8Text = Interpreter::fixDefinesBook(utf8Text, interpreterContext);

            boost::algorithm::replace_all(utf8Text, "\n", "");
            boost::algorithm::replace_all(utf8Text, "\r", "");
            boost::algorithm::replace_all(utf8Text, "<BR>", "\n");
            boost::algorithm::replace_all(utf8Text, "<P>", "\n\n");

            UTFString text(utf8Text);
            UTFString plainText;

            const UTFString::unicode_char LEFT_ANGLE = unicodeCharFromChar('<');
            const UTFString::unicode_char NEWLINE = unicodeCharFromChar('\n');

            Paginator pag(pageWidth, pageHeight);

            while (parent->getChildCount())
            {
                MyGUI::Gui::getInstance().destroyWidget(parent->getChildAt(0));
            }

            MyGUI::Widget * paper = parent->createWidget<MyGUI::Widget>("Widget", MyGUI::IntCoord(0, 0, pag.getPageWidth(), pag.getPageHeight()), MyGUI::Align::Left | MyGUI::Align::Top);
            paper->setNeedMouseFocus(false);

            bool ignoreNewlines = true;
            for (size_t index = 0; index < text.size(); ++index)
            {
                const UTFString::unicode_char ch = text.getChar(index);
                if (!plainText.empty() && (ch == LEFT_ANGLE || index == text.size() - 1))
                {
                    // if there's a newline at the end of the box caption, remove it
                    if (plainText[plainText.size()-1] == NEWLINE)
                        plainText.erase(plainText.end()-1);

                    TextElement elem(paper, pag, mTextStyle, plainText.asUTF8());
                    elem.paginate();

                    plainText.clear();
                }

                if (ch == LEFT_ANGLE)
                {
                    const size_t tagStart = index + 1;
                    const size_t tagEnd = text.find('>', tagStart);
                    if (tagEnd == UTFString::npos)
                        throw std::runtime_error("BookTextParser Error: Tag is not terminated");
                    const std::string tag = text.substr(tagStart, tagEnd - tagStart).asUTF8();

                    if (boost::algorithm::starts_with(tag, "IMG"))
                    {
                        ImageElement elem(paper, pag, mTextStyle, tag);
                        elem.paginate();

                        ignoreNewlines = false;
                    }
                    else if (boost::algorithm::starts_with(tag, "FONT"))
                    {
                        parseFont(tag);
                    }
                    else if (boost::algorithm::starts_with(tag, "DIV"))
                    {
                        parseDiv(tag);
                    }

                    index = tagEnd;
                }
                else
                {
                    if (!ignoreNewlines || ch != NEWLINE)
                    {
                        plainText.push_back(ch);
                        ignoreNewlines = false;
                    }
                }
            }

            // insert last page
            pag << Paginator::Page(pag.getStartTop(), pag.getStartTop() + pag.getPageHeight());

            paper->setSize(paper->getWidth(), pag.getCurrentTop());

            return pag.getPages();
        }

        Paginator::Pages BookFormatter::markupToWidget(MyGUI::Widget * parent, std::string utf8Text)
        {
            return markupToWidget(parent, utf8Text, parent->getWidth(), parent->getHeight());
        }

        void BookFormatter::parseDiv(std::string tag)
        {
            if (tag.find("ALIGN=") == std::string::npos)
                return;

            int align_start = tag.find("ALIGN=")+7;
            std::string align = tag.substr(align_start, tag.find('"', align_start)-align_start);
            if (align == "CENTER")
                mTextStyle.mTextAlign = MyGUI::Align::HCenter;
            else if (align == "LEFT")
                mTextStyle.mTextAlign = MyGUI::Align::Left;
        }

        void BookFormatter::parseFont(std::string tag)
        {
            if (tag.find("COLOR=") != std::string::npos)
            {
                int color_start = tag.find("COLOR=")+7;

                int color;
                std::stringstream ss;
                ss << tag.substr(color_start, tag.find('"', color_start)-color_start);
                ss >> std::hex >> color;

                mTextStyle.mColour = MyGUI::Colour(
                    (color>>16 & 0xFF) / 255.f,
                    (color>>8 & 0xFF) / 255.f,
                    (color & 0xFF) / 255.f);
            }
            if (tag.find("FACE=") != std::string::npos)
            {
                int face_start = tag.find("FACE=")+6;
                std::string face = tag.substr(face_start, tag.find('"', face_start)-face_start);

                if (face != "Magic Cards")
                    mTextStyle.mFont = face;
            }
            if (tag.find("SIZE=") != std::string::npos)
            {
                /// \todo
            }
        }

        /* GraphicElement */
        GraphicElement::GraphicElement(MyGUI::Widget * parent, Paginator & pag, const TextStyle & style)
            : mParent(parent), mPaginator(pag), mStyle(style)
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

            mPaginator.modifyCurrentTop(getHeight());
        }

        int GraphicElement::pageSplit()
        {
            return mPaginator.getStartTop() + mPaginator.getPageHeight();
        }

        int GraphicElement::currentFontHeight() const
        {
            std::string fontName(mStyle.mFont == "Default" ? MyGUI::FontManager::getInstance().getDefaultFont() : mStyle.mFont);
            return MyGUI::FontManager::getInstance().getByName(fontName)->getDefaultHeight();
        }

        float GraphicElement::widthForCharGlyph(MyGUI::Char unicodeChar) const
        {
            std::string fontName(mStyle.mFont == "Default" ? MyGUI::FontManager::getInstance().getDefaultFont() : mStyle.mFont);
            return MyGUI::FontManager::getInstance().getByName(fontName)
                    ->getGlyphInfo(unicodeChar)->width;
        }

        /* TextElement */
        TextElement::TextElement(MyGUI::Widget * parent, Paginator & pag, const TextStyle & style, const std::string & text)
            : GraphicElement(parent, pag, style)
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
        ImageElement::ImageElement(MyGUI::Widget * parent, Paginator & pag, const TextStyle & style, const std::string & tag)
            : GraphicElement(parent, pag, style),
              mImageHeight(0)
        {
            int src_start = tag.find("SRC=")+5;
            std::string src = tag.substr(src_start, tag.find('"', src_start)-src_start);

            int width_start = tag.find("WIDTH=")+7;
            int width = boost::lexical_cast<int>(tag.substr(width_start, tag.find('"', width_start)-width_start));

            int height_start = tag.find("HEIGHT=")+8;
            mImageHeight = boost::lexical_cast<int>(tag.substr(height_start, tag.find('"', height_start)-height_start));

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
            return mPaginator.getCurrentTop();
        }
    }
}
