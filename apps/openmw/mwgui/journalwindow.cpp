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

using namespace MyGUI;
using namespace MWGui;

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

    struct JournalWindowImpl : WindowBase, JournalBooks, JournalWindow
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
            getWidget <TextBox> (name) ->
                setCaption (boost::lexical_cast <std::string> (value));
        }

        void setVisible (char const * name, bool visible)
        {
            getWidget <Widget> (name) ->
                setVisible (visible);
        }

        void adviseButtonClick (char const * name, void (JournalWindowImpl::*Handler) (Widget* _sender))
        {
            getWidget <MWGui::ImageButton> (name) ->
                eventMouseButtonClick += newDelegate(this, Handler);
        }

        MWGui::BookPage* getPage (char const * name)
        {
            return getWidget <MWGui::BookPage> (name);
        }

        JournalWindowImpl (JournalViewModel::Ptr Model)
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
                BookPage::ClickCallback callback;
                
                callback = boost::bind (&JournalWindowImpl::notifyTopicClicked, this, _1);

                getPage (TopicsPage)->adviseLinkClicked (callback);
                getPage (LeftBookPage)->adviseLinkClicked (callback);
                getPage (RightBookPage)->adviseLinkClicked (callback);
            }

            {
                BookPage::ClickCallback callback;
                
                callback = boost::bind (&JournalWindowImpl::notifyIndexLinkClicked, this, _1);

                getPage (LeftTopicIndex)->adviseLinkClicked (callback);
                getPage (RightTopicIndex)->adviseLinkClicked (callback);
            }

            {
                BookPage::ClickCallback callback;
                
                callback = boost::bind (&JournalWindowImpl::notifyQuestClicked, this, _1);

                getPage (QuestsPage)->adviseLinkClicked (callback);
            }

            mQuestMode = false;
            mAllQuests = false;
        }

        void open()
        {
            mModel->load ();

            setBookMode ();

            MWBase::Environment::get().getSoundManager()->playSound ("book open", 1.0, 1.0);

            Book journalBook;
            if (mModel->isEmpty ())
                journalBook = createEmptyJournalBook ();
            else
                journalBook = createJournalBook ();

            pushBook (journalBook, 0);
        }

        void close()
        {
            mModel->unload ();

            getPage (LeftBookPage)->showPage (Book (), 0);
            getPage (RightBookPage)->showPage (Book (), 0);

            while (!mStates.empty ())
                mStates.pop ();

            mTopicIndexBook.reset ();

            MWBase::Environment::get().getSoundManager()->playSound ("book close", 1.0, 1.0);
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

        void notifyOptions(Widget* _sender)
        {
            setOptionsMode ();

            if (!mTopicIndexBook)
                mTopicIndexBook = createTopicIndexBook ();

            getPage (LeftTopicIndex)->showPage (mTopicIndexBook, 0);
            getPage (RightTopicIndex)->showPage (mTopicIndexBook, 1);
        }

        void notifyJournal(Widget* _sender)
        {
            assert (mStates.size () > 1);
            popBook ();
        }

        void showList (char const * listId, char const * pageId, Book book)
        {
            std::pair <int, int> size = book->getSize ();

            getPage (pageId)->showPage (book, 0);

            getWidget <ScrollView> (listId)->setCanvasSize (size.first, size.second);
        }

        void notifyIndexLinkClicked (TypesetBook::InteractiveId character)
        {
            setVisible (LeftTopicIndex, false);
            setVisible (RightTopicIndex, false);
            setVisible (TopicsList, true);

            showList (TopicsList, TopicsPage, createTopicIndexBook ((char)character));
        }

        void notifyTopics(Widget* _sender)
        {
            mQuestMode = false;
            setVisible (LeftTopicIndex, true);
            setVisible (RightTopicIndex, true);
            setVisible (TopicsList, false);
            setVisible (QuestsList, false);
            setVisible (ShowAllBTN, false);
            setVisible (ShowActiveBTN, false);
        }

        void notifyQuests(Widget* _sender)
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

        void notifyShowAll(Widget* _sender)
        {
            mAllQuests = true;
            setVisible (ShowAllBTN, !mAllQuests);
            setVisible (ShowActiveBTN, mAllQuests);
            showList (QuestsList, QuestsPage, createQuestIndexBook (!mAllQuests));
        }

        void notifyShowActive(Widget* _sender)
        {
            mAllQuests = false;
            setVisible (ShowAllBTN, !mAllQuests);
            setVisible (ShowActiveBTN, mAllQuests);
            showList (QuestsList, QuestsPage, createQuestIndexBook (!mAllQuests));
        }

        void notifyCancel(Widget* _sender)
        {
            setBookMode ();
        }

        void notifyClose(Widget* _sender)
        {

            MWBase::Environment::get().getWindowManager ()->popGuiMode ();
        }

        void notifyNextPage(Widget* _sender)
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

        void notifyPrevPage(Widget* _sender)
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
JournalWindow * MWGui::JournalWindow::create (JournalViewModel::Ptr Model)
{
    return new JournalWindowImpl (Model);
}
