#ifndef MWGUI_TEXTCOLORS_H
#define MWGUI_TEXTCOLORS_H

#include <MyGUI_Colour.h>

namespace MWGui
{
    struct TextColours
    {
        MyGUI::Colour header;
        MyGUI::Colour normal;
        MyGUI::Colour notify;

        MyGUI::Colour link;
        MyGUI::Colour linkOver;
        MyGUI::Colour linkPressed;

        MyGUI::Colour answer;
        MyGUI::Colour answerOver;
        MyGUI::Colour answerPressed;

        MyGUI::Colour journalLink;
        MyGUI::Colour journalLinkOver;
        MyGUI::Colour journalLinkPressed;

        MyGUI::Colour journalTopic;
        MyGUI::Colour journalTopicOver;
        MyGUI::Colour journalTopicPressed;

    public:
        void loadColours();
    };

}

#endif
