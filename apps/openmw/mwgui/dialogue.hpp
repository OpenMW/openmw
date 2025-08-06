#ifndef MWGUI_DIALOGE_H
#define MWGUI_DIALOGE_H

#include <memory>

#include "referenceinterface.hpp"
#include "windowbase.hpp"

#include "bookpage.hpp"

#include "../mwbase/dialoguemanager.hpp"
#include "../mwdialogue/keywordsearch.hpp"

#include <MyGUI_Delegate.h>

namespace Gui
{
    class AutoSizedTextBox;
    class MWList;
}

namespace MWGui
{
    class DialogueWindow;

    class ResponseCallback : public MWBase::DialogueManager::ResponseCallback
    {
        DialogueWindow* mWindow;
        bool mNeedMargin;

    public:
        ResponseCallback(DialogueWindow* win, bool needMargin = true)
            : mWindow(win)
            , mNeedMargin(needMargin)
        {
        }

        void addResponse(std::string_view title, std::string_view text) override;

        void updateTopics() const;
    };

    class PersuasionDialog : public WindowModal
    {
    public:
        PersuasionDialog(std::unique_ptr<ResponseCallback> callback);

        void onOpen() override;

        MyGUI::Widget* getDefaultKeyFocus() override;

    protected:
        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;

    private:
        std::unique_ptr<ResponseCallback> mCallback;

        int mInitialGoldLabelWidth;
        int mInitialMainWidgetWidth;

        MyGUI::Button* mCancelButton;
        MyGUI::Button* mAdmireButton;
        MyGUI::Button* mIntimidateButton;
        MyGUI::Button* mTauntButton;
        MyGUI::Button* mBribe10Button;
        MyGUI::Button* mBribe100Button;
        MyGUI::Button* mBribe1000Button;
        MyGUI::Widget* mActionsBox;
        Gui::AutoSizedTextBox* mGoldLabel;

        std::vector<MyGUI::Button*> mButtons;
        int mControllerFocus = 0;

        void adjustAction(MyGUI::Widget* action, int& totalHeight);

        void onCancel(MyGUI::Widget* sender);
        void onPersuade(MyGUI::Widget* sender);
    };

    struct Link
    {
        virtual ~Link() {}
        virtual void activated() = 0;
    };

    struct Topic : Link
    {
        typedef MyGUI::delegates::MultiDelegate<const std::string&> EventHandle_TopicId;
        EventHandle_TopicId eventTopicActivated;
        Topic(const std::string& id)
            : mTopicId(id)
        {
        }
        std::string mTopicId;
        void activated() override;
    };

    struct Choice : Link
    {
        typedef MyGUI::delegates::MultiDelegate<int> EventHandle_ChoiceId;
        EventHandle_ChoiceId eventChoiceActivated;
        Choice(int id)
            : mChoiceId(id)
        {
        }
        int mChoiceId;
        void activated() override;
    };

    struct Goodbye : Link
    {
        typedef MyGUI::delegates::MultiDelegate<> Event_Activated;
        Event_Activated eventActivated;
        void activated() override;
    };

    typedef MWDialogue::KeywordSearch<intptr_t> KeywordSearchT;

    struct DialogueText
    {
        virtual ~DialogueText() = default;
        virtual void write(BookTypesetter::Ptr typesetter, KeywordSearchT* keywordSearch,
            std::map<std::string, std::unique_ptr<Link>>& topicLinks) const = 0;
        std::string mText;
    };

    struct Response : DialogueText
    {
        Response(std::string_view text, std::string_view title = {}, bool needMargin = true);
        void write(BookTypesetter::Ptr typesetter, KeywordSearchT* keywordSearch,
            std::map<std::string, std::unique_ptr<Link>>& topicLinks) const override;
        void addTopicLink(BookTypesetter::Ptr typesetter, intptr_t topicId, size_t begin, size_t end) const;
        std::string mTitle;
        bool mNeedMargin;
    };

    struct Message : DialogueText
    {
        Message(std::string_view text);
        void write(BookTypesetter::Ptr typesetter, KeywordSearchT* keywordSearch,
            std::map<std::string, std::unique_ptr<Link>>& topicLinks) const override;
    };

    class DialogueWindow : public WindowBase, public ReferenceInterface
    {
    public:
        DialogueWindow();

        void onTradeComplete();

        bool exit() override;

        // Events
        typedef MyGUI::delegates::MultiDelegate<> EventHandle_Void;

        void notifyLinkClicked(TypesetBook::InteractiveId link);

        void setPtr(const MWWorld::Ptr& actor) override;

        /// @return true if stale keywords were updated successfully
        bool setKeywords(const std::list<std::string>& keyWord);

        void addResponse(std::string_view title, std::string_view text, bool needMargin = true);

        void addMessageBox(std::string_view text);

        void onFrame(float dt) override;
        void clear() override { resetReference(); }

        void updateTopics();

        void onClose() override;

        std::string_view getWindowIdForLua() const override { return "Dialogue"; }

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

        void onScrollbarMoved(MyGUI::ScrollBar* sender, size_t pos);

        void updateHistory(bool scrollbar = false);

        void onReferenceUnavailable() override;

        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;

    private:
        void updateDisposition();
        void restock();
        void deleteLater();
        void redrawTopicsList();

        bool mIsCompanion;
        std::list<std::string> mKeywords;

        std::vector<std::unique_ptr<DialogueText>> mHistoryContents;
        std::vector<std::pair<std::string, int>> mChoices;
        std::vector<BookTypesetter::Style*> mChoiceStyles;
        bool mGoodbye;

        std::vector<std::unique_ptr<Link>> mLinks;
        std::map<std::string, std::unique_ptr<Link>> mTopicLinks;

        std::vector<std::unique_ptr<Link>> mDeleteLater;

        KeywordSearchT mKeywordSearch;

        BookPage* mHistory;
        Gui::MWList* mTopicsList;
        MyGUI::ScrollBar* mScrollBar;
        MyGUI::ProgressBar* mDispositionBar;
        MyGUI::TextBox* mDispositionText;
        MyGUI::Button* mGoodbyeButton;

        PersuasionDialog mPersuasionDialog;

        MyGUI::IntSize mCurrentWindowSize;

        std::unique_ptr<ResponseCallback> mCallback;
        std::unique_ptr<ResponseCallback> mGreetingCallback;

        void setControllerFocus(size_t index, bool focused);
        int mControllerFocus = 0;
        int mControllerChoice = -1;

        void updateTopicFormat();
    };
}
#endif
