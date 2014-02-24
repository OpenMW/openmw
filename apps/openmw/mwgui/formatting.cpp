#include "formatting.hpp"

#include <components/interpreter/defines.hpp>

#include "../mwscript/interpretercontext.hpp"

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <OgreUTFString.h>

#include <OgreResourceGroupManager.h>

namespace
{
    int convertFromHex(std::string hex)
    {
        int value = 0;

        int a = 0;
        int b = hex.length() - 1;
        for (; b >= 0; a++, b--)
        {
            if (hex[b] >= '0' && hex[b] <= '9')
            {
                value += (hex[b] - '0') * (1 << (a * 4));
            }
            else
            {
                switch (hex[b])
                {
                    case 'A':
                    case 'a':
                        value += 10 * (1 << (a * 4));
                        break;

                    case 'B':
                    case 'b':
                        value += 11 * (1 << (a * 4));
                        break;

                    case 'C':
                    case 'c':
                        value += 12 * (1 << (a * 4));
                        break;

                    case 'D':
                    case 'd':
                        value += 13 * (1 << (a * 4));
                        break;

                    case 'E':
                    case 'e':
                        value += 14 * (1 << (a * 4));
                        break;

                    case 'F':
                    case 'f':
                        value += 15 * (1 << (a * 4));
                        break;

                    default:
                        throw std::runtime_error("invalid character in hex number");
                        break;
                }
            }
        }

        return value;
    }

    Ogre::UTFString::unicode_char unicodeCharFromChar(char ch)
    {
        std::string s;
        s += ch;
        Ogre::UTFString string(s);
        return string.getChar(0);
    }
    
    bool is_not_empty(const std::string& s) {
        std::string temp = s;
        boost::algorithm::trim(temp);
        return !temp.empty();
    }
}

namespace MWGui
{

    std::vector<std::string> BookTextParser::split(std::string utf8Text, const int width, const int height)
    {
        using Ogre::UTFString;
        std::vector<std::string> result;

        MWScript::InterpreterContext interpreterContext(NULL, MWWorld::Ptr()); // empty arguments, because there is no locals or actor
        utf8Text = Interpreter::fixDefinesBook(utf8Text, interpreterContext);

        boost::algorithm::replace_all(utf8Text, "\n", "");
        boost::algorithm::replace_all(utf8Text, "\r", "");
        boost::algorithm::replace_all(utf8Text, "<BR>", "\n");
        boost::algorithm::replace_all(utf8Text, "<P>", "\n\n");

        UTFString text(utf8Text);
        const int spacing = 48;

        const UTFString::unicode_char LEFT_ANGLE = unicodeCharFromChar('<');
        const UTFString::unicode_char NEWLINE = unicodeCharFromChar('\n');
        const UTFString::unicode_char SPACE = unicodeCharFromChar(' ');

        while (!text.empty())
        {
            // read in characters until we have exceeded the size, or run out of text
            int currentWidth = 0;
            int currentHeight = 0;

            size_t currentWordStart = 0;
            size_t index = 0;
            
            {
                std::string texToTrim = text.asUTF8();
                boost::algorithm::trim( texToTrim );
                text = UTFString(texToTrim);
            }
            
            
            while (currentHeight <= height - spacing && index < text.size())
            {
                const UTFString::unicode_char ch = text.getChar(index);
                if (ch == LEFT_ANGLE)
                {
                    const size_t tagStart = index + 1;
                    const size_t tagEnd = text.find('>', tagStart);
                    if (tagEnd == UTFString::npos)
                        throw std::runtime_error("BookTextParser Error: Tag is not terminated");
                    const std::string tag = text.substr(tagStart, tagEnd - tagStart).asUTF8();

                    if (boost::algorithm::starts_with(tag, "IMG"))
                    {
                        const int h = mHeight;
                        parseImage(tag, false);
                        currentHeight += (mHeight - h);
                        currentWidth = 0;
                    }
                    else if (boost::algorithm::starts_with(tag, "FONT"))
                    {
                        parseFont(tag);
                        if (currentWidth != 0) {
                            currentHeight += currentFontHeight();
                            currentWidth = 0;
                        }
                        currentWidth = 0;
                    }
                    else if (boost::algorithm::starts_with(tag, "DIV"))
                    {
                        parseDiv(tag);
                        if (currentWidth != 0) {
                            currentHeight += currentFontHeight();
                            currentWidth = 0;
                        }
                    }
                    index = tagEnd;
                }
                else if (ch == NEWLINE)
                {
                    currentHeight += currentFontHeight();
                    currentWidth = 0;
                    currentWordStart = index;
                }
                else if (ch == SPACE)
                {
                    currentWidth += 3; // keep this in sync with the font's SpaceWidth property
                    currentWordStart = index;
                }
                else
                {
                    currentWidth += widthForCharGlyph(ch);
                }

                if (currentWidth > width)
                {
                    currentHeight += currentFontHeight();
                    currentWidth = 0;
                    // add size of the current word
                    UTFString word = text.substr(currentWordStart, index - currentWordStart);
                    for (UTFString::const_iterator it = word.begin(), end = word.end(); it != end; ++it)
                        currentWidth += widthForCharGlyph(it.getCharacter());
                }
                index += UTFString::_utf16_char_length(ch);
            }
            const size_t pageEnd = (currentHeight > height - spacing && currentWordStart != 0)
                    ? currentWordStart : index;

            result.push_back(text.substr(0, pageEnd).asUTF8());
            text.erase(0, pageEnd);
        }
        
        std::vector<std::string> nonEmptyPages;
        boost::copy(result | boost::adaptors::filtered(is_not_empty), std::back_inserter(nonEmptyPages));
        return nonEmptyPages;
    }

    float BookTextParser::widthForCharGlyph(unsigned unicodeChar) const
    {
        std::string fontName(mTextStyle.mFont == "Default" ? MyGUI::FontManager::getInstance().getDefaultFont() : mTextStyle.mFont);
        return MyGUI::FontManager::getInstance().getByName(fontName)
                ->getGlyphInfo(unicodeChar)->width;
    }

    float BookTextParser::currentFontHeight() const
    {
        std::string fontName(mTextStyle.mFont == "Default" ? MyGUI::FontManager::getInstance().getDefaultFont() : mTextStyle.mFont);
        return MyGUI::FontManager::getInstance().getByName(fontName)->getDefaultHeight();
    }

    MyGUI::IntSize BookTextParser::parsePage(std::string text, MyGUI::Widget* parent, const int width)
    {
        MWScript::InterpreterContext interpreterContext(NULL, MWWorld::Ptr()); // empty arguments, because there is no locals or actor
        text = Interpreter::fixDefinesBook(text, interpreterContext);

        mParent = parent;
        mWidth = width;
        mHeight = 0;

        assert(mParent);
        while (mParent->getChildCount())
        {
            MyGUI::Gui::getInstance().destroyWidget(mParent->getChildAt(0));
        }

        // remove trailing "
        if (text[text.size()-1] == '\"')
            text.erase(text.size()-1);

        parseSubText(text);
        return MyGUI::IntSize(mWidth, mHeight);
    }
    
    MyGUI::IntSize BookTextParser::parseScroll(std::string text, MyGUI::Widget* parent, const int width)
    {
        MWScript::InterpreterContext interpreterContext(NULL, MWWorld::Ptr()); // empty arguments, because there is no locals or actor
        text = Interpreter::fixDefinesBook(text, interpreterContext);

        mParent = parent;
        mWidth = width;
        mHeight = 0;

        assert(mParent);
        while (mParent->getChildCount())
        {
            MyGUI::Gui::getInstance().destroyWidget(mParent->getChildAt(0));
        }

        boost::algorithm::replace_all(text, "<BR>", "\n");
        boost::algorithm::replace_all(text, "<P>", "\n\n");
        boost::algorithm::trim_left(text);

        // remove trailing "
        if (text[text.size()-1] == '\"')
            text.erase(text.size()-1);

        parseSubText(text);
        return MyGUI::IntSize(mWidth, mHeight);
    }
    

    void BookTextParser::parseImage(std::string tag, bool createWidget)
    {
        int src_start = tag.find("SRC=")+5;
        std::string image = tag.substr(src_start, tag.find('"', src_start)-src_start);

        // fix texture extension to .dds
        if (image.size() > 4)
        {
            image[image.size()-3] = 'd';
            image[image.size()-2] = 'd';
            image[image.size()-1] = 's';
        }

        int width_start = tag.find("WIDTH=")+7;
        int width = boost::lexical_cast<int>(tag.substr(width_start, tag.find('"', width_start)-width_start));

        int height_start = tag.find("HEIGHT=")+8;
        int height = boost::lexical_cast<int>(tag.substr(height_start, tag.find('"', height_start)-height_start));

        if (createWidget)
        {
            MyGUI::ImageBox* box = mParent->createWidget<MyGUI::ImageBox> ("ImageBox",
                MyGUI::IntCoord(0, mHeight, width, height), MyGUI::Align::Left | MyGUI::Align::Top,
                mParent->getName() + boost::lexical_cast<std::string>(mParent->getChildCount()));

            // Apparently a bug with some morrowind versions, they reference the image without the size suffix.
            // So if the image isn't found, try appending the size.
            if (!Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup("bookart\\"+image))
            {
                std::stringstream str;
                str << image.substr(0, image.rfind(".")) << "_" << width << "_" << height << image.substr(image.rfind("."));
                image = str.str();
            }

            box->setImageTexture("bookart\\" + image);
            box->setProperty("NeedMouse", "false");
        }

        mWidth = std::max(mWidth, width);
        mHeight += height;
    }

    void BookTextParser::parseDiv(std::string tag)
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

    void BookTextParser::parseFont(std::string tag)
    {
        if (tag.find("COLOR=") != std::string::npos)
        {
            int color_start = tag.find("COLOR=")+7;
            std::string color = tag.substr(color_start, tag.find('"', color_start)-color_start);

            mTextStyle.mColour = MyGUI::Colour(
                convertFromHex(color.substr(0, 2))/255.0,
                convertFromHex(color.substr(2, 2))/255.0,
                convertFromHex(color.substr(4, 2))/255.0);
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

    void BookTextParser::parseSubText(std::string text)
    {
        if (text[0] == '<')
        {
            const size_t tagStart = 1;
            const size_t tagEnd = text.find('>', tagStart);
            if (tagEnd == std::string::npos)
                throw std::runtime_error("BookTextParser Error: Tag is not terminated");
            const std::string tag = text.substr(tagStart, tagEnd - tagStart);

            if (boost::algorithm::starts_with(tag, "IMG"))
                parseImage(tag);
            if (boost::algorithm::starts_with(tag, "FONT"))
                parseFont(tag);
            if (boost::algorithm::starts_with(tag, "DIV"))
                parseDiv(tag);

            text.erase(0, tagEnd + 1);
        }

        size_t tagStart = std::string::npos;
        std::string realText; // real text, without tags
        for (size_t i = 0; i<text.size(); ++i)
        {
            char c = text[i];
            if (c == '<')
            {
                if ((i + 1 < text.size()) && text[i+1] == '/') // ignore closing tags
                {
                    while (c != '>')
                    {
                        if (i >= text.size())
                            throw std::runtime_error("BookTextParser Error: Tag is not terminated");
                        ++i;
                        c = text[i];
                    }
                    continue;
                }
                else
                {
                    tagStart = i;
                    break;
                }
            }
            else
                realText += c;
        }

        MyGUI::EditBox* box = mParent->createWidget<MyGUI::EditBox>("NormalText",
            MyGUI::IntCoord(0, mHeight, mWidth, 24), MyGUI::Align::Left | MyGUI::Align::Top,
            mParent->getName() + boost::lexical_cast<std::string>(mParent->getChildCount()));
        box->setProperty("Static", "true");
        box->setProperty("MultiLine", "true");
        box->setProperty("WordWrap", "true");
        box->setProperty("NeedMouse", "false");
        box->setMaxTextLength(realText.size());
        box->setTextAlign(mTextStyle.mTextAlign);
        box->setTextColour(mTextStyle.mColour);
        box->setFontName(mTextStyle.mFont);
        box->setCaption(realText);
        box->setSize(box->getSize().width, box->getTextSize().height);
        mHeight += box->getTextSize().height;

        if (tagStart != std::string::npos)
        {
            parseSubText(text.substr(tagStart, text.size()));
        }
    }

}
