#include "textcolours.hpp"

#include <MyGUI_LanguageManager.h>

#include <string>

namespace MWGui
{
    MyGUI::Colour getTextColour(const std::string& type)
    {
        return MyGUI::Colour::parse(MyGUI::LanguageManager::getInstance().replaceTags("#{fontcolour=" + type + "}"));
    }

    void TextColours::loadColours()
    {
        header = getTextColour("header");
        normal = getTextColour("normal");
        notify = getTextColour("notify");

        link = getTextColour("link");
        linkOver = getTextColour("link_over");
        linkPressed = getTextColour("link_pressed");

        answer = getTextColour("answer");
        answerOver = getTextColour("answer_over");
        answerPressed = getTextColour("answer_pressed");

        journalLink = getTextColour("journal_link");
        journalLinkOver = getTextColour("journal_link_over");
        journalLinkPressed = getTextColour("journal_link_pressed");

        journalTopic = getTextColour("journal_topic");
        journalTopicOver = getTextColour("journal_topic_over");
        journalTopicPressed = getTextColour("journal_topic_pressed");
    }
}
