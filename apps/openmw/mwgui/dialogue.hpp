#ifndef MWGUI_DIALOGE_H
#define MWGUI_DIALOGE_H

#include "window_base.hpp"
#include <boost/array.hpp>

namespace MWGui
{
    class WindowManager;
}

namespace MWWorld
{
    class Environment;
}

/*
  This file contains the dialouge window
  Layout is defined by resources/mygui/openmw_dialogue_window_layout.xml.
 */

namespace MWGui
{
    class DialogueHistory;

    using namespace MyGUI;

    class DialogueWindow: public WindowBase
    {
    public:
        DialogueWindow(WindowManager& parWindowManager,MWWorld::Environment& environment);

        void open();

        // Events
        typedef delegates::CMultiDelegate0 EventHandle_Void;

        /** Event : Dialog finished, OK button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventBye;

        void startDialogue(std::string npcName);
        void stopDialogue();
        void setKeywords(std::list<std::string> keyWord);
        void removeKeyword(std::string keyWord);
        void addText(std::string text);
        void addTitle(std::string text);
        void askQuestion(std::string question);

    protected:
        void onSelectTopic(MyGUI::List* _sender, size_t _index);
        void onByeClicked(MyGUI::Widget* _sender);
        void onHistoryClicked(MyGUI::Widget* _sender);

    private:
        void updateOptions();
        /**
        *Helper function that add topic keyword in blue in a text.
        */
        std::string parseText(std::string text);

        DialogueHistory*     history;
        MyGUI::ListPtr      topicsList;
        MyGUI::ProgressPtr pDispositionBar;
        MyGUI::EditPtr pDispositionText;
        std::map<std::string,std::string> pTopicsText;// this map links keyword and "real" text.

        MWWorld::Environment& mEnvironment;
    };
}
#endif
