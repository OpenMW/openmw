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

    class PersuasionDialog : public WindowModal
    {
    public:
        PersuasionDialog(MWBase::WindowManager& parWindowManager);

        virtual void open();

    private:
        MyGUI::Button* mCancelButton;
        MyGUI::Button* mAdmireButton;
        MyGUI::Button* mIntimidateButton;
        MyGUI::Button* mTauntButton;
        MyGUI::Button* mBribe10Button;
        MyGUI::Button* mBribe100Button;
        MyGUI::Button* mBribe1000Button;
        MyGUI::TextBox* mGoldLabel;

        void onCancel (MyGUI::Widget* sender);
        void onPersuade (MyGUI::Widget* sender);
    };

    class DialogueWindow: public WindowBase, public ReferenceInterface
    {
    public:
        DialogueWindow(MWBase::WindowManager& parWindowManager);

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
        void addMessageBox(const std::string& text);
        void addTitle(std::string text);
        void askQuestion(std::string question);
        void goodbye();
        void onFrame();

        // make sure to call these before setKeywords()
        void setServices(int services) { mServices = services; }

        enum Services
        {
            Service_Trade = 0x01,
            Service_BuySpells = 0x02,
            Service_CreateSpells = 0x04,
            Service_Enchant = 0x08,
            Service_Training = 0x10,
            Service_Travel = 0x20,
            Service_Repair = 0x40
        };

    protected:
        void onSelectTopic(const std::string& topic, int id);
        void onByeClicked(MyGUI::Widget* _sender);
        void onHistoryClicked(MyGUI::Widget* _sender);
        void onMouseWheel(MyGUI::Widget* _sender, int _rel);
        void onWindowResize(MyGUI::Window* _sender);

        virtual void onReferenceUnavailable();

        struct HyperLink
        {
            size_t mLength;
            std::string mTrueValue;
        };

    private:
        void updateOptions();
        /**
        *Helper function that add topic keyword in blue in a text.
        */
        std::string parseText(const std::string& text);

        int mServices;

        bool mEnabled;

        DialogueHistory*   mHistory;
        Widgets::MWList*   mTopicsList;
        MyGUI::ProgressPtr mDispositionBar;
        MyGUI::EditBox*     mDispositionText;

        PersuasionDialog mPersuasionDialog;

        std::map<size_t, HyperLink> mHyperLinks;
    };
}
#endif
