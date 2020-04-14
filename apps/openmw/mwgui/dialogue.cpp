#include "dialogue.hpp"

#include <MyGUI_LanguageManager.h>
#include <MyGUI_Window.h>
#include <MyGUI_ProgressBar.h>
#include <MyGUI_ScrollBar.h>
#include <MyGUI_Button.h>

#include <components/debug/debuglog.hpp>
#include <components/widgets/list.hpp>
#include <components/translation/translation.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/dialoguemanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "bookpage.hpp"
#include "textcolours.hpp"

#include "journalbooks.hpp" // to_utf8_span

namespace MWGui
{

    class ResponseCallback : public MWBase::DialogueManager::ResponseCallback
    {
    public:
        ResponseCallback(DialogueWindow* win, bool needMargin=true)
            : mWindow(win)
            , mNeedMargin(needMargin)
        {

        }

        void addResponse(const std::string& title, const std::string& text)
        {
            mWindow->addResponse(title, text, mNeedMargin);
        }

        void updateTopics()
        {
            mWindow->updateTopics();
        }

    private:
        DialogueWindow* mWindow;
        bool mNeedMargin;
    };

    PersuasionDialog::PersuasionDialog(ResponseCallback* callback)
        : WindowModal("openmw_persuasion_dialog.layout")
        , mCallback(callback)
    {
        getWidget(mCancelButton, "CancelButton");
        getWidget(mAdmireButton, "AdmireButton");
        getWidget(mIntimidateButton, "IntimidateButton");
        getWidget(mTauntButton, "TauntButton");
        getWidget(mBribe10Button, "Bribe10Button");
        getWidget(mBribe100Button, "Bribe100Button");
        getWidget(mBribe1000Button, "Bribe1000Button");
        getWidget(mGoldLabel, "GoldLabel");

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PersuasionDialog::onCancel);
        mAdmireButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PersuasionDialog::onPersuade);
        mIntimidateButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PersuasionDialog::onPersuade);
        mTauntButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PersuasionDialog::onPersuade);
        mBribe10Button->eventMouseButtonClick += MyGUI::newDelegate(this, &PersuasionDialog::onPersuade);
        mBribe100Button->eventMouseButtonClick += MyGUI::newDelegate(this, &PersuasionDialog::onPersuade);
        mBribe1000Button->eventMouseButtonClick += MyGUI::newDelegate(this, &PersuasionDialog::onPersuade);
    }

    void PersuasionDialog::onCancel(MyGUI::Widget *sender)
    {
        setVisible(false);
    }

    void PersuasionDialog::onPersuade(MyGUI::Widget *sender)
    {
        MWBase::MechanicsManager::PersuasionType type;
        if (sender == mAdmireButton) type = MWBase::MechanicsManager::PT_Admire;
        else if (sender == mIntimidateButton) type = MWBase::MechanicsManager::PT_Intimidate;
        else if (sender == mTauntButton) type = MWBase::MechanicsManager::PT_Taunt;
        else if (sender == mBribe10Button)
            type = MWBase::MechanicsManager::PT_Bribe10;
        else if (sender == mBribe100Button)
            type = MWBase::MechanicsManager::PT_Bribe100;
        else /*if (sender == mBribe1000Button)*/
            type = MWBase::MechanicsManager::PT_Bribe1000;

        MWBase::Environment::get().getDialogueManager()->persuade(type, mCallback.get());
        mCallback->updateTopics();

        setVisible(false);
    }

    void PersuasionDialog::onOpen()
    {
        center();

        MWWorld::Ptr player = MWMechanics::getPlayer();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        mBribe10Button->setEnabled (playerGold >= 10);
        mBribe100Button->setEnabled (playerGold >= 100);
        mBribe1000Button->setEnabled (playerGold >= 1000);

        mGoldLabel->setCaptionWithReplacing("#{sGold}: " + MyGUI::utility::toString(playerGold));
        WindowModal::onOpen();
    }

    MyGUI::Widget* PersuasionDialog::getDefaultKeyFocus()
    {
        return mAdmireButton;
    }

    // --------------------------------------------------------------------------------------------------

    Response::Response(const std::string &text, const std::string &title, bool needMargin)
        : mTitle(title), mNeedMargin(needMargin)
    {
        mText = text;
    }

    void Response::write(BookTypesetter::Ptr typesetter, KeywordSearchT* keywordSearch, std::map<std::string, Link*>& topicLinks) const
    {
        typesetter->sectionBreak(mNeedMargin ? 9 : 0);

        if (mTitle != "")
        {
            const MyGUI::Colour& headerColour = MWBase::Environment::get().getWindowManager()->getTextColours().header;
            BookTypesetter::Style* title = typesetter->createStyle("", headerColour, false);
            typesetter->write(title, to_utf8_span(mTitle.c_str()));
            typesetter->sectionBreak();
        }

        typedef std::pair<size_t, size_t> Range;
        std::map<Range, intptr_t> hyperLinks;

        // We need this copy for when @# hyperlinks are replaced
        std::string text = mText;

        size_t pos_end;
        for(;;)
        {
            size_t pos_begin = text.find('@');
            if (pos_begin != std::string::npos)
                pos_end = text.find('#', pos_begin);

            if (pos_begin != std::string::npos && pos_end != std::string::npos)
            {
                std::string link = text.substr(pos_begin + 1, pos_end - pos_begin - 1);
                const char specialPseudoAsteriskCharacter = 127;
                std::replace(link.begin(), link.end(), specialPseudoAsteriskCharacter, '*');
                std::string topicName = MWBase::Environment::get().getWindowManager()->
                        getTranslationDataStorage().topicStandardForm(link);

                std::string displayName = link;
                while (displayName[displayName.size()-1] == '*')
                    displayName.erase(displayName.size()-1, 1);

                text.replace(pos_begin, pos_end+1-pos_begin, displayName);

                if (topicLinks.find(Misc::StringUtils::lowerCase(topicName)) != topicLinks.end())
                    hyperLinks[std::make_pair(pos_begin, pos_begin+displayName.size())] = intptr_t(topicLinks[Misc::StringUtils::lowerCase(topicName)]);
            }
            else
                break;
        }

        typesetter->addContent(to_utf8_span(text.c_str()));

        if (hyperLinks.size() && MWBase::Environment::get().getWindowManager()->getTranslationDataStorage().hasTranslation())
        {
            const TextColours& textColours = MWBase::Environment::get().getWindowManager()->getTextColours();

            BookTypesetter::Style* style = typesetter->createStyle("", textColours.normal, false);
            size_t formatted = 0; // points to the first character that is not laid out yet
            for (auto& hyperLink : hyperLinks)
            {
                intptr_t topicId = hyperLink.second;
                BookTypesetter::Style* hotStyle = typesetter->createHotStyle (style, textColours.link,
                                                                              textColours.linkOver, textColours.linkPressed,
                                                                              topicId);
                if (formatted < hyperLink.first.first)
                    typesetter->write(style, formatted, hyperLink.first.first);
                typesetter->write(hotStyle, hyperLink.first.first, hyperLink.first.second);
                formatted = hyperLink.first.second;
            }
            if (formatted < text.size())
                typesetter->write(style, formatted, text.size());
        }
        else
        {
            std::vector<KeywordSearchT::Match> matches;
            keywordSearch->highlightKeywords(text.begin(), text.end(), matches);

            std::string::const_iterator i = text.begin ();
            for (KeywordSearchT::Match& match : matches)
            {
                if (i != match.mBeg)
                    addTopicLink (typesetter, 0, i - text.begin (), match.mBeg - text.begin ());

                addTopicLink (typesetter, match.mValue, match.mBeg - text.begin (), match.mEnd - text.begin ());

                i = match.mEnd;
            }
            if (i != text.end ())
                addTopicLink (typesetter, 0, i - text.begin (), text.size ());
        }
    }

    void Response::addTopicLink(BookTypesetter::Ptr typesetter, intptr_t topicId, size_t begin, size_t end) const
    {
        const TextColours& textColours = MWBase::Environment::get().getWindowManager()->getTextColours();

        BookTypesetter::Style* style = typesetter->createStyle("", textColours.normal, false);


        if (topicId)
            style = typesetter->createHotStyle (style, textColours.link, textColours.linkOver, textColours.linkPressed, topicId);
        typesetter->write (style, begin, end);
    }

    Message::Message(const std::string& text)
    {
        mText = text;
    }

    void Message::write(BookTypesetter::Ptr typesetter, KeywordSearchT* keywordSearch, std::map<std::string, Link*>& topicLinks) const
    {
        const MyGUI::Colour& textColour = MWBase::Environment::get().getWindowManager()->getTextColours().notify;
        BookTypesetter::Style* title = typesetter->createStyle("", textColour, false);
        typesetter->sectionBreak(9);
        typesetter->write(title, to_utf8_span(mText.c_str()));
    }

    // --------------------------------------------------------------------------------------------------

    void Choice::activated()
    {
        MWBase::Environment::get().getWindowManager()->playSound("Menu Click");
        eventChoiceActivated(mChoiceId);
    }

    void Topic::activated()
    {
        MWBase::Environment::get().getWindowManager()->playSound("Menu Click");
        eventTopicActivated(mTopicId);
    }

    void Goodbye::activated()
    {
        MWBase::Environment::get().getWindowManager()->playSound("Menu Click");
        eventActivated();
    }

    // --------------------------------------------------------------------------------------------------

    DialogueWindow::DialogueWindow()
        : WindowBase("openmw_dialogue_window.layout")
        , mIsCompanion(false)
        , mGoodbye(false)
        , mPersuasionDialog(new ResponseCallback(this))
        , mCallback(new ResponseCallback(this))
        , mGreetingCallback(new ResponseCallback(this, false))
    {
        // Centre dialog
        center();

        mPersuasionDialog.setVisible(false);

        //History view
        getWidget(mHistory, "History");

        //Topics list
        getWidget(mTopicsList, "TopicsList");
        mTopicsList->eventItemSelected += MyGUI::newDelegate(this, &DialogueWindow::onSelectListItem);

        getWidget(mGoodbyeButton, "ByeButton");
        mGoodbyeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DialogueWindow::onByeClicked);

        getWidget(mDispositionBar, "Disposition");
        getWidget(mDispositionText,"DispositionText");
        getWidget(mScrollBar, "VScroll");

        mScrollBar->eventScrollChangePosition += MyGUI::newDelegate(this, &DialogueWindow::onScrollbarMoved);
        mHistory->eventMouseWheel += MyGUI::newDelegate(this, &DialogueWindow::onMouseWheel);

        BookPage::ClickCallback callback = std::bind (&DialogueWindow::notifyLinkClicked, this, std::placeholders::_1);
        mHistory->adviseLinkClicked(callback);

        mMainWidget->castType<MyGUI::Window>()->eventWindowChangeCoord += MyGUI::newDelegate(this, &DialogueWindow::onWindowResize);
    }

    DialogueWindow::~DialogueWindow()
    {
        deleteLater();
        for (Link* link : mLinks)
            delete link;
        for (auto link : mTopicLinks)
            delete link.second;
        for (auto history : mHistoryContents)
            delete history;
    }

    void DialogueWindow::onTradeComplete()
    {
        addResponse("", MyGUI::LanguageManager::getInstance().replaceTags("#{sBarterDialog5}"));
    }

    bool DialogueWindow::exit()
    {
        if ((MWBase::Environment::get().getDialogueManager()->isInChoice()))
        {
            return false;
        }
        else
        {
            resetReference();
            MWBase::Environment::get().getDialogueManager()->goodbyeSelected();
            mTopicsList->scrollToTop();
            return true;
        }
    }

    void DialogueWindow::onWindowResize(MyGUI::Window* _sender)
    {
        // if the window has only been moved, not resized, we don't need to update
        if (mCurrentWindowSize == _sender->getSize()) return;

        mTopicsList->adjustSize();
        updateHistory();
        mCurrentWindowSize = _sender->getSize();
    }

    void DialogueWindow::onMouseWheel(MyGUI::Widget* _sender, int _rel)
    {
        if (!mScrollBar->getVisible())
            return;
        mScrollBar->setScrollPosition(std::min(static_cast<int>(mScrollBar->getScrollRange()-1),
                                               std::max(0, static_cast<int>(mScrollBar->getScrollPosition() - _rel*0.3))));
        onScrollbarMoved(mScrollBar, mScrollBar->getScrollPosition());
    }

    void DialogueWindow::onByeClicked(MyGUI::Widget* _sender)
    {
        if (exit())
        {
            MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Dialogue);
        }
    }

    void DialogueWindow::onSelectListItem(const std::string& topic, int id)
    {
        if (mGoodbye ||  MWBase::Environment::get().getDialogueManager()->isInChoice())
            return;

        const MWWorld::Store<ESM::GameSetting> &gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        const std::string sPersuasion = gmst.find("sPersuasion")->mValue.getString();
        const std::string sCompanionShare = gmst.find("sCompanionShare")->mValue.getString();
        const std::string sBarter = gmst.find("sBarter")->mValue.getString();
        const std::string sSpells = gmst.find("sSpells")->mValue.getString();
        const std::string sTravel = gmst.find("sTravel")->mValue.getString();
        const std::string sSpellMakingMenuTitle = gmst.find("sSpellMakingMenuTitle")->mValue.getString();
        const std::string sEnchanting = gmst.find("sEnchanting")->mValue.getString();
        const std::string sServiceTrainingTitle = gmst.find("sServiceTrainingTitle")->mValue.getString();
        const std::string sRepair = gmst.find("sRepair")->mValue.getString();

        if (topic != sPersuasion && topic != sCompanionShare && topic != sBarter 
         && topic != sSpells && topic != sTravel && topic != sSpellMakingMenuTitle 
         && topic != sEnchanting && topic != sServiceTrainingTitle && topic != sRepair)
        {
            onTopicActivated(topic);
            if (mGoodbyeButton->getEnabled())
                MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mGoodbyeButton);
        }
        else if (topic == sPersuasion)
            mPersuasionDialog.setVisible(true);
        else if (topic == sCompanionShare)
            MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Companion, mPtr);
        else if (!MWBase::Environment::get().getDialogueManager()->checkServiceRefused(mCallback.get()))
        {
            if (topic == sBarter)
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Barter, mPtr);
            else if (topic == sSpells)
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_SpellBuying, mPtr);
            else if (topic == sTravel)
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Travel, mPtr);
            else if (topic == sSpellMakingMenuTitle)
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_SpellCreation, mPtr);
            else if (topic == sEnchanting)
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Enchanting, mPtr);
            else if (topic == sServiceTrainingTitle)
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Training, mPtr);
            else if (topic == sRepair)
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_MerchantRepair, mPtr);
        }
        else
            updateTopics();
    }

    void DialogueWindow::setPtr(const MWWorld::Ptr& actor)
    {
        if (!actor.getClass().isActor())
        {
            Log(Debug::Warning) << "Warning: can not talk with non-actor object.";
            return;
        }

        bool sameActor = (mPtr == actor);
        if (!sameActor)
        {
            // The history is not reset here
            mKeywords.clear();
            mTopicsList->clear();
            for (Link* link : mLinks)
                mDeleteLater.push_back(link); // Links are not deleted right away to prevent issues with event handlers
            mLinks.clear();
        }

        mPtr = actor;
        mGoodbye = false;
        mTopicsList->setEnabled(true);

        if (!MWBase::Environment::get().getDialogueManager()->startDialogue(actor, mGreetingCallback.get()))
        {
            // No greetings found. The dialogue window should not be shown.
            // If this is a companion, we must show the companion window directly (used by BM_bear_be_unique).
            MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Dialogue);
            mPtr = MWWorld::Ptr();
            if (isCompanion(actor))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Companion, actor);
            return;
        }

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mGoodbyeButton);

        setTitle(mPtr.getClass().getName(mPtr));

        updateTopics();
        updateTopicsPane(); // force update for new services

        updateDisposition();
        restock();
    }

    void DialogueWindow::restock()
    {
        MWMechanics::CreatureStats &sellerStats = mPtr.getClass().getCreatureStats(mPtr);
        float delay = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fBarterGoldResetDelay")->mValue.getFloat();

        // Gold is restocked every 24h
        if (MWBase::Environment::get().getWorld()->getTimeStamp() >= sellerStats.getLastRestockTime() + delay)
        {
            sellerStats.setGoldPool(mPtr.getClass().getBaseGold(mPtr));

            sellerStats.setLastRestockTime(MWBase::Environment::get().getWorld()->getTimeStamp());
        }
    }

    void DialogueWindow::deleteLater()
    {
        for (Link* link : mDeleteLater)
            delete link;
        mDeleteLater.clear();
    }

    void DialogueWindow::onClose()
    {
        // Reset history
        for (DialogueText* text : mHistoryContents)
            delete text;
        mHistoryContents.clear();
    }

    void DialogueWindow::setKeywords(std::list<std::string> keyWords)
    {
        if (mKeywords == keyWords && isCompanion() == mIsCompanion)
            return;
        mIsCompanion = isCompanion();
        mKeywords = keyWords;

        updateTopicsPane();
    }

    void DialogueWindow::updateTopicsPane()
    {
        mTopicsList->clear();
        for (auto& linkPair : mTopicLinks)
            mDeleteLater.push_back(linkPair.second);
        mTopicLinks.clear();
        mKeywordSearch.clear();

        int services = mPtr.getClass().getServices(mPtr);

        bool travel = (mPtr.getTypeName() == typeid(ESM::NPC).name() && !mPtr.get<ESM::NPC>()->mBase->getTransport().empty())
                || (mPtr.getTypeName() == typeid(ESM::Creature).name() && !mPtr.get<ESM::Creature>()->mBase->getTransport().empty());

        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        if (mPtr.getTypeName() == typeid(ESM::NPC).name())
            mTopicsList->addItem(gmst.find("sPersuasion")->mValue.getString());

        if (services & ESM::NPC::AllItems)
            mTopicsList->addItem(gmst.find("sBarter")->mValue.getString());

        if (services & ESM::NPC::Spells)
            mTopicsList->addItem(gmst.find("sSpells")->mValue.getString());

        if (travel)
            mTopicsList->addItem(gmst.find("sTravel")->mValue.getString());

        if (services & ESM::NPC::Spellmaking)
            mTopicsList->addItem(gmst.find("sSpellmakingMenuTitle")->mValue.getString());

        if (services & ESM::NPC::Enchanting)
            mTopicsList->addItem(gmst.find("sEnchanting")->mValue.getString());

        if (services & ESM::NPC::Training)
            mTopicsList->addItem(gmst.find("sServiceTrainingTitle")->mValue.getString());

        if (services & ESM::NPC::Repair)
            mTopicsList->addItem(gmst.find("sRepair")->mValue.getString());

        if (isCompanion())
            mTopicsList->addItem(gmst.find("sCompanionShare")->mValue.getString());

        if (mTopicsList->getItemCount() > 0)
            mTopicsList->addSeparator();


        for(std::string& keyword : mKeywords)
        {
            mTopicsList->addItem(keyword);

            Topic* t = new Topic(keyword);
            t->eventTopicActivated += MyGUI::newDelegate(this, &DialogueWindow::onTopicActivated);
            mTopicLinks[Misc::StringUtils::lowerCase(keyword)] = t;

            mKeywordSearch.seed(Misc::StringUtils::lowerCase(keyword), intptr_t(t));
        }
        mTopicsList->adjustSize();

        updateHistory();
    }

    void DialogueWindow::updateHistory(bool scrollbar)
    {
        if (!scrollbar && mScrollBar->getVisible())
        {
            mHistory->setSize(mHistory->getSize()+MyGUI::IntSize(mScrollBar->getWidth(),0));
            mScrollBar->setVisible(false);
        }
        if (scrollbar && !mScrollBar->getVisible())
        {
            mHistory->setSize(mHistory->getSize()-MyGUI::IntSize(mScrollBar->getWidth(),0));
            mScrollBar->setVisible(true);
        }

        BookTypesetter::Ptr typesetter = BookTypesetter::create (mHistory->getWidth(), std::numeric_limits<int>::max());

        for (DialogueText* text : mHistoryContents)
            text->write(typesetter, &mKeywordSearch, mTopicLinks);

        BookTypesetter::Style* body = typesetter->createStyle("", MyGUI::Colour::White, false);

        typesetter->sectionBreak(9);
        // choices
        const TextColours& textColours = MWBase::Environment::get().getWindowManager()->getTextColours();
        mChoices = MWBase::Environment::get().getDialogueManager()->getChoices();
        for (std::pair<std::string, int>& choice : mChoices)
        {
            Choice* link = new Choice(choice.second);
            link->eventChoiceActivated += MyGUI::newDelegate(this, &DialogueWindow::onChoiceActivated);
            mLinks.push_back(link);

            typesetter->lineBreak();
            BookTypesetter::Style* questionStyle = typesetter->createHotStyle(body, textColours.answer, textColours.answerOver,
                                                                              textColours.answerPressed,
                                                                              TypesetBook::InteractiveId(link));
            typesetter->write(questionStyle, to_utf8_span(choice.first.c_str()));
        }

        mGoodbye = MWBase::Environment::get().getDialogueManager()->isGoodbye();
        if (mGoodbye)
        {
            Goodbye* link = new Goodbye();
            link->eventActivated += MyGUI::newDelegate(this, &DialogueWindow::onGoodbyeActivated);
            mLinks.push_back(link);
            std::string goodbye = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("sGoodbye")->mValue.getString();
            BookTypesetter::Style* questionStyle = typesetter->createHotStyle(body, textColours.answer, textColours.answerOver,
                                                                              textColours.answerPressed,
                                                                              TypesetBook::InteractiveId(link));
            typesetter->lineBreak();
            typesetter->write(questionStyle, to_utf8_span(goodbye.c_str()));
        }

        TypesetBook::Ptr book = typesetter->complete();
        mHistory->showPage(book, 0);
        size_t viewHeight = mHistory->getParent()->getHeight();
        if (!scrollbar && book->getSize().second > viewHeight)
            updateHistory(true);
        else if (scrollbar)
        {
            mHistory->setSize(MyGUI::IntSize(mHistory->getWidth(), book->getSize().second));
            size_t range = book->getSize().second - viewHeight;
            mScrollBar->setScrollRange(range);
            mScrollBar->setScrollPosition(range-1);
            mScrollBar->setTrackSize(static_cast<int>(viewHeight / static_cast<float>(book->getSize().second) * mScrollBar->getLineSize()));
            onScrollbarMoved(mScrollBar, range-1);
        }
        else
        {
            // no scrollbar
            onScrollbarMoved(mScrollBar, 0);
        }

        bool goodbyeEnabled = !MWBase::Environment::get().getDialogueManager()->isInChoice() || mGoodbye;
        bool goodbyeWasEnabled = mGoodbyeButton->getEnabled();
        mGoodbyeButton->setEnabled(goodbyeEnabled);
        if (goodbyeEnabled && !goodbyeWasEnabled)
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mGoodbyeButton);

        bool topicsEnabled = !MWBase::Environment::get().getDialogueManager()->isInChoice() && !mGoodbye;
        mTopicsList->setEnabled(topicsEnabled);
    }

    void DialogueWindow::notifyLinkClicked (TypesetBook::InteractiveId link)
    {
        reinterpret_cast<Link*>(link)->activated();
    }

    void DialogueWindow::onTopicActivated(const std::string &topicId)
    {
        if (mGoodbye)
            return;

        MWBase::Environment::get().getDialogueManager()->keywordSelected(topicId, mCallback.get());
        updateTopics();
    }

    void DialogueWindow::onChoiceActivated(int id)
    {
        if (mGoodbye)
        {
            onGoodbyeActivated();
            return;
        }
        MWBase::Environment::get().getDialogueManager()->questionAnswered(id, mCallback.get());
        updateTopics();
    }

    void DialogueWindow::onGoodbyeActivated()
    {
        MWBase::Environment::get().getDialogueManager()->goodbyeSelected();
        MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Dialogue);
        resetReference();
    }

    void DialogueWindow::onScrollbarMoved(MyGUI::ScrollBar *sender, size_t pos)
    {
        mHistory->setPosition(0, static_cast<int>(pos) * -1);
    }

    void DialogueWindow::addResponse(const std::string &title, const std::string &text, bool needMargin)
    {
        mHistoryContents.push_back(new Response(text, title, needMargin));
        updateHistory();
        updateTopics();
    }

    void DialogueWindow::addMessageBox(const std::string& text)
    {
        mHistoryContents.push_back(new Message(text));
        updateHistory();
    }

    void DialogueWindow::updateDisposition()
    {
        bool dispositionVisible = false;
        if (!mPtr.isEmpty() && mPtr.getClass().isNpc())
        {
            dispositionVisible = true;
            mDispositionBar->setProgressRange(100);
            mDispositionBar->setProgressPosition(MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(mPtr));
            mDispositionText->setCaption(MyGUI::utility::toString(MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(mPtr))+std::string("/100"));
        }

        bool dispositionWasVisible = mDispositionBar->getVisible();

        if (dispositionVisible && !dispositionWasVisible)
        {
            mDispositionBar->setVisible(true);
            int offset = mDispositionBar->getHeight()+5;
            mTopicsList->setCoord(mTopicsList->getCoord() + MyGUI::IntCoord(0,offset,0,-offset));
            mTopicsList->adjustSize();
        }
        else if (!dispositionVisible && dispositionWasVisible)
        {
            mDispositionBar->setVisible(false);
            int offset = mDispositionBar->getHeight()+5;
            mTopicsList->setCoord(mTopicsList->getCoord() - MyGUI::IntCoord(0,offset,0,-offset));
            mTopicsList->adjustSize();
        }
    }

    void DialogueWindow::onReferenceUnavailable()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Dialogue);
    }

    void DialogueWindow::onFrame(float dt)
    {
        checkReferenceAvailable();
        if (mPtr.isEmpty())
            return;

        updateDisposition();
        deleteLater();

        if (mChoices != MWBase::Environment::get().getDialogueManager()->getChoices()
                || mGoodbye != MWBase::Environment::get().getDialogueManager()->isGoodbye())
            updateHistory();
    }

    void DialogueWindow::updateTopics()
    {
        setKeywords(MWBase::Environment::get().getDialogueManager()->getAvailableTopics());
    }

    bool DialogueWindow::isCompanion()
    {
        return isCompanion(mPtr);
    }

    bool DialogueWindow::isCompanion(const MWWorld::Ptr& actor)
    {
        if (actor.isEmpty())
            return false;

        return !actor.getClass().getScript(actor).empty()
                && actor.getRefData().getLocals().getIntVar(actor.getClass().getScript(actor), "companion");
    }

}
