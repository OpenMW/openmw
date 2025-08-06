#include "journalwindow.hpp"

#include <set>
#include <stack>
#include <string>
#include <utility>

#include <MyGUI_Button.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_TextBox.h>

#include <components/misc/strings/algorithm.hpp>
#include <components/widgets/imagebutton.hpp>
#include <components/widgets/list.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/windowmanager.hpp"

#include "bookpage.hpp"
#include "journalbooks.hpp"
#include "journalviewmodel.hpp"
#include "windowbase.hpp"

namespace
{
    static constexpr std::string_view OptionsOverlay = "OptionsOverlay";
    static constexpr std::string_view OptionsBTN = "OptionsBTN";
    static constexpr std::string_view PrevPageBTN = "PrevPageBTN";
    static constexpr std::string_view NextPageBTN = "NextPageBTN";
    static constexpr std::string_view CloseBTN = "CloseBTN";
    static constexpr std::string_view JournalBTN = "JournalBTN";
    static constexpr std::string_view TopicsBTN = "TopicsBTN";
    static constexpr std::string_view QuestsBTN = "QuestsBTN";
    static constexpr std::string_view CancelBTN = "CancelBTN";
    static constexpr std::string_view ShowAllBTN = "ShowAllBTN";
    static constexpr std::string_view ShowActiveBTN = "ShowActiveBTN";
    static constexpr std::string_view PageOneNum = "PageOneNum";
    static constexpr std::string_view PageTwoNum = "PageTwoNum";
    static constexpr std::string_view TopicsList = "TopicsList";
    static constexpr std::string_view QuestsList = "QuestsList";
    static constexpr std::string_view LeftBookPage = "LeftBookPage";
    static constexpr std::string_view RightBookPage = "RightBookPage";
    static constexpr std::string_view LeftTopicIndex = "LeftTopicIndex";
    static constexpr std::string_view CenterTopicIndex = "CenterTopicIndex";
    static constexpr std::string_view RightTopicIndex = "RightTopicIndex";

    struct JournalWindowImpl : MWGui::JournalBooks, MWGui::JournalWindow
    {
        struct DisplayState
        {
            unsigned int mPage;
            Book mBook;
        };

        typedef std::stack<DisplayState> DisplayStateStack;

        DisplayStateStack mStates;
        Book mTopicIndexBook;
        bool mQuestMode;
        bool mOptionsMode;
        bool mTopicsMode;
        bool mAllQuests;

        template <typename T>
        T* getWidget(std::string_view name)
        {
            T* widget;
            WindowBase::getWidget(widget, name);
            return widget;
        }

        template <typename value_type>
        void setText(std::string_view name, value_type const& value)
        {
            getWidget<MyGUI::TextBox>(name)->setCaption(MyGUI::utility::toString(value));
        }

        void setVisible(std::string_view name, bool visible) { getWidget<MyGUI::Widget>(name)->setVisible(visible); }

        void adviseButtonClick(std::string_view name, void (JournalWindowImpl::*handler)(MyGUI::Widget*))
        {
            getWidget<MyGUI::Widget>(name)->eventMouseButtonClick += newDelegate(this, handler);
        }

        void adviseKeyPress(
            std::string_view name, void (JournalWindowImpl::*handler)(MyGUI::Widget*, MyGUI::KeyCode, MyGUI::Char))
        {
            getWidget<MyGUI::Widget>(name)->eventKeyButtonPressed += newDelegate(this, handler);
        }

        MWGui::BookPage* getPage(std::string_view name) { return getWidget<MWGui::BookPage>(name); }

        JournalWindowImpl(MWGui::JournalViewModel::Ptr model, bool questList, ToUTF8::FromType encoding)
            : JournalBooks(std::move(model), encoding)
            , JournalWindow()
        {
            center();

            adviseButtonClick(OptionsBTN, &JournalWindowImpl::notifyOptions);
            adviseButtonClick(PrevPageBTN, &JournalWindowImpl::notifyPrevPage);
            adviseButtonClick(NextPageBTN, &JournalWindowImpl::notifyNextPage);
            adviseButtonClick(CloseBTN, &JournalWindowImpl::notifyClose);
            adviseButtonClick(JournalBTN, &JournalWindowImpl::notifyJournal);

            adviseButtonClick(TopicsBTN, &JournalWindowImpl::notifyTopics);
            adviseButtonClick(QuestsBTN, &JournalWindowImpl::notifyQuests);
            adviseButtonClick(CancelBTN, &JournalWindowImpl::notifyCancel);

            adviseButtonClick(ShowAllBTN, &JournalWindowImpl::notifyShowAll);
            adviseButtonClick(ShowActiveBTN, &JournalWindowImpl::notifyShowActive);

            adviseKeyPress(OptionsBTN, &JournalWindowImpl::notifyKeyPress);
            adviseKeyPress(PrevPageBTN, &JournalWindowImpl::notifyKeyPress);
            adviseKeyPress(NextPageBTN, &JournalWindowImpl::notifyKeyPress);
            adviseKeyPress(CloseBTN, &JournalWindowImpl::notifyKeyPress);
            adviseKeyPress(JournalBTN, &JournalWindowImpl::notifyKeyPress);

            Gui::MWList* list = getWidget<Gui::MWList>(QuestsList);
            list->eventItemSelected += MyGUI::newDelegate(this, &JournalWindowImpl::notifyQuestClicked);

            Gui::MWList* topicsList = getWidget<Gui::MWList>(TopicsList);
            topicsList->eventItemSelected += MyGUI::newDelegate(this, &JournalWindowImpl::notifyTopicSelected);

            {
                MWGui::BookPage::ClickCallback callback = [this](intptr_t linkId) { notifyTopicClicked(linkId); };

                getPage(LeftBookPage)->adviseLinkClicked(callback);
                getPage(RightBookPage)->adviseLinkClicked(std::move(callback));

                getPage(LeftBookPage)->eventMouseWheel
                    += MyGUI::newDelegate(this, &JournalWindowImpl::notifyMouseWheel);
                getPage(RightBookPage)->eventMouseWheel
                    += MyGUI::newDelegate(this, &JournalWindowImpl::notifyMouseWheel);
            }

            {
                MWGui::BookPage::ClickCallback callback
                    = [this](MWGui::TypesetBook::InteractiveId index) { notifyIndexLinkClicked(index); };

                getPage(LeftTopicIndex)->adviseLinkClicked(callback);
                getPage(CenterTopicIndex)->adviseLinkClicked(callback);
                getPage(RightTopicIndex)->adviseLinkClicked(std::move(callback));
            }

            adjustButton(PrevPageBTN);
            float nextButtonScale = adjustButton(NextPageBTN);
            adjustButton(CloseBTN);
            adjustButton(CancelBTN);
            adjustButton(JournalBTN);

            Gui::ImageButton* optionsButton = getWidget<Gui::ImageButton>(OptionsBTN);
            Gui::ImageButton* showActiveButton = getWidget<Gui::ImageButton>(ShowActiveBTN);
            Gui::ImageButton* showAllButton = getWidget<Gui::ImageButton>(ShowAllBTN);
            Gui::ImageButton* questsButton = getWidget<Gui::ImageButton>(QuestsBTN);

            Gui::ImageButton* nextButton = getWidget<Gui::ImageButton>(NextPageBTN);
            if (nextButton->getSize().width == 64)
            {
                // english button has a 7 pixel wide strip of garbage on its right edge
                nextButton->setSize(64 - 7, nextButton->getSize().height);
                nextButton->setImageCoord(
                    MyGUI::IntCoord(0, 0, (64 - 7) * nextButtonScale, nextButton->getSize().height * nextButtonScale));
            }

            if (!questList)
            {
                // If tribunal is not installed (-> no options button), we still want the Topics button available,
                // so place it where the options button would have been
                Gui::ImageButton* topicsButton = getWidget<Gui::ImageButton>(TopicsBTN);
                topicsButton->detachFromWidget();
                topicsButton->attachToWidget(optionsButton->getParent());
                topicsButton->setPosition(optionsButton->getPosition());
                topicsButton->eventMouseButtonClick.clear();
                topicsButton->eventMouseButtonClick += MyGUI::newDelegate(this, &JournalWindowImpl::notifyOptions);

                optionsButton->setVisible(false);
                showActiveButton->setVisible(false);
                showAllButton->setVisible(false);
                questsButton->setVisible(false);

                adjustButton(TopicsBTN);
            }
            else
            {
                optionsButton->setImage("textures/tx_menubook_options.dds");
                showActiveButton->setImage("textures/tx_menubook_quests_active.dds");
                showAllButton->setImage("textures/tx_menubook_quests_all.dds");
                questsButton->setImage("textures/tx_menubook_quests.dds");

                adjustButton(ShowAllBTN);
                adjustButton(ShowActiveBTN);
                adjustButton(OptionsBTN);
                adjustButton(QuestsBTN);
                adjustButton(TopicsBTN);
                int topicsWidth = getWidget<MyGUI::Widget>(TopicsBTN)->getSize().width;
                int cancelLeft = getWidget<MyGUI::Widget>(CancelBTN)->getPosition().left;
                int cancelRight = getWidget<MyGUI::Widget>(CancelBTN)->getPosition().left
                    + getWidget<MyGUI::Widget>(CancelBTN)->getSize().width;

                getWidget<MyGUI::Widget>(QuestsBTN)->setPosition(
                    cancelRight, getWidget<MyGUI::Widget>(QuestsBTN)->getPosition().top);

                // Usually Topics, Quests, and Cancel buttons have the 64px width, so we can place the Topics left-up
                // from the Cancel button, and the Quests right-up from the Cancel button. But in some installations,
                // e.g. German one, the Topics button has the 128px width, so we should place it exactly left from the
                // Quests button.
                if (topicsWidth == 64)
                {
                    getWidget<MyGUI::Widget>(TopicsBTN)->setPosition(
                        cancelLeft - topicsWidth, getWidget<MyGUI::Widget>(TopicsBTN)->getPosition().top);
                }
                else
                {
                    int questLeft = getWidget<MyGUI::Widget>(QuestsBTN)->getPosition().left;
                    getWidget<MyGUI::Widget>(TopicsBTN)->setPosition(
                        questLeft - topicsWidth, getWidget<MyGUI::Widget>(TopicsBTN)->getPosition().top);
                }
            }

            // Latin = 26 (13 + 13)
            mIndexRowCount = 13;
            bool isRussian = (mEncoding == ToUTF8::WINDOWS_1251);
            if (isRussian) // Cyrillic is either (10 + 10 + 10) or (15 + 15)
                mIndexRowCount = MWGui::getCyrillicIndexPageCount();

            mControllerButtons.mA = "#{sSelect}";
            mControllerButtons.mX = "#{OMWEngine:JournalQuests}";
            mControllerButtons.mY = "#{sTopics}";

            mQuestMode = false;
            mAllQuests = false;
            mOptionsMode = false;
            mTopicsMode = false;
        }

        void onOpen() override
        {
            mModel->load();

            setBookMode();

            Book journalBook;
            if (mModel->isEmpty())
                journalBook = createEmptyJournalBook();
            else
                journalBook = createJournalBook();

            pushBook(journalBook, 0);

            // fast forward to the last page
            if (!mStates.empty())
            {
                unsigned int& page = mStates.top().mPage;
                page = mStates.top().mBook->pageCount() - 1;
                if (page % 2)
                    --page;
            }
            updateShowingPages();

            if (Settings::gui().mControllerMenus)
                setControllerFocusedQuest(0);

            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(getWidget<MyGUI::Widget>(CloseBTN));
        }

        void onClose() override
        {
            mModel->unload();

            getPage(LeftBookPage)->showPage(Book(), 0);
            getPage(RightBookPage)->showPage(Book(), 0);

            while (!mStates.empty())
                mStates.pop();

            mTopicIndexBook.reset();
        }

        void setVisible(bool newValue) override { WindowBase::setVisible(newValue); }

        void setBookMode()
        {
            mOptionsMode = false;
            mTopicsMode = false;
            setVisible(OptionsBTN, true);
            setVisible(OptionsOverlay, false);

            updateShowingPages();
            updateCloseJournalButton();

            MWBase::Environment::get().getWindowManager()->updateControllerButtonsOverlay();
        }

        void setOptionsMode()
        {
            mOptionsMode = true;
            mTopicsMode = false;

            setVisible(OptionsBTN, false);
            setVisible(OptionsOverlay, true);

            setVisible(PrevPageBTN, false);
            setVisible(NextPageBTN, false);
            setVisible(CloseBTN, false);
            setVisible(JournalBTN, false);

            setVisible(TopicsList, false);
            setVisible(QuestsList, mQuestMode);
            setVisible(LeftTopicIndex, !mQuestMode);
            setVisible(CenterTopicIndex, !mQuestMode);
            setVisible(RightTopicIndex, !mQuestMode);
            setVisible(ShowAllBTN, mQuestMode && !mAllQuests);
            setVisible(ShowActiveBTN, mQuestMode && mAllQuests);

            // TODO: figure out how to make "options" page overlay book page
            //       correctly, so that text may show underneath
            getPage(RightBookPage)->showPage(Book(), 0);

            // If in quest mode, ensure the quest list is updated
            if (mQuestMode)
                notifyQuests(getWidget<MyGUI::Widget>(QuestsList));
            else
                notifyTopics(getWidget<MyGUI::Widget>(TopicsList));

            MWBase::Environment::get().getWindowManager()->updateControllerButtonsOverlay();
        }

        void pushBook(Book& book, unsigned int page)
        {
            DisplayState bs;
            bs.mPage = page;
            bs.mBook = book;
            mStates.push(bs);
            updateShowingPages();
            updateCloseJournalButton();
        }

        void replaceBook(Book& book, unsigned int page)
        {
            assert(!mStates.empty());
            mStates.top().mBook = book;
            mStates.top().mPage = page;
            updateShowingPages();
        }

        void popBook()
        {
            mStates.pop();
            updateShowingPages();
            updateCloseJournalButton();
        }

        void updateCloseJournalButton()
        {
            setVisible(CloseBTN, mStates.size() < 2);
            setVisible(JournalBTN, mStates.size() >= 2);
            MWBase::Environment::get().getWindowManager()->updateControllerButtonsOverlay();
        }

        void updateShowingPages()
        {
            Book book;
            unsigned int page;
            unsigned int relPages;

            if (!mStates.empty())
            {
                book = mStates.top().mBook;
                page = mStates.top().mPage;
                relPages = book->pageCount() - page;
            }
            else
            {
                page = 0;
                relPages = 0;
            }

            MyGUI::Widget* nextPageBtn = getWidget<MyGUI::Widget>(NextPageBTN);
            MyGUI::Widget* prevPageBtn = getWidget<MyGUI::Widget>(PrevPageBTN);

            MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
            bool nextPageVisible = relPages > 2;
            nextPageBtn->setVisible(nextPageVisible);
            bool prevPageVisible = page > 0;
            prevPageBtn->setVisible(prevPageVisible);

            if (focus == nextPageBtn && !nextPageVisible && prevPageVisible)
                MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(prevPageBtn);
            else if (focus == prevPageBtn && !prevPageVisible && nextPageVisible)
                MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(nextPageBtn);

            setVisible(PageOneNum, relPages > 0);
            setVisible(PageTwoNum, relPages > 1);

            getPage(LeftBookPage)->showPage((relPages > 0) ? book : Book(), page + 0);
            getPage(RightBookPage)->showPage((relPages > 0) ? std::move(book) : Book(), page + 1);

            setText(PageOneNum, page + 1);
            setText(PageTwoNum, page + 2);

            MWBase::Environment::get().getWindowManager()->updateControllerButtonsOverlay();
        }

        void notifyKeyPress(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character)
        {
            if (key == MyGUI::KeyCode::ArrowUp)
                notifyPrevPage(sender);
            else if (key == MyGUI::KeyCode::ArrowDown)
                notifyNextPage(sender);
        }

        void notifyTopicClicked(intptr_t linkId)
        {
            Book topicBook = createTopicBook(linkId);

            if (mStates.size() > 1)
                replaceBook(topicBook, 0);
            else
                pushBook(topicBook, 0);

            setVisible(OptionsOverlay, false);
            setVisible(OptionsBTN, true);
            setVisible(JournalBTN, true);

            mOptionsMode = false;
            mTopicsMode = false;

            MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("book page"));
            MWBase::Environment::get().getWindowManager()->updateControllerButtonsOverlay();
        }

        void notifyTopicSelected(const std::string& topicIdString, int id)
        {
            ESM::RefId topic = ESM::RefId::stringRefId(topicIdString);
            const MWBase::Journal* journal = MWBase::Environment::get().getJournal();
            intptr_t topicId = 0; /// \todo get rid of intptr ids
            const auto it = journal->getTopics().find(topic);
            if (it != journal->getTopics().end())
                topicId = intptr_t(&it->second);

            notifyTopicClicked(topicId);
        }

        void notifyQuestClicked(const std::string& name, int id)
        {
            Book book = createQuestBook(name);

            if (mStates.size() > 1)
                replaceBook(book, 0);
            else
                pushBook(book, 0);

            setVisible(OptionsOverlay, false);
            setVisible(OptionsBTN, true);
            setVisible(JournalBTN, true);

            mOptionsMode = false;

            MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("book page"));
            MWBase::Environment::get().getWindowManager()->updateControllerButtonsOverlay();
        }

        void notifyOptions(MyGUI::Widget* /*sender*/)
        {
            setOptionsMode();

            if (!mTopicIndexBook)
                mTopicIndexBook = createTopicIndexBook();

            if (mIndexPagesCount == 3)
            {
                getPage(LeftTopicIndex)->showPage(mTopicIndexBook, 0);
                getPage(CenterTopicIndex)->showPage(mTopicIndexBook, 1);
                getPage(RightTopicIndex)->showPage(mTopicIndexBook, 2);
            }
            else
            {
                getPage(LeftTopicIndex)->showPage(mTopicIndexBook, 0);
                getPage(RightTopicIndex)->showPage(mTopicIndexBook, 1);
            }

            if (Settings::gui().mControllerMenus)
                setIndexControllerFocus(true);
        }

        void notifyJournal(MyGUI::Widget* /*sender*/)
        {
            assert(mStates.size() > 1);
            popBook();

            MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("book page"));
            MWBase::Environment::get().getWindowManager()->updateControllerButtonsOverlay();
        }

        void addControllerButtons(Gui::MWList* list, size_t selectedIndex)
        {
            mButtons.clear();
            for (size_t i = 0; i < list->getItemCount(); i++)
            {
                MyGUI::Button* listItem = list->getItemWidget(list->getItemNameAt(i));
                if (listItem)
                {
                    listItem->setTextColour(
                        mButtons.size() == selectedIndex ? MWGui::journalHeaderColour : MyGUI::Colour::Black);
                    mButtons.push_back(listItem);
                }
            }
        }

        void notifyIndexLinkClicked(MWGui::TypesetBook::InteractiveId index)
        {
            setVisible(LeftTopicIndex, false);
            setVisible(CenterTopicIndex, false);
            setVisible(RightTopicIndex, false);
            setVisible(TopicsList, true);

            mTopicsMode = true;

            Gui::MWList* list = getWidget<Gui::MWList>(TopicsList);
            list->clear();

            AddNamesToList add(list);

            mModel->visitTopicNamesStartingWith(index, add);

            list->adjustSize();

            if (Settings::gui().mControllerMenus)
            {
                setControllerFocusedQuest(0);
                addControllerButtons(list, mSelectedQuest);
            }

            MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("book page"));
            MWBase::Environment::get().getWindowManager()->updateControllerButtonsOverlay();
        }

        void notifyTopics(MyGUI::Widget* /*sender*/)
        {
            mQuestMode = false;
            mTopicsMode = false;
            setVisible(LeftTopicIndex, true);
            setVisible(CenterTopicIndex, true);
            setVisible(RightTopicIndex, true);
            setVisible(TopicsList, false);
            setVisible(QuestsList, false);
            setVisible(ShowAllBTN, false);
            setVisible(ShowActiveBTN, false);

            MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("book page"));
            MWBase::Environment::get().getWindowManager()->updateControllerButtonsOverlay();
        }

        struct AddNamesToList
        {
            AddNamesToList(Gui::MWList* list)
                : mList(list)
            {
            }

            Gui::MWList* mList;
            void operator()(std::string_view name, bool finished = false) { mList->addItem(name); }
        };
        struct SetNamesInactive
        {
            SetNamesInactive(Gui::MWList* list)
                : mList(list)
            {
            }

            Gui::MWList* mList;
            void operator()(std::string_view name, bool finished)
            {
                if (finished)
                {
                    mList->getItemWidget(name)->setStateSelected(true);
                }
            }
        };

        void notifyQuests(MyGUI::Widget* /*sender*/)
        {
            mQuestMode = true;

            setVisible(LeftTopicIndex, false);
            setVisible(CenterTopicIndex, false);
            setVisible(RightTopicIndex, false);
            setVisible(TopicsList, false);
            setVisible(QuestsList, true);
            setVisible(ShowAllBTN, !mAllQuests);
            setVisible(ShowActiveBTN, mAllQuests);

            Gui::MWList* list = getWidget<Gui::MWList>(QuestsList);
            list->clear();

            AddNamesToList add(list);

            mModel->visitQuestNames(!mAllQuests, add);

            list->sort();
            list->adjustSize();

            if (Settings::gui().mControllerMenus)
            {
                addControllerButtons(list, mSelectedQuest);
                setControllerFocusedQuest(MWGui::wrap(mSelectedQuest, mButtons.size()));
            }

            if (mAllQuests)
            {
                SetNamesInactive setInactive(list);
                mModel->visitQuestNames(false, setInactive);
            }

            MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("book page"));
            MWBase::Environment::get().getWindowManager()->updateControllerButtonsOverlay();
        }

        void notifyShowAll(MyGUI::Widget* sender)
        {
            mAllQuests = true;
            notifyQuests(sender);
        }

        void notifyShowActive(MyGUI::Widget* sender)
        {
            mAllQuests = false;
            notifyQuests(sender);
        }

        void notifyCancel(MyGUI::Widget* sender)
        {
            if (mTopicsMode)
            {
                notifyTopics(sender);
            }
            else
            {
                setBookMode();
                MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("book page"));
            }
        }

        void notifyClose(MyGUI::Widget* /*sender*/)
        {
            MWBase::WindowManager* winMgr = MWBase::Environment::get().getWindowManager();
            winMgr->playSound(ESM::RefId::stringRefId("book close"));
            winMgr->popGuiMode();
        }

        void notifyMouseWheel(MyGUI::Widget* sender, int rel)
        {
            if (rel < 0)
                notifyNextPage(sender);
            else
                notifyPrevPage(sender);
        }

        void notifyNextPage(MyGUI::Widget* /*sender*/)
        {
            if (mOptionsMode)
                return;
            if (!mStates.empty())
            {
                unsigned int& page = mStates.top().mPage;
                Book book = mStates.top().mBook;

                if (page + 2 < book->pageCount())
                {
                    MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("book page"));

                    page += 2;
                    updateShowingPages();
                }
            }
        }

        void notifyPrevPage(MyGUI::Widget* /*sender*/)
        {
            if (mOptionsMode)
                return;
            if (!mStates.empty())
            {
                unsigned int& page = mStates.top().mPage;

                if (page >= 2)
                {
                    MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("book page"));

                    page -= 2;
                    updateShowingPages();
                }
            }
        }

        MWGui::ControllerButtons* getControllerButtons() override
        {
            mControllerButtons.mB = mOptionsMode || mStates.size() > 1 ? "#{sBack}" : "#{Interface:Close}";
            mControllerButtons.mL1 = mOptionsMode ? "" : "#{sPrev}";
            mControllerButtons.mR1 = mOptionsMode ? "" : "#{sNext}";
            mControllerButtons.mR3 = mOptionsMode && mQuestMode ? "#{OMWEngine:JournalShowAll}" : "";
            return &mControllerButtons;
        }

        void setIndexControllerFocus(bool focused)
        {
            int col = mSelectedIndex / mIndexRowCount;
            int row = mSelectedIndex % mIndexRowCount;
            mTopicIndexBook->setColour(col, row, 0, focused ? MWGui::journalHeaderColour : MyGUI::Colour::Black);
        }

        void moveSelectedIndex(int offset)
        {
            setIndexControllerFocus(false);

            int numChars = mEncoding == ToUTF8::WINDOWS_1251 ? 30 : 26;
            int col = mSelectedIndex / mIndexRowCount;

            if (offset == -1) // Up
            {
                if (mSelectedIndex % mIndexRowCount == 0)
                    mSelectedIndex = (col * mIndexRowCount) + mIndexRowCount - 1;
                else
                    mSelectedIndex--;
            }
            else if (offset == 1) // Down
            {
                if (mSelectedIndex % mIndexRowCount == mIndexRowCount - 1)
                    mSelectedIndex = col * mIndexRowCount;
                else
                    mSelectedIndex++;
            }
            else
            {
                // mSelectedIndex is unsigned, so we have to be careful with our math.
                if (offset < 0)
                    offset += numChars;

                mSelectedIndex = (mSelectedIndex + offset) % numChars;
            }

            setIndexControllerFocus(true);
            setText(PageOneNum, 1); // Redraw the list
        }

        bool optionsModeButtonHandler(const SDL_ControllerButtonEvent& arg)
        {
            if (arg.button == SDL_CONTROLLER_BUTTON_A) // A: Mouse click or Select
            {
                if (mQuestMode)
                {
                    // Choose a quest
                    Gui::MWList* list = getWidget<Gui::MWList>(QuestsList);
                    if (mSelectedQuest < list->getItemCount())
                        notifyQuestClicked(list->getItemNameAt(mSelectedQuest), 0);
                }
                else if (mTopicsMode)
                {
                    // Choose a topic
                    Gui::MWList* list = getWidget<Gui::MWList>(TopicsList);
                    if (mSelectedQuest < list->getItemCount())
                        notifyTopicSelected(list->getItemNameAt(mSelectedQuest), 0);
                }
                else
                {
                    // Choose an index. Cyrillic capital A is a 0xd090 in UTF-8.
                    // Words can not be started with characters 26 or 28.
                    int russianOffset = 0xd090;
                    if (mSelectedIndex >= 26)
                        russianOffset++;
                    if (mSelectedIndex >= 27)
                        russianOffset++; // 27, not 28, because of skipping char 26
                    bool isRussian = (mEncoding == ToUTF8::WINDOWS_1251);
                    notifyIndexLinkClicked(isRussian ? mSelectedIndex + russianOffset : mSelectedIndex + 'A');
                }
            }
            else if (arg.button == SDL_CONTROLLER_BUTTON_B) // B: Back
            {
                // Hide the options overlay
                notifyCancel(getWidget<MyGUI::Widget>(CancelBTN));
                mQuestMode = false;
            }
            else if (arg.button == SDL_CONTROLLER_BUTTON_X) // X: Quests
            {
                if (mQuestMode)
                {
                    // Hide the quest overlay if visible
                    notifyCancel(getWidget<MyGUI::Widget>(CancelBTN));
                    mQuestMode = false;
                }
                else
                {
                    // Show the quest overlay if viewing the topics list
                    notifyQuests(getWidget<MyGUI::Widget>(QuestsBTN));
                }
            }
            else if (arg.button == SDL_CONTROLLER_BUTTON_Y) // Y: Topics
            {
                if (!mQuestMode)
                {
                    // Hide the topics overlay if visible
                    notifyCancel(getWidget<MyGUI::Widget>(CancelBTN));
                }
                else
                {
                    // Show the topics overlay if viewing the quest list
                    notifyTopics(getWidget<MyGUI::Widget>(TopicsBTN));
                }
            }
            else if (arg.button == SDL_CONTROLLER_BUTTON_RIGHTSTICK && mQuestMode) // R3: Show All/Some
            {
                if (mAllQuests)
                    notifyShowActive(getWidget<MyGUI::Widget>(ShowActiveBTN));
                else
                    notifyShowAll(getWidget<MyGUI::Widget>(ShowAllBTN));
            }
            else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_UP)
            {
                if (mQuestMode || mTopicsMode)
                {
                    if (mButtons.size() <= 1)
                        return true;

                    // Scroll through the list of quests or topics
                    setControllerFocusedQuest(MWGui::wrap(mSelectedQuest - 1, mButtons.size()));
                }
                else
                    moveSelectedIndex(-1);
            }
            else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
            {
                if (mQuestMode || mTopicsMode)
                {
                    if (mButtons.size() <= 1)
                        return true;

                    // Scroll through the list of quests or topics
                    setControllerFocusedQuest(MWGui::wrap(mSelectedQuest + 1, mButtons.size()));
                }
                else
                    moveSelectedIndex(1);
            }
            else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT && !mQuestMode && !mTopicsMode)
                moveSelectedIndex(-mIndexRowCount);
            else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT && !mQuestMode && !mTopicsMode)
                moveSelectedIndex(mIndexRowCount);
            else if (arg.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER && (mQuestMode || mTopicsMode))
            {
                // Scroll up 5 items in the list of quests or topics
                setControllerFocusedQuest(std::max(static_cast<int>(mSelectedQuest) - 5, 0));
            }
            else if (arg.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER && (mQuestMode || mTopicsMode))
            {
                // Scroll down 5 items in the list of quests or topics
                setControllerFocusedQuest(std::min(mSelectedQuest + 5, mButtons.size() - 1));
            }

            return true;
        }

        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override
        {
            // If the topics or quest list is open, it should handle the buttons.
            if (mOptionsMode)
                return optionsModeButtonHandler(arg);

            if (arg.button == SDL_CONTROLLER_BUTTON_A)
                return false;
            else if (arg.button == SDL_CONTROLLER_BUTTON_B) // B: Back
            {
                if (mStates.size() > 1)
                {
                    // Pop the current book. If in quest mode, reopen the quest list.
                    notifyJournal(getWidget<MyGUI::Widget>(JournalBTN));
                    if (mQuestMode)
                    {
                        notifyOptions(getWidget<MyGUI::Widget>(OptionsBTN));
                        notifyQuests(getWidget<MyGUI::Widget>(QuestsBTN));
                    }
                }
                else
                {
                    // Close the journal window
                    notifyClose(getWidget<MyGUI::Widget>(CloseBTN));
                }
            }
            else if (arg.button == SDL_CONTROLLER_BUTTON_X) // X: Quests
            {
                // Show the quest overlay
                notifyOptions(getWidget<MyGUI::Widget>(OptionsBTN));
                if (!mQuestMode)
                    notifyQuests(getWidget<MyGUI::Widget>(QuestsBTN));
            }
            else if (arg.button == SDL_CONTROLLER_BUTTON_Y) // Y: Topics
            {
                // Show the topics overlay
                notifyOptions(getWidget<MyGUI::Widget>(OptionsBTN));
                if (mQuestMode)
                    notifyTopics(getWidget<MyGUI::Widget>(TopicsBTN));
            }
            else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT || arg.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
                notifyPrevPage(getWidget<MyGUI::Widget>(PrevPageBTN));
            else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT
                || arg.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)
                notifyNextPage(getWidget<MyGUI::Widget>(NextPageBTN));

            return true;
        }

        void setControllerFocusedQuest(size_t index)
        {
            size_t listSize = mButtons.size();
            if (mSelectedQuest < listSize)
                mButtons[mSelectedQuest]->setTextColour(MyGUI::Colour::Black);

            mSelectedQuest = index;
            if (mSelectedQuest < listSize)
            {
                mButtons[mSelectedQuest]->setTextColour(MWGui::journalHeaderColour);

                // Scroll the list to keep the active item in view
                Gui::MWList* list = getWidget<Gui::MWList>(mQuestMode ? QuestsList : TopicsList);
                int offset = 0;
                for (int i = 4; i < static_cast<int>(mSelectedQuest); i++)
                    offset += mButtons[i]->getHeight();
                list->setViewOffset(-offset);
            }
        }
    };
}

// glue the implementation to the interface
std::unique_ptr<MWGui::JournalWindow> MWGui::JournalWindow::create(
    JournalViewModel::Ptr model, bool questList, ToUTF8::FromType encoding)
{
    return std::make_unique<JournalWindowImpl>(model, questList, encoding);
}

MWGui::JournalWindow::JournalWindow()
    : BookWindowBase("openmw_journal.layout")
{
}
