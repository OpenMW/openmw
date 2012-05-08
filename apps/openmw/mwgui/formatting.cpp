#include "formatting.hpp"

#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/xml_parser.hpp>

using namespace MWGui;

MyGUI::IntSize BookTextParser::parse(std::string text, MyGUI::Widget* parent, const int width)
{
    mParent = parent;
    mWidth = width;
    mHeight = 0;

    assert(mParent);
    while (mParent->getChildCount())
    {
        MyGUI::Gui::getInstance().destroyWidget(mParent->getChildAt(0));
    }

    boost::algorithm::replace_all(text, "<BR>", "\n");

    // remove leading newlines
    //while (text[0] == '\n')
    //    text.erase(0);

    // remove trailing " and newlines
    if (text[text.size()-1] == '\"')
        text.erase(text.size()-1);
    while (text[text.size()-1] == '\n')
        text.erase(text.size()-1);

    parseSubText(text);
    return MyGUI::IntSize(mWidth, mHeight);
}

void BookTextParser::parseImage(std::string tag)
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

    MyGUI::ImageBox* box = mParent->createWidget<MyGUI::ImageBox> ("ImageBox",
        MyGUI::IntCoord(0, mHeight, width, height), MyGUI::Align::Left | MyGUI::Align::Top,
        mParent->getName() + boost::lexical_cast<std::string>(mParent->getChildCount()));
    box->setImageTexture("bookart\\" + image);
    box->setProperty("NeedMouse", "false");

    mWidth = std::max(mWidth, width);
    mHeight += height;
}

void BookTextParser::parseSubText(std::string text)
{
    if (text[0] == '<')
    {
        if (text.find('>') == std::string::npos)
            throw std::runtime_error("BookTextParser Error: Tag is not terminated");

        if (text.size() > 4 && text.substr(0, 4) == "<IMG")
        {
            parseImage(text.substr(0, text.find('>')));
        }

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
