#ifndef MWGUI_DIALOGE_H
#define MWGUI_DIALOGE_H

#include "windowbase.hpp"
#include "referenceinterface.hpp"

#include "bookpage.hpp"

#include "keywordsearch.hpp"

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
    class DialogueHistoryViewModel;
    class BookPage;

    class PersuasionDialog : public WindowModal
    {
    public:
        PersuasionDialog();

        virtual void open();

        // The receiver of the bribe
        void setReceiver(MWWorld::Ptr receiver);

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

        // The receiver of the bribe
        MWWorld::Ptr mReceiver;
    };


    struct Link
    {
        virtual ~Link() {}
        virtual void activated () = 0;
    };

    struct Topic : Link
    {
        Topic(const std::string& id) : mTopicId(id) {}
        std::string mTopicId;
        virtual void activated ();
    };

    struct Choice : Link
    {
        Choice(int id) : mChoiceId(id) {}
        int mChoiceId;
        virtual void activated ();
    };

    struct Goodbye : Link
    {
        virtual void activated ();
    };

    typedef KeywordSearch <std::string, intptr_t> KeywordSearchT;

    struct DialogueText
    {
        virtual ~DialogueText() {}
        virtual void write (BookTypesetter::Ptr typesetter, KeywordSearchT* keywordSearch, std::map<std::string, Link*>& topicLinks) const = 0;
        std::string mText;
    };

    struct Response : DialogueText
    {
        Response(const std::string& text, const std::string& title = "");
        virtual void write (BookTypesetter::Ptr typesetter, KeywordSearchT* keywordSearch, std::map<std::string, Link*>& topicLinks) const;
        void addTopicLink (BookTypesetter::Ptr typesetter, intptr_t topicId, size_t begin, size_t end) const;
        std::string mTitle;
    };

    struct Message : DialogueText
    {
        Message(const std::string& text);
        virtual void write (BookTypesetter::Ptr typesetter, KeywordSearchT* keywordSearch, std::map<std::string, Link*>& topicLinks) const;
    };

    class DialogueWindow: public WindowBase, public ReferenceInterface
    {
    public:
        DialogueWindow();

        // Events
        typedef MyGUI::delegates::CMultiDelegate0 EventHandle_Void;

        void notifyLinkClicked (TypesetBook::InteractiveId link);

        void startDialogue(MWWorld::Ptr actor, std::string npcName);
        void setKeywords(std::list<std::string> keyWord);

        void addResponse (const std::string& text, const std::string& title="");

        void addMessageBox(const std::string& text);

        void addChoice(const std::string& choice, int id);
        void clearChoices();

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
        void onMouseWheel(MyGUI::Widget* _sender, int _rel);
        void onWindowResize(MyGUI::Window* _sender);

        void onScrollbarMoved (MyGUI::ScrollBar* sender, size_t pos);

        void updateHistory(bool scrollbar=false);

        virtual void onReferenceUnavailable();

    private:
        void updateOptions();

        int mServices;

        bool mEnabled;

        bool mGoodbye;

        std::vector<DialogueText*> mHistoryContents;
        std::map<std::string, int> mChoices;

        std::vector<Link*> mLinks;
        std::map<std::string, Link*> mTopicLinks;

        KeywordSearchT mKeywordSearch;

        BookPage* mHistory;
        Widgets::MWList*   mTopicsList;
        MyGUI::ScrollBar* mScrollBar;
        MyGUI::ProgressPtr mDispositionBar;
        MyGUI::EditBox*     mDispositionText;

        PersuasionDialog mPersuasionDialog;
    };
}
#endif
