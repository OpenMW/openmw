#include "formatting.hpp"

#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/xml_parser.hpp>

using namespace MWGui;

MyGUI::IntSize BookTextParser::parse(std::string text, MyGUI::Widget* parent, const int width)
{
    mParent = parent;
    mWidth = width;

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

    return parseSubText(text, -1, MyGUI::Align::Left | MyGUI::Align::Top);
}

MyGUI::IntSize BookTextParser::parseSubText(std::string text, int textSize, MyGUI::Align textAlign)
{
    MyGUI::IntSize size(mWidth,0);

    bool tagFound = false;
    std::string realText; // real text, without tags
    for (unsigned int i=0; i<text.size(); ++i)
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
                while (c != '>')
                {
                    if (i >= text.size())
                        throw std::runtime_error("BookTextParser Error: Tag is not terminated");

                    c = text[++i];
                }
                ++i;
                /// \todo parse tags
                size += parseSubText(text.substr(i, text.size()), textSize, textAlign);
                break;
            }
        }
        else
            realText += c;
    }

    if (!tagFound)
    {
        MyGUI::EditBox* box = mParent->createWidget<MyGUI::EditBox>("NormalText",
            MyGUI::IntCoord(0, size.height, mWidth, 24), MyGUI::Align::Left | MyGUI::Align::Top,
            mParent->getName() + boost::lexical_cast<std::string>(mParent->getChildCount()));
        box->setProperty("Static", "true");
        box->setProperty("MultiLine", "true");
        box->setProperty("WordWrap", "true");
        box->setProperty("NeedMouse", "false");
        box->setMaxTextLength(realText.size());
        box->setTextAlign(textAlign);
        box->setTextColour(MyGUI::Colour(0,0,0));
        box->setCaption(realText);
        box->setSize(box->getSize().width, box->getTextSize().height);
        size += MyGUI::IntSize(0, box->getTextSize().height);
    }

    return size;
}
