#include "dialogue.hpp"

#include <MyGUI_LanguageManager.h>
#include <MyGUI_Window.h>
#include <MyGUI_ProgressBar.h>
#include <MyGUI_ScrollBar.h>
#include <MyGUI_Button.h>

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

    PersuasionDialog::PersuasionDialog()
        : WindowModal("openmw_persuasion_dialog.layout")
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

        MWBase::DialogueManager::Response response = MWBase::Environment::get().getDialogueManager()->persuade(type);

        eventPersuadeMsg(response.first, response.second);

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
            BookTypesetter::Style* title = typesetter->createStyle("", headerColour);
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

            BookTypesetter::Style* style = typesetter->createStyle("", textColours.normal);
            size_t formatted = 0; // points to the first character that is not laid out yet
            for (std::map<Range, intptr_t>::iterator it = hyperLinks.begin(); it != hyperLinks.end(); ++it)
            {
                intptr_t topicId = it->second;
                BookTypesetter::Style* hotStyle = typesetter->createHotStyle (style, textColours.link,
                                                                              textColours.linkOver, textColours.linkPressed,
                                                                              topicId);
                if (formatted < it->first.first)
                    typesetter->write(style, formatted, it->first.first);
                typesetter->write(hotStyle, it->first.first, it->first.second);
                formatted = it->first.second;
            }
            if (formatted < text.size())
                typesetter->write(style, formatted, text.size());
        }
        else
        {
            std::vector<KeywordSearchT::Match> matches;
            keywordSearch->highlightKeywords(text.begin(), text.end(), matches);

            std::string::const_iterator i = text.begin ();
            for (std::vector<KeywordSearchT::Match>::iterator it = matches.begin(); it != matches.end(); ++it)
            {
                KeywordSearchT::Match match = *it;
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

        BookTypesetter::Style* style = typesetter->createStyle("", textColours.normal);


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
        BookTypesetter::Style* title = typesetter->createStyle("", textColour);
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
        , mPersuasionDialog()
    {
        // Centre dialog
        center();

        mPersuasionDialog.setVisible(false);
        mPersuasionDialog.eventPersuadeMsg += MyGUI::newDelegate(this, &DialogueWindow::onPersuadeResult);

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
        mPersuasionDialog.eventPersuadeMsg.clear();

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
            MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Dialogue);
    }

    void DialogueWindow::onSelectListItem(const std::string& topic, int id)
    {
        if (mGoodbye ||  MWBase::Environment::get().getDialogueManager()->isInChoice())
            return;

        int separatorPos = 0;
        for (unsigned int i=0; i<mTopicsList->getItemCount(); ++i)
        {
            if (mTopicsList->getItemNameAt(i) == "")
                separatorPos = i;
        }

        if (id >= separatorPos)
        {
            onTopicActivated(topic);
            if (mGoodbyeButton->getEnabled())
                MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mGoodbyeButton);
        }
        else
        {
            const MWWorld::Store<ESM::GameSetting> &gmst =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

            MWBase::DialogueManager::Response response;
            if (topic == gmst.find("sPersuasion")->getString())
                mPersuasionDialog.setVisible(true);
            else if (topic == gmst.find("sCompanionShare")->getString())
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Companion, mPtr);
            else if (!MWBase::Environment::get().getDialogueManager()->checkServiceRefused(response))
            {
                if (topic == gmst.find("sBarter")->getString())
                    MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Barter, mPtr);
                else if (topic == gmst.find("sSpells")->getString())
                    MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_SpellBuying, mPtr);
                else if (topic == gmst.find("sTravel")->getString())
                    MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Travel, mPtr);
                else if (topic == gmst.find("sSpellMakingMenuTitle")->getString())
                    MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_SpellCreation, mPtr);
                else if (topic == gmst.find("sEnchanting")->getString())
                    MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Enchanting, mPtr);
                else if (topic == gmst.find("sServiceTrainingTitle")->getString())
                    MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Training, mPtr);
                else if (topic == gmst.find("sRepair")->getString())
                    MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_MerchantRepair, mPtr);
            }
            else
                addResponse(response.first, response.second);
        }
    }

    void DialogueWindow::setPtr(const MWWorld::Ptr& actor)
    {
        MWBase::DialogueManager::Response response;
        if (!MWBase::Environment::get().getDialogueManager()->startDialogue(actor, response))
        {
            // No greetings found. The dialogue window should not be shown.
            // If this is a companion, we must show the companion window directly (used by BM_bear_be_unique).
            MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Dialogue);
            if (isCompanion(actor))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Companion, actor);
            return;
        }

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mGoodbyeButton);

        mGoodbye = false;
        bool sameActor = (mPtr == actor);
        mPtr = actor;
        mTopicsList->setEnabled(true);
        setTitle(mPtr.getClass().getName(mPtr));

        mTopicsList->clear();

        if (!sameActor)
        {
            for (std::vector<DialogueText*>::iterator it = mHistoryContents.begin(); it != mHistoryContents.end(); ++it)
                delete (*it);
            mHistoryContents.clear();

            mKeywords.clear();
            updateTopicsPane();
        }

        for (std::vector<Link*>::iterator it = mLinks.begin(); it != mLinks.end(); ++it)
            mDeleteLater.push_back(*it); // Links are not deleted right away to prevent issues with event handlers
        mLinks.clear();

        updateDisposition();
        restock();

        addResponse(response.first, response.second, false);
    }

    void DialogueWindow::restock()
    {
        MWMechanics::CreatureStats &sellerStats = mPtr.getClass().getCreatureStats(mPtr);
        float delay = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fBarterGoldResetDelay")->getFloat();

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
        for (std::map<std::string, Link*>::iterator it = mTopicLinks.begin(); it != mTopicLinks.end(); ++it)
            mDeleteLater.push_back(it->second);
        mTopicLinks.clear();
        mKeywordSearch.clear();

        int services = mPtr.getClass().getServices(mPtr);

        bool travel = (mPtr.getTypeName() == typeid(ESM::NPC).name() && !mPtr.get<ESM::NPC>()->mBase->getTransport().empty())
                || (mPtr.getTypeName() == typeid(ESM::Creature).name() && !mPtr.get<ESM::Creature>()->mBase->getTransport().empty());

        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        if (mPtr.getTypeName() == typeid(ESM::NPC).name())
            mTopicsList->addItem(gmst.find("sPersuasion")->getString());

        if (services & ESM::NPC::AllItems)
            mTopicsList->addItem(gmst.find("sBarter")->getString());

        if (services & ESM::NPC::Spells)
            mTopicsList->addItem(gmst.find("sSpells")->getString());

        if (travel)
            mTopicsList->addItem(gmst.find("sTravel")->getString());

        if (services & ESM::NPC::Spellmaking)
            mTopicsList->addItem(gmst.find("sSpellmakingMenuTitle")->getString());

        if (services & ESM::NPC::Enchanting)
            mTopicsList->addItem(gmst.find("sEnchanting")->getString());

        if (services & ESM::NPC::Training)
            mTopicsList->addItem(gmst.find("sServiceTrainingTitle")->getString());

        if (services & ESM::NPC::Repair)
            mTopicsList->addItem(gmst.find("sRepair")->getString());

        if (isCompanion())
            mTopicsList->addItem(gmst.find("sCompanionShare")->getString());

        if (mTopicsList->getItemCount() > 0)
            mTopicsList->addSeparator();


        for(std::list<std::string>::iterator it = mKeywords.begin(); it != mKeywords.end(); ++it)
        {
            mTopicsList->addItem(*it);

            Topic* t = new Topic(*it);
            t->eventTopicActivated += MyGUI::newDelegate(this, &DialogueWindow::onTopicActivated);
            mTopicLinks[Misc::StringUtils::lowerCase(*it)] = t;

            mKeywordSearch.seed(Misc::StringUtils::lowerCase(*it), intptr_t(t));
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

        for (std::vector<DialogueText*>::iterator it = mHistoryContents.begin(); it != mHistoryContents.end(); ++it)
            (*it)->write(typesetter, &mKeywordSearch, mTopicLinks);


        BookTypesetter::Style* body = typesetter->createStyle("", MyGUI::Colour::White);

        typesetter->sectionBreak(9);
        // choices
        const TextColours& textColours = MWBase::Environment::get().getWindowManager()->getTextColours();
        mChoices = MWBase::Environment::get().getDialogueManager()->getChoices();
        for (std::vector<std::pair<std::string, int> >::const_iterator it = mChoices.begin(); it != mChoices.end(); ++it)
        {
            Choice* link = new Choice(it->second);
            link->eventChoiceActivated += MyGUI::newDelegate(this, &DialogueWindow::onChoiceActivated);
            mLinks.push_back(link);

            typesetter->lineBreak();
            BookTypesetter::Style* questionStyle = typesetter->createHotStyle(body, textColours.answer, textColours.answerOver,
                                                                              textColours.answerPressed,
                                                                              TypesetBook::InteractiveId(link));
            typesetter->write(questionStyle, to_utf8_span(it->first.c_str()));
        }

        mGoodbye = MWBase::Environment::get().getDialogueManager()->isGoodbye();
        if (mGoodbye)
        {
            Goodbye* link = new Goodbye();
            link->eventActivated += MyGUI::newDelegate(this, &DialogueWindow::onGoodbyeActivated);
            mLinks.push_back(link);
            std::string goodbye = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("sGoodbye")->getString();
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
        MWBase::DialogueManager::Response response = MWBase::Environment::get().getDialogueManager()->keywordSelected(topicId);
        addResponse(response.first, response.second);
    }

    void DialogueWindow::onChoiceActivated(int id)
    {
        MWBase::DialogueManager::Response response = MWBase::Environment::get().getDialogueManager()->questionAnswered(id);
        addResponse(response.first, response.second);
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
        return !actor.getClass().getScript(actor).empty()
                && actor.getRefData().getLocals().getIntVar(actor.getClass().getScript(actor), "companion");
    }

    void DialogueWindow::onPersuadeResult(const std::string &title, const std::string &text)
    {
        addResponse(title, text);
    }
}
