#ifndef MWGUI_DIALOGE_H
#define MWGUI_DIALOGE_H

#include "windowbase.hpp"
#include "referenceinterface.hpp"

#include "bookpage.hpp"

#include "../mwdialogue/keywordsearch.hpp"

#include <MyGUI_Delegate.h>

namespace Gui
{
    class MWList;
}

namespace MWGui
{
    class WindowManager;
}

namespace MWGui
{
    class ResponseCallback;

    class PersuasionDialog : public WindowModal
    {
    public:
        PersuasionDialog(ResponseCallback* callback);

        virtual void onOpen();

        virtual MyGUI::Widget* getDefaultKeyFocus();

    private:
        std::unique_ptr<ResponseCallback> mCallback;

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


    struct Link
    {
        virtual ~Link() {}
        virtual void activated () = 0;
    };

    struct Topic : Link
    {
        typedef MyGUI::delegates::CMultiDelegate1<const std::string&> EventHandle_TopicId;
        EventHandle_TopicId eventTopicActivated;
        Topic(const std::string& id) : mTopicId(id) {}
        std::string mTopicId;
        virtual void activated ();
    };

    struct Choice : Link
    {
        typedef MyGUI::delegates::CMultiDelegate1<int> EventHandle_ChoiceId;
        EventHandle_ChoiceId eventChoiceActivated;
        Choice(int id) : mChoiceId(id) {}
        int mChoiceId;
        virtual void activated ();
    };

    struct Goodbye : Link
    {
        typedef MyGUI::delegates::CMultiDelegate0 Event_Activated;
        Event_Activated eventActivated;
        virtual void activated ();
    };

    typedef MWDialogue::KeywordSearch <std::string, intptr_t> KeywordSearchT;

    struct DialogueText
    {
        virtual ~DialogueText() {}
        virtual void write (BookTypesetter::Ptr typesetter, KeywordSearchT* keywordSearch, std::map<std::string, Link*>& topicLinks) const = 0;
        std::string mText;
    };

    struct Response : DialogueText
    {
        Response(const std::string& text, const std::string& title = "", bool needMargin = true);
        virtual void write (BookTypesetter::Ptr typesetter, KeywordSearchT* keywordSearch, std::map<std::string, Link*>& topicLinks) const;
        void addTopicLink (BookTypesetter::Ptr typesetter, intptr_t topicId, size_t begin, size_t end) const;
        std::string mTitle;
        bool mNeedMargin;
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
        ~DialogueWindow();

        void onTradeComplete();

        virtual bool exit();

        // Events
        typedef MyGUI::delegates::CMultiDelegate0 EventHandle_Void;

        void notifyLinkClicked (TypesetBook::InteractiveId link);

        void setPtr(const MWWorld::Ptr& actor);

        void setKeywords(std::list<std::string> keyWord);

        void addResponse (const std::string& title, const std::string& text, bool needMargin = true);

        void addMessageBox(const std::string& text);

        void onFrame(float dt);
        void clear() { resetReference(); }

        void updateTopics();

        void onClose();

    protected:
        void updateTopicsPane();
        bool isCompanion(const MWWorld::Ptr& actor);
        bool isCompanion();

        void onSelectListItem(const std::string& topic, int id);
        void onByeClicked(MyGUI::Widget* _sender);
        void onMouseWheel(MyGUI::Widget* _sender, int _rel);
        void onWindowResize(MyGUI::Window* _sender);
        void onTopicActivated(const std::string& topicId);
        void onChoiceActivated(int id);
        void onGoodbyeActivated();

        void onScrollbarMoved (MyGUI::ScrollBar* sender, size_t pos);

        void updateHistory(bool scrollbar=false);

        virtual void onReferenceUnavailable();

    private:
        void updateDisposition();
        void restock();
        void deleteLater();

        bool mIsCompanion;
        std::list<std::string> mKeywords;

        std::vector<DialogueText*> mHistoryContents;
        std::vector<std::pair<std::string, int> > mChoices;
        bool mGoodbye;

        std::vector<Link*> mLinks;
        std::map<std::string, Link*> mTopicLinks;

        std::vector<Link*> mDeleteLater;

        KeywordSearchT mKeywordSearch;

        BookPage* mHistory;
        Gui::MWList*   mTopicsList;
        MyGUI::ScrollBar* mScrollBar;
        MyGUI::ProgressBar* mDispositionBar;
        MyGUI::TextBox*     mDispositionText;
        MyGUI::Button* mGoodbyeButton;

        PersuasionDialog mPersuasionDialog;

        MyGUI::IntSize mCurrentWindowSize;

        std::unique_ptr<ResponseCallback> mCallback;
        std::unique_ptr<ResponseCallback> mGreetingCallback;
    };
}
#endif
