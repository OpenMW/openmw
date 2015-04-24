#include "dialogue.hpp"

#include <boost/bind.hpp>

#include <MyGUI_LanguageManager.h>
#include <MyGUI_Window.h>
#include <MyGUI_ProgressBar.h>

#include <components/widgets/list.hpp>
#include <components/translation/translation.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/dialoguemanager.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "widgets.hpp"
#include "bookpage.hpp"

#include "journalbooks.hpp" // to_utf8_span

namespace
{

    MyGUI::Colour getTextColour (const std::string& type)
    {
        return MyGUI::Colour::parse(MyGUI::LanguageManager::getInstance().replaceTags("#{fontcolour=" + type + "}"));
    }

}

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
        exit();
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

        MWBase::Environment::get().getDialogueManager()->persuade(type);

        setVisible(false);
    }

    void PersuasionDialog::open()
    {
        WindowModal::open();
        center();

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        mBribe10Button->setEnabled (playerGold >= 10);
        mBribe100Button->setEnabled (playerGold >= 100);
        mBribe1000Button->setEnabled (playerGold >= 1000);

        mGoldLabel->setCaptionWithReplacing("#{sGold}: " + MyGUI::utility::toString(playerGold));
    }

    void PersuasionDialog::exit()
    {
        setVisible(false);
    }

    // --------------------------------------------------------------------------------------------------

    Response::Response(const std::string &text, const std::string &title)
        : mTitle(title)
    {
        mText = text;
    }

    void Response::write(BookTypesetter::Ptr typesetter, KeywordSearchT* keywordSearch, std::map<std::string, Link*>& topicLinks) const
    {
        BookTypesetter::Style* title = typesetter->createStyle("", getTextColour("header"));
        typesetter->sectionBreak(9);
        if (mTitle != "")
            typesetter->write(title, to_utf8_span(mTitle.c_str()));
        typesetter->sectionBreak(9);

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
            BookTypesetter::Style* style = typesetter->createStyle("", getTextColour("normal"));
            size_t formatted = 0; // points to the first character that is not laid out yet
            for (std::map<Range, intptr_t>::iterator it = hyperLinks.begin(); it != hyperLinks.end(); ++it)
            {
                intptr_t topicId = it->second;
                const MyGUI::Colour linkHot    (getTextColour("link_over"));
                const MyGUI::Colour linkNormal (getTextColour("link"));
                const MyGUI::Colour linkActive (getTextColour("link_pressed"));
                BookTypesetter::Style* hotStyle = typesetter->createHotStyle (style, linkNormal, linkHot, linkActive, topicId);
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
        BookTypesetter::Style* style = typesetter->createStyle("", getTextColour("normal"));

        const MyGUI::Colour linkHot    (getTextColour("link_over"));
        const MyGUI::Colour linkNormal (getTextColour("link"));
        const MyGUI::Colour linkActive (getTextColour("link_pressed"));

        if (topicId)
            style = typesetter->createHotStyle (style, linkNormal, linkHot, linkActive, topicId);
        typesetter->write (style, begin, end);
    }

    Message::Message(const std::string& text)
    {
        mText = text;
    }

    void Message::write(BookTypesetter::Ptr typesetter, KeywordSearchT* keywordSearch, std::map<std::string, Link*>& topicLinks) const
    {
        BookTypesetter::Style* title = typesetter->createStyle("", getTextColour("notify"));
        typesetter->sectionBreak(9);
        typesetter->write(title, to_utf8_span(mText.c_str()));
    }

    // --------------------------------------------------------------------------------------------------

    void Choice::activated()
    {

        MWBase::Environment::get().getSoundManager()->playSound("Menu Click", 1.0, 1.0);
        MWBase::Environment::get().getDialogueManager()->questionAnswered(mChoiceId);
    }

    void Topic::activated()
    {

        MWBase::Environment::get().getSoundManager()->playSound("Menu Click", 1.f, 1.f);
        MWBase::Environment::get().getDialogueManager()->keywordSelected(Misc::StringUtils::lowerCase(mTopicId));
    }

    void Goodbye::activated()
    {

        MWBase::Environment::get().getSoundManager()->playSound("Menu Click", 1.f, 1.f);
        MWBase::Environment::get().getDialogueManager()->goodbyeSelected();
    }

    // --------------------------------------------------------------------------------------------------

    DialogueWindow::DialogueWindow()
        : WindowBase("openmw_dialogue_window.layout")
        , mPersuasionDialog()
        , mEnabled(false)
        , mServices(0)
        , mGoodbye(false)
    {
        // Centre dialog
        center();

        mPersuasionDialog.setVisible(false);

        //History view
        getWidget(mHistory, "History");

        //Topics list
        getWidget(mTopicsList, "TopicsList");
        mTopicsList->eventItemSelected += MyGUI::newDelegate(this, &DialogueWindow::onSelectTopic);

        MyGUI::Button* byeButton;
        getWidget(byeButton, "ByeButton");
        byeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DialogueWindow::onByeClicked);

        getWidget(mDispositionBar, "Disposition");
        getWidget(mDispositionText,"DispositionText");
        getWidget(mScrollBar, "VScroll");

        mScrollBar->eventScrollChangePosition += MyGUI::newDelegate(this, &DialogueWindow::onScrollbarMoved);
        mHistory->eventMouseWheel += MyGUI::newDelegate(this, &DialogueWindow::onMouseWheel);

        BookPage::ClickCallback callback = boost::bind (&DialogueWindow::notifyLinkClicked, this, _1);
        mHistory->adviseLinkClicked(callback);

        mMainWidget->castType<MyGUI::Window>()->eventWindowChangeCoord += MyGUI::newDelegate(this, &DialogueWindow::onWindowResize);
    }

    void DialogueWindow::exit()
    {
        if ((!mEnabled || MWBase::Environment::get().getDialogueManager()->isInChoice())
                && !mGoodbye)
        {
            // in choice, not allowed to escape, but give access to main menu to allow loading other saves
            MWBase::Environment::get().getWindowManager()->pushGuiMode (MWGui::GM_MainMenu);
        }
        else
            MWBase::Environment::get().getDialogueManager()->goodbyeSelected();
    }

    void DialogueWindow::onWindowResize(MyGUI::Window* _sender)
    {
        mTopicsList->adjustSize();
        updateHistory();
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
        exit();
    }

    void DialogueWindow::onSelectTopic(const std::string& topic, int id)
    {
        if (!mEnabled || MWBase::Environment::get().getDialogueManager()->isInChoice())
            return;

        int separatorPos = 0;
        for (unsigned int i=0; i<mTopicsList->getItemCount(); ++i)
        {
            if (mTopicsList->getItemNameAt(i) == "")
                separatorPos = i;
        }

        if (id >= separatorPos)
            MWBase::Environment::get().getDialogueManager()->keywordSelected(Misc::StringUtils::lowerCase(topic));
        else
        {
            const MWWorld::Store<ESM::GameSetting> &gmst =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

            if (topic == gmst.find("sPersuasion")->getString())
                mPersuasionDialog.setVisible(true);
            else if (topic == gmst.find("sCompanionShare")->getString())
                MWBase::Environment::get().getWindowManager()->showCompanionWindow(mPtr);
            else if (!MWBase::Environment::get().getDialogueManager()->checkServiceRefused())
            {
                if (topic == gmst.find("sBarter")->getString())
                    MWBase::Environment::get().getWindowManager()->startTrade(mPtr);
                else if (topic == gmst.find("sSpells")->getString())
                    MWBase::Environment::get().getWindowManager()->startSpellBuying(mPtr);
                else if (topic == gmst.find("sTravel")->getString())
                    MWBase::Environment::get().getWindowManager()->startTravel(mPtr);
                else if (topic == gmst.find("sSpellMakingMenuTitle")->getString())
                    MWBase::Environment::get().getWindowManager()->startSpellMaking (mPtr);
                else if (topic == gmst.find("sEnchanting")->getString())
                    MWBase::Environment::get().getWindowManager()->startEnchanting (mPtr);
                else if (topic == gmst.find("sServiceTrainingTitle")->getString())
                    MWBase::Environment::get().getWindowManager()->startTraining (mPtr);
                else if (topic == gmst.find("sRepair")->getString())
                    MWBase::Environment::get().getWindowManager()->startRepair (mPtr);
            }
        }
    }

    void DialogueWindow::startDialogue(MWWorld::Ptr actor, std::string npcName, bool resetHistory)
    {
        mGoodbye = false;
        mEnabled = true;
        bool sameActor = (mPtr == actor);
        mPtr = actor;
        mTopicsList->setEnabled(true);
        setTitle(npcName);

        clearChoices();

        mTopicsList->clear();

        if (resetHistory || !sameActor)
        {
            for (std::vector<DialogueText*>::iterator it = mHistoryContents.begin(); it != mHistoryContents.end(); ++it)
                delete (*it);
            mHistoryContents.clear();
        }

        for (std::vector<Link*>::iterator it = mLinks.begin(); it != mLinks.end(); ++it)
            delete (*it);
        mLinks.clear();

        updateOptions();

        restock();
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

    void DialogueWindow::setKeywords(std::list<std::string> keyWords)
    {
        mTopicsList->clear();
        for (std::map<std::string, Link*>::iterator it = mTopicLinks.begin(); it != mTopicLinks.end(); ++it)
            delete it->second;
        mTopicLinks.clear();
        mKeywordSearch.clear();

        bool isCompanion = !mPtr.getClass().getScript(mPtr).empty()
                && mPtr.getRefData().getLocals().getIntVar(mPtr.getClass().getScript(mPtr), "companion");

        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        if (mPtr.getTypeName() == typeid(ESM::NPC).name())
            mTopicsList->addItem(gmst.find("sPersuasion")->getString());

        if (mServices & Service_Trade)
            mTopicsList->addItem(gmst.find("sBarter")->getString());

        if (mServices & Service_BuySpells)
            mTopicsList->addItem(gmst.find("sSpells")->getString());

        if (mServices & Service_Travel)
            mTopicsList->addItem(gmst.find("sTravel")->getString());

        if (mServices & Service_CreateSpells)
            mTopicsList->addItem(gmst.find("sSpellmakingMenuTitle")->getString());

        if (mServices & Service_Enchant)
            mTopicsList->addItem(gmst.find("sEnchanting")->getString());

        if (mServices & Service_Training)
            mTopicsList->addItem(gmst.find("sServiceTrainingTitle")->getString());

        if (mServices & Service_Repair)
            mTopicsList->addItem(gmst.find("sRepair")->getString());

        if (isCompanion)
            mTopicsList->addItem(gmst.find("sCompanionShare")->getString());

        if (mTopicsList->getItemCount() > 0)
            mTopicsList->addSeparator();


        for(std::list<std::string>::iterator it = keyWords.begin(); it != keyWords.end(); ++it)
        {
            mTopicsList->addItem(*it);

            Topic* t = new Topic(*it);
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
        const MyGUI::Colour linkHot    (getTextColour("answer_over"));
        const MyGUI::Colour linkNormal (getTextColour("answer"));
        const MyGUI::Colour linkActive (getTextColour("answer_pressed"));
        for (std::vector<std::pair<std::string, int> >::iterator it = mChoices.begin(); it != mChoices.end(); ++it)
        {
            Choice* link = new Choice(it->second);
            mLinks.push_back(link);

            typesetter->lineBreak();
            BookTypesetter::Style* questionStyle = typesetter->createHotStyle(body, linkNormal, linkHot, linkActive,
                                                                              TypesetBook::InteractiveId(link));
            typesetter->write(questionStyle, to_utf8_span(it->first.c_str()));
        }

        if (mGoodbye)
        {
            Goodbye* link = new Goodbye();
            mLinks.push_back(link);
            std::string goodbye = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("sGoodbye")->getString();
            BookTypesetter::Style* questionStyle = typesetter->createHotStyle(body, linkNormal, linkHot, linkActive,
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

        MyGUI::Button* byeButton;
        getWidget(byeButton, "ByeButton");
        bool goodbyeEnabled = !MWBase::Environment::get().getDialogueManager()->isInChoice() || mGoodbye;
        byeButton->setEnabled(goodbyeEnabled);

        bool topicsEnabled = !MWBase::Environment::get().getDialogueManager()->isInChoice() && !mGoodbye;
        mTopicsList->setEnabled(topicsEnabled);
    }

    void DialogueWindow::notifyLinkClicked (TypesetBook::InteractiveId link)
    {
        reinterpret_cast<Link*>(link)->activated();
    }

    void DialogueWindow::onScrollbarMoved(MyGUI::ScrollBar *sender, size_t pos)
    {
        mHistory->setPosition(0, pos * -1);
    }

    void DialogueWindow::addResponse(const std::string &text, const std::string &title)
    {
        // This is called from the dialogue manager, so text is
        // case-smashed - thus we have to retrieve the correct case
        // of the title through the topic list.
        std::string realTitle = title;
        if (realTitle != "")
        {
            for (size_t i=0; i<mTopicsList->getItemCount(); ++i)
            {
                std::string item = mTopicsList->getItemNameAt(i);
                if (Misc::StringUtils::ciEqual(item, title))
                {
                    realTitle = item;
                    break;
                }
            }
        }

        mHistoryContents.push_back(new Response(text, realTitle));
        updateHistory();
    }

    void DialogueWindow::addMessageBox(const std::string& text)
    {
        mHistoryContents.push_back(new Message(text));
        updateHistory();
    }

    void DialogueWindow::addChoice(const std::string& choice, int id)
    {
        mChoices.push_back(std::make_pair(choice, id));
        updateHistory();
    }

    void DialogueWindow::clearChoices()
    {
        mChoices.clear();
        updateHistory();
    }

    void DialogueWindow::updateOptions()
    {
        //Clear the list of topics
        mTopicsList->clear();

        bool dispositionVisible = false;
        if (mPtr.getClass().isNpc())
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

    void DialogueWindow::goodbye()
    {
        mGoodbye = true;
        mEnabled = false;
        updateHistory();
    }

    void DialogueWindow::onReferenceUnavailable()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Dialogue);
    }

    void DialogueWindow::onFrame()
    {
        if(mMainWidget->getVisible() && mEnabled && mPtr.getTypeName() == typeid(ESM::NPC).name())
        {
            int disp = std::max(0, std::min(100,
                MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(mPtr)
                    + MWBase::Environment::get().getDialogueManager()->getTemporaryDispositionChange()));
            mDispositionBar->setProgressRange(100);
            mDispositionBar->setProgressPosition(disp);
            mDispositionText->setCaption(MyGUI::utility::toString(disp)+std::string("/100"));
        }
    }
}
