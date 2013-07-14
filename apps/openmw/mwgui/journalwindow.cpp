#include "journalwindow.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "list.hpp"

#include <sstream>
#include <set>
#include <stack>
#include <string>
#include <utility>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include "boost/lexical_cast.hpp"

#include "bookpage.hpp"
#include "windowbase.hpp"
#include "imagebutton.hpp"
#include "journalviewmodel.hpp"
#include "journalbooks.hpp"

namespace
{
    static char const OptionsOverlay [] = "OptionsOverlay";
    static char const OptionsBTN [] = "OptionsBTN";
    static char const PrevPageBTN [] = "PrevPageBTN";
    static char const NextPageBTN [] = "NextPageBTN";
    static char const CloseBTN [] = "CloseBTN";
    static char const JournalBTN [] = "JournalBTN";
    static char const TopicsBTN [] = "TopicsBTN";
    static char const QuestsBTN [] = "QuestsBTN";
    static char const CancelBTN [] = "CancelBTN";
    static char const ShowAllBTN [] = "ShowAllBTN";
    static char const ShowActiveBTN [] = "ShowActiveBTN";
    static char const PageOneNum [] = "PageOneNum";
    static char const PageTwoNum [] = "PageTwoNum";
    static char const TopicsList [] = "TopicsList";
    static char const TopicsPage [] = "TopicsPage";
    static char const QuestsList [] = "QuestsList";
    static char const QuestsPage [] = "QuestsPage";
    static char const LeftBookPage [] = "LeftBookPage";
    static char const RightBookPage [] = "RightBookPage";
    static char const LeftTopicIndex [] = "LeftTopicIndex";
    static char const RightTopicIndex [] = "RightTopicIndex";

    struct JournalWindowImpl : MWGui::WindowBase, MWGui::JournalBooks, MWGui::JournalWindow
    {
        struct DisplayState
        {
            unsigned int mPage;
            Book mBook;
        };

        typedef std::stack <DisplayState> DisplayStateStack;

        DisplayStateStack mStates;
        Book mTopicIndexBook;
        bool mQuestMode;
        bool mAllQuests;

        template <typename T>
        T * getWidget (char const * name)
        {
            T * widget;
            WindowBase::getWidget (widget, name);
            return widget;
        }

        template <typename value_type>
        void setText (char const * name, value_type const & value)
        {
            getWidget <MyGUI::TextBox> (name) ->
                setCaption (boost::lexical_cast <std::string> (value));
        }

        void setVisible (char const * name, bool visible)
        {
            getWidget <MyGUI::Widget> (name) ->
                setVisible (visible);
        }

        void adviseButtonClick (char const * name, void (JournalWindowImpl::*Handler) (MyGUI::Widget* _sender))
        {
            getWidget <MWGui::ImageButton> (name) ->
                eventMouseButtonClick += newDelegate(this, Handler);
        }

        MWGui::BookPage* getPage (char const * name)
        {
            return getWidget <MWGui::BookPage> (name);
        }

        JournalWindowImpl (MWGui::JournalViewModel::Ptr Model)
            : WindowBase("openmw_journal.layout"), JournalBooks (Model)
        {
            mMainWidget->setVisible(false);
            center();

            adviseButtonClick (OptionsBTN,    &JournalWindowImpl::notifyOptions   );
            adviseButtonClick (PrevPageBTN,   &JournalWindowImpl::notifyPrevPage  );
            adviseButtonClick (NextPageBTN,   &JournalWindowImpl::notifyNextPage  );
            adviseButtonClick (CloseBTN,      &JournalWindowImpl::notifyClose     );
            adviseButtonClick (JournalBTN,    &JournalWindowImpl::notifyJournal   );

            adviseButtonClick (TopicsBTN,     &JournalWindowImpl::notifyTopics    );
            adviseButtonClick (QuestsBTN,     &JournalWindowImpl::notifyQuests    );
            adviseButtonClick (CancelBTN,     &JournalWindowImpl::notifyCancel    );

            adviseButtonClick (ShowAllBTN,    &JournalWindowImpl::notifyShowAll   );
            adviseButtonClick (ShowActiveBTN, &JournalWindowImpl::notifyShowActive);

            {
                MWGui::BookPage::ClickCallback callback;
                
                callback = boost::bind (&JournalWindowImpl::notifyTopicClicked, this, _1);

                getPage (TopicsPage)->adviseLinkClicked (callback);
                getPage (LeftBookPage)->adviseLinkClicked (callback);
                getPage (RightBookPage)->adviseLinkClicked (callback);
            }

            {
                MWGui::BookPage::ClickCallback callback;
                
                callback = boost::bind (&JournalWindowImpl::notifyIndexLinkClicked, this, _1);

                getPage (LeftTopicIndex)->adviseLinkClicked (callback);
                getPage (RightTopicIndex)->adviseLinkClicked (callback);
            }

            {
                MWGui::BookPage::ClickCallback callback;
                
                callback = boost::bind (&JournalWindowImpl::notifyQuestClicked, this, _1);

                getPage (QuestsPage)->adviseLinkClicked (callback);
            }

            adjustButton(OptionsBTN);
            adjustButton(PrevPageBTN);
            adjustButton(NextPageBTN);
            adjustButton(CloseBTN);
            adjustButton(CancelBTN);
            adjustButton(ShowAllBTN);
            adjustButton(ShowActiveBTN);
            adjustButton(JournalBTN);

            MWGui::ImageButton* nextButton = getWidget<MWGui::ImageButton>(NextPageBTN);
            if (nextButton->getSize().width == 64)
            {
                // english button has a 7 pixel wide strip of garbage on its right edge
                nextButton->setSize(64-7, nextButton->getSize().height);
                nextButton->setImageCoord(MyGUI::IntCoord(0,0,64-7,nextButton->getSize().height));
            }

            adjustButton(TopicsBTN);
            adjustButton(QuestsBTN);
            int width = getWidget<MyGUI::Widget>(TopicsBTN)->getSize().width + getWidget<MyGUI::Widget>(QuestsBTN)->getSize().width;
            int topicsWidth = getWidget<MyGUI::Widget>(TopicsBTN)->getSize().width;
            int pageWidth = getWidget<MyGUI::Widget>(RightBookPage)->getSize().width;

            getWidget<MyGUI::Widget>(TopicsBTN)->setPosition((pageWidth - width)/2, getWidget<MyGUI::Widget>(TopicsBTN)->getPosition().top);
            getWidget<MyGUI::Widget>(QuestsBTN)->setPosition((pageWidth - width)/2 + topicsWidth, getWidget<MyGUI::Widget>(QuestsBTN)->getPosition().top);

            mQuestMode = false;
            mAllQuests = false;
        }

        void adjustButton (char const * name)
        {
            MWGui::ImageButton* button = getWidget<MWGui::ImageButton>(name);

            MyGUI::IntSize diff = button->getSize() - button->getRequestedSize();
            button->setSize(button->getRequestedSize());

            if (button->getAlign().isRight())
                button->setPosition(button->getPosition() + MyGUI::IntPoint(diff.width,0));
        }

        void open()
        {
            if (!MWBase::Environment::get().getWindowManager ()->getJournalAllowed ())
            {
                MWBase::Environment::get().getWindowManager()->popGuiMode ();
            }
            mModel->load ();

            setBookMode ();

            /// \todo Wiping the whole book layout each time the journal is opened is probably too costly for a large journal (eg 300+ pages).
            /// There should be a way to keep the existing layout and append new entries to the end of it.
            /// However, that still leaves the problem of having to add links to previously unknown, but now known topics, so
            /// we maybe need to find another way to speed things up.
            Book journalBook;
            if (mModel->isEmpty ())
                journalBook = createEmptyJournalBook ();
            else
                journalBook = createJournalBook ();

            pushBook (journalBook, 0);

            // fast forward to the last page
            if (!mStates.empty ())
            {
                unsigned int  & page = mStates.top ().mPage;
                page = mStates.top().mBook->pageCount()-1;
                if (page%2)
                    --page;
            }
            updateShowingPages();
        }

        void close()
        {
            mModel->unload ();

            getPage (LeftBookPage)->showPage (Book (), 0);
            getPage (RightBookPage)->showPage (Book (), 0);

            while (!mStates.empty ())
                mStates.pop ();

            mTopicIndexBook.reset ();
        }

        void setVisible (bool newValue)
        {
            WindowBase::setVisible (newValue);
        }

        void setBookMode ()
        {
            setVisible (OptionsBTN, true);
            setVisible (OptionsOverlay, false);

            updateShowingPages ();
            updateCloseJournalButton ();
        }

        void setOptionsMode ()
        {
            setVisible (OptionsBTN, false);
            setVisible (OptionsOverlay, true);

            setVisible (PrevPageBTN, false);
            setVisible (NextPageBTN, false);
            setVisible (CloseBTN, false);
            setVisible (JournalBTN, false);

            setVisible (TopicsList, false);
            setVisible (QuestsList, mQuestMode);
            setVisible (LeftTopicIndex, !mQuestMode);
            setVisible (RightTopicIndex, !mQuestMode);
            setVisible (ShowAllBTN, mQuestMode && !mAllQuests);
            setVisible (ShowActiveBTN, mQuestMode && mAllQuests);

            //TODO: figure out how to make "options" page overlay book page
            //      correctly, so that text may show underneath
            getPage (RightBookPage)->showPage (Book (), 0);
        }

        void pushBook (Book book, unsigned int page)
        {
            DisplayState bs;
            bs.mPage = page;
            bs.mBook = book;
            mStates.push (bs);
            updateShowingPages ();
            updateCloseJournalButton ();
        }

        void replaceBook (Book book, unsigned int page)
        {
            assert (!mStates.empty ());
            mStates.top ().mBook = book;
            mStates.top ().mPage = page;
            updateShowingPages ();
        }

        void popBook ()
        {
            mStates.pop ();
            updateShowingPages ();
            updateCloseJournalButton ();
        }

        void updateCloseJournalButton ()
        {
            setVisible (CloseBTN, mStates.size () < 2);
            setVisible (JournalBTN, mStates.size () >= 2);
        }

        void updateShowingPages ()
        {
            Book book;
            unsigned int page;
            unsigned int relPages;

            if (!mStates.empty ())
            {
                book = mStates.top ().mBook;
                page = mStates.top ().mPage;
                relPages = book->pageCount () - page;
            }
            else
            {
                page = 0;
                relPages = 0;
            }

            setVisible (PrevPageBTN, page > 0);
            setVisible (NextPageBTN, relPages > 2);

            setVisible (PageOneNum, relPages > 0);
            setVisible (PageTwoNum, relPages > 1);

            getPage (LeftBookPage)->showPage ((relPages > 0) ? book : Book (), page+0);
            getPage (RightBookPage)->showPage ((relPages > 0) ? book : Book (), page+1);

            setText (PageOneNum, page + 1);
            setText (PageTwoNum, page + 2);
        }

        void notifyTopicClicked (intptr_t linkId)
        {
            Book topicBook = createTopicBook (linkId);

            if (mStates.size () > 1)
                replaceBook (topicBook, 0);
            else
                pushBook (topicBook, 0);

            setVisible (OptionsOverlay, false);
            setVisible (OptionsBTN, true);
            setVisible (JournalBTN, true);
        }

        void notifyQuestClicked (intptr_t questId)
        {
            Book book = createQuestBook (questId);

            if (mStates.size () > 1)
                replaceBook (book, 0);
            else
                pushBook (book, 0);

            setVisible (OptionsOverlay, false);
            setVisible (OptionsBTN, true);
            setVisible (JournalBTN, true);
        }

        void notifyOptions(MyGUI::Widget* _sender)
        {
            setOptionsMode ();

            if (!mTopicIndexBook)
                mTopicIndexBook = createTopicIndexBook ();

            getPage (LeftTopicIndex)->showPage (mTopicIndexBook, 0);
            getPage (RightTopicIndex)->showPage (mTopicIndexBook, 1);
        }

        void notifyJournal(MyGUI::Widget* _sender)
        {
            assert (mStates.size () > 1);
            popBook ();
        }

        void showList (char const * listId, char const * pageId, Book book)
        {
            std::pair <int, int> size = book->getSize ();

            getPage (pageId)->showPage (book, 0);

            getWidget <MyGUI::ScrollView> (listId)->setCanvasSize (size.first, size.second);
        }

        void notifyIndexLinkClicked (MWGui::TypesetBook::InteractiveId character)
        {
            setVisible (LeftTopicIndex, false);
            setVisible (RightTopicIndex, false);
            setVisible (TopicsList, true);

            showList (TopicsList, TopicsPage, createTopicIndexBook ((char)character));
        }

        void notifyTopics(MyGUI::Widget* _sender)
        {
            mQuestMode = false;
            setVisible (LeftTopicIndex, true);
            setVisible (RightTopicIndex, true);
            setVisible (TopicsList, false);
            setVisible (QuestsList, false);
            setVisible (ShowAllBTN, false);
            setVisible (ShowActiveBTN, false);
        }

        void notifyQuests(MyGUI::Widget* _sender)
        {
            mQuestMode = true;
            setVisible (LeftTopicIndex, false);
            setVisible (RightTopicIndex, false);
            setVisible (TopicsList, false);
            setVisible (QuestsList, true);
            setVisible (ShowAllBTN, !mAllQuests);
            setVisible (ShowActiveBTN, mAllQuests);

            showList (QuestsList, QuestsPage, createQuestIndexBook (!mAllQuests));
        }

        void notifyShowAll(MyGUI::Widget* _sender)
        {
            mAllQuests = true;
            setVisible (ShowAllBTN, !mAllQuests);
            setVisible (ShowActiveBTN, mAllQuests);
            showList (QuestsList, QuestsPage, createQuestIndexBook (!mAllQuests));
        }

        void notifyShowActive(MyGUI::Widget* _sender)
        {
            mAllQuests = false;
            setVisible (ShowAllBTN, !mAllQuests);
            setVisible (ShowActiveBTN, mAllQuests);
            showList (QuestsList, QuestsPage, createQuestIndexBook (!mAllQuests));
        }

        void notifyCancel(MyGUI::Widget* _sender)
        {
            setBookMode ();
        }

        void notifyClose(MyGUI::Widget* _sender)
        {
            MWBase::Environment::get().getSoundManager()->playSound ("book close", 1.0, 1.0);
            MWBase::Environment::get().getWindowManager ()->popGuiMode ();
        }

        void notifyNextPage(MyGUI::Widget* _sender)
        {
            if (!mStates.empty ())
            {
                unsigned int  & page = mStates.top ().mPage;
                Book   book = mStates.top ().mBook;

                if (page < book->pageCount () - 2)
                {
                    page += 2;
                    updateShowingPages ();
                }
            }
        }

        void notifyPrevPage(MyGUI::Widget* _sender)
        {
            if (!mStates.empty ())
            {
                unsigned int & page = mStates.top ().mPage;

                if(page > 0)
                {
                    page -= 2;
                    updateShowingPages ();
                }
            }
        }
    };
}

// glue the implementation to the interface
MWGui::JournalWindow * MWGui::JournalWindow::create (JournalViewModel::Ptr Model)
{
    return new JournalWindowImpl (Model);
}
