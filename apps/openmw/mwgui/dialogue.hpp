#ifndef MWGUI_DIALOGE_H
#define MWGUI_DIALOGE_H

#include "window_base.hpp"
#include "referenceinterface.hpp"
#include <boost/array.hpp>

#include "../mwworld/ptr.hpp"

namespace MWGui
{
    class WindowManager;

    namespace Widgets
    {
        class MWList;
    }
}

/*
  This file contains the dialouge window
  Layout is defined by resources/mygui/openmw_dialogue_window.layout.
 */

namespace MWGui
{
    class DialogueHistory;

    class DialogueWindow: public WindowBase, public ReferenceInterface
    {
    public:
        DialogueWindow(WindowManager& parWindowManager);

        // Events
        typedef MyGUI::delegates::CMultiDelegate0 EventHandle_Void;

        /** Event : Dialog finished, OK button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventBye;

        void startDialogue(MWWorld::Ptr actor, std::string npcName);
        void stopDialogue();
        void setKeywords(std::list<std::string> keyWord);
        void removeKeyword(std::string keyWord);
        void addText(std::string text);
        void addTitle(std::string text);
        void askQuestion(std::string question);
        void goodbye();

        // various service button visibilities, depending if the npc/creature talked to has these services
        // make sure to call these before setKeywords()
        void setShowTrade(bool show) { mShowTrade = show; }

    protected:
        void onSelectTopic(std::string topic);
        void onByeClicked(MyGUI::Widget* _sender);
        void onHistoryClicked(MyGUI::Widget* _sender);
        void onMouseWheel(MyGUI::Widget* _sender, int _rel);
        void onWindowResize(MyGUI::Window* _sender);

        virtual void onReferenceUnavailable();

    private:
        void updateOptions();
        /**
        *Helper function that add topic keyword in blue in a text.
        */
        std::string parseText(std::string text);

        // various service button visibilities, depending if the npc/creature talked to has these services
        bool mShowTrade;

        bool mEnabled;

        DialogueHistory*   mHistory;
        Widgets::MWList*   mTopicsList;
        MyGUI::ProgressPtr mDispositionBar;
        MyGUI::EditPtr     mDispositionText;
    };
}
#endif
