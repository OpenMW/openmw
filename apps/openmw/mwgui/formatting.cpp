#include "formatting.hpp"

#include <components/interpreter/defines.hpp>

#include "../mwscript/interpretercontext.hpp"
#include "../mwworld/ptr.hpp"

#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>

using namespace MWGui;

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
}

std::vector<std::string> BookTextParser::split(std::string text, const int width, const int height)
{
    std::vector<std::string> result;

    MWScript::InterpreterContext interpreterContext(NULL, MWWorld::Ptr()); // empty arguments, because there is no locals or actor
    text = Interpreter::fixDefinesBook(text, interpreterContext);

    boost::algorithm::replace_all(text, "<BR>", "\n");
    boost::algorithm::replace_all(text, "<P>", "\n\n");

    const int spacing = 48;

    while (text.size() > 0)
    {
        // read in characters until we have exceeded the size, or run out of text
        int currentWidth = 0;
        int currentHeight = 0;
        std::string currentText;
        std::string currentWord;

        unsigned int i=0;
        while (currentHeight <= height-spacing && i<text.size())
        {
            if (text[i] == '<')
            {
                if (text.find('>', i) == std::string::npos)
                    throw std::runtime_error("BookTextParser Error: Tag is not terminated");

                if (text.size() > i+4 && text.substr(i, 4) == "<IMG")
                {
                    int h = mHeight;
                    parseImage(text.substr(i, text.find('>', i)-i), false);
                    currentHeight += (mHeight-h);
                    currentWidth = 0;
                }
                else if (text.size() > i+5 && text.substr(i, 5) == "<FONT")
                {
                    parseFont(text.substr(i, text.find('>', i)-i));
                    currentHeight += 18; // keep this in sync with the font size
                    currentWidth = 0;
                }
                else if (text.size() > i+4 && text.substr(i, 4) == "<DIV")
                {
                    parseDiv(text.substr(i, text.find('>', i)-i));
                    currentHeight += 18; // keep this in sync with the font size
                    currentWidth = 0;
                }

                currentText += text.substr(i, text.find('>', i)-i+1);
                i = text.find('>', i);
            }
            else if (text[i] == '\n')
            {
                currentHeight += 18; // keep this in sync with the font size
                currentWidth = 0;
                currentWord = "";
                currentText += text[i];
            }
            else if (text[i] == ' ')
            {
                currentWidth += 3; // keep this in sync with the font's SpaceWidth property
                currentWord = "";
                currentText += text[i];
            }
            else
            {
                currentWidth +=
                    MyGUI::FontManager::getInstance().getByName (mTextStyle.mFont == "Default" ? "EB Garamond" : mTextStyle.mFont)
                        ->getGlyphInfo(static_cast<unsigned int>(text[i]))->width;
                currentWord += text[i];
                currentText += text[i];
            }

            if (currentWidth > width)
            {
                currentHeight += 18; // keep this in sync with the font size
                currentWidth = 0;

                // add size of the current word
                unsigned int j=0;
                while (j<currentWord.size())
                {
                    currentWidth +=
                        MyGUI::FontManager::getInstance().getByName (mTextStyle.mFont == "Default" ? "EB Garamond" : mTextStyle.mFont)
                            ->getGlyphInfo(static_cast<unsigned int>(currentWord[j]))->width;
                    ++j;
                }
            }

            ++i;
        }
        if (currentHeight > height-spacing)
        {
            // remove the last word
            currentText.erase(currentText.size()-currentWord.size(), currentText.size());
        }

        result.push_back(currentText);
        text.erase(0, currentText.size());
    }

    return result;
}

MyGUI::IntSize BookTextParser::parse(std::string text, MyGUI::Widget* parent, const int width)
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

    // remove leading newlines
    //while (text[0] == '\n')
    //    text.erase(0);

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
        if (text.find('>') == std::string::npos)
            throw std::runtime_error("BookTextParser Error: Tag is not terminated");

        if (text.size() > 4 && text.substr(0, 4) == "<IMG")
            parseImage(text.substr(0, text.find('>')));
        else if (text.size() > 5 && text.substr(0, 5) == "<FONT")
            parseFont(text.substr(0, text.find('>')));
        else if (text.size() > 4 && text.substr(0, 4) == "<DIV")
            parseDiv(text.substr(0, text.find('>')));

        text.erase(0, text.find('>')+1);
    }

    bool tagFound = false;
    std::string realText; // real text, without tags
    unsigned int i=0;
    for (; i<text.size(); ++i)
    {
        char c = text[i];
        if (c == '<')
        {
            if (text[i+1] == '/') // ignore closing tags
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
                tagFound = true;
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

    if (tagFound)
    {
        parseSubText(text.substr(i, text.size()));
    }
}
