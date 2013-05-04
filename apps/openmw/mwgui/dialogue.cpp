#include "dialogue.hpp"

#include <boost/lexical_cast.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "../mwdialogue/dialoguemanagerimp.hpp"

#include "dialoguehistory.hpp"
#include "widgets.hpp"
#include "list.hpp"
#include "tradewindow.hpp"
#include "spellbuyingwindow.hpp"
#include "inventorywindow.hpp"
#include "travelwindow.hpp"
#include "bookpage.hpp"


namespace
{
    MWGui::BookTypesetter::Utf8Span to_utf8_span (char const * text)
    {
        typedef MWGui::BookTypesetter::Utf8Point point;

        point begin = reinterpret_cast <point> (text);

        return MWGui::BookTypesetter::Utf8Span (begin, begin + strlen (text));
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
        setVisible(false);
    }

    void PersuasionDialog::onPersuade(MyGUI::Widget *sender)
    {
        MWBase::MechanicsManager::PersuasionType type;
        if (sender == mAdmireButton) type = MWBase::MechanicsManager::PT_Admire;
        else if (sender == mIntimidateButton) type = MWBase::MechanicsManager::PT_Intimidate;
        else if (sender == mTauntButton) type = MWBase::MechanicsManager::PT_Taunt;
        else if (sender == mBribe10Button)
        {
            MWBase::Environment::get().getWindowManager()->getTradeWindow()->addOrRemoveGold(-10);
            type = MWBase::MechanicsManager::PT_Bribe10;
        }
        else if (sender == mBribe100Button)
        {
            MWBase::Environment::get().getWindowManager()->getTradeWindow()->addOrRemoveGold(-100);
            type = MWBase::MechanicsManager::PT_Bribe100;
        }
        else /*if (sender == mBribe1000Button)*/
        {
            MWBase::Environment::get().getWindowManager()->getTradeWindow()->addOrRemoveGold(-1000);
            type = MWBase::MechanicsManager::PT_Bribe1000;
        }

        MWBase::Environment::get().getDialogueManager()->persuade(type);

        setVisible(false);
    }

    void PersuasionDialog::open()
    {
        WindowModal::open();
        center();

        int playerGold = MWBase::Environment::get().getWindowManager()->getInventoryWindow()->getPlayerGold();

        mBribe10Button->setEnabled (playerGold >= 10);
        mBribe100Button->setEnabled (playerGold >= 100);
        mBribe1000Button->setEnabled (playerGold >= 1000);

        mGoldLabel->setCaptionWithReplacing("#{sGold}: " + boost::lexical_cast<std::string>(playerGold));
    }

    // --------------------------------------------------------------------------------------------------

    Response::Response(const std::string &text, const std::string &title)
        : mTitle(title)
    {
        mText = text;
    }

    void Response::write(BookTypesetter::Ptr typesetter, KeywordSearchT* keywordSearch, std::map<std::string, Link*>& topicLinks)
    {
        BookTypesetter::Style* title = typesetter->createStyle("EB Garamond", MyGUI::Colour(223/255.f, 201/255.f, 159/255.f));
        typesetter->sectionBreak(9);
        if (mTitle != "")
            typesetter->write(title, to_utf8_span(mTitle.c_str()));
        typesetter->sectionBreak(9);

        typedef std::pair<size_t, size_t> Range;
        std::map<Range, intptr_t> hyperLinks;

        size_t pos_begin, pos_end;
        for(;;)
        {
            pos_begin = mText.find('@');
            if (pos_begin != std::string::npos)
                pos_end = mText.find('#', pos_begin);

            if (pos_begin != std::string::npos && pos_end != std::string::npos)
            {
                std::string link = mText.substr(pos_begin + 1, pos_end - pos_begin - 1);
                const char specialPseudoAsteriskCharacter = 127;
                std::replace(link.begin(), link.end(), specialPseudoAsteriskCharacter, '*');
                std::string topicName = MWBase::Environment::get().getWindowManager()->
                        getTranslationDataStorage().topicStandardForm(link);

                std::string displayName = link;
                MWDialogue::RemovePseudoAsterisks(displayName);

                mText.replace(pos_begin, pos_end+1, displayName);

                assert(topicLinks.find(topicName) != topicLinks.end());
                hyperLinks[std::make_pair(pos_begin, pos_begin+displayName.size())] = intptr_t(topicLinks[topicName]);
            }
            else
                break;
        }

        typesetter->addContent(to_utf8_span(mText.c_str()));

        for (std::map<Range, intptr_t>::iterator it = hyperLinks.begin(); it != hyperLinks.end(); ++it)
        {
            intptr_t topicId = it->second;
            BookTypesetter::Style* style = typesetter->createStyle("EB Garamond", MyGUI::Colour::Green);
            const MyGUI::Colour linkHot    (143/255.f, 155/255.f, 218/255.f);
            const MyGUI::Colour linkNormal (112/255.f, 126/255.f, 207/255.f);
            const MyGUI::Colour linkActive (175/255.f, 184/255.f, 228/255.f);
            style = typesetter->createHotStyle (style, linkNormal, linkHot, linkActive, topicId);
            typesetter->write(style, it->first.first, it->first.second);
        }

        std::string::const_iterator i = mText.begin ();
        KeywordSearchT::Match match;
        while (i != mText.end () && keywordSearch->search (i, mText.end (), match))
        {
            if (i != match.mBeg)
                addTopicLink (typesetter, 0, i - mText.begin (), match.mBeg - mText.begin ());

            addTopicLink (typesetter, match.mValue, match.mBeg - mText.begin (), match.mEnd - mText.begin ());

            i = match.mEnd;
        }

        if (i != mText.end ())
            addTopicLink (typesetter, 0, i - mText.begin (), mText.size ());
    }

    void Response::addTopicLink(BookTypesetter::Ptr typesetter, intptr_t topicId, size_t begin, size_t end)
    {
        BookTypesetter::Style* style = typesetter->createStyle("EB Garamond", MyGUI::Colour(202/255.f, 165/255.f, 96/255.f));

        const MyGUI::Colour linkHot    (143/255.f, 155/255.f, 218/255.f);
        const MyGUI::Colour linkNormal (112/255.f, 126/255.f, 207/255.f);
        const MyGUI::Colour linkActive (175/255.f, 184/255.f, 228/255.f);

        if (topicId)
            style = typesetter->createHotStyle (style, linkNormal, linkHot, linkActive, topicId);
        typesetter->write (style, begin, end);
    }

    Message::Message(const std::string& text)
    {
        mText = text;
    }

    void Message::write(BookTypesetter::Ptr typesetter, KeywordSearchT* keywordSearch, std::map<std::string, Link*>& topicLinks)
    {
        BookTypesetter::Style* title = typesetter->createStyle("EB Garamond", MyGUI::Colour(223/255.f, 201/255.f, 159/255.f));
        typesetter->sectionBreak(9);
        typesetter->write(title, to_utf8_span(mText.c_str()));
    }

    // --------------------------------------------------------------------------------------------------

    void Choice::activated()
    {
        MWBase::Environment::get().getDialogueManager()->questionAnswered(mChoiceId);
    }

    void Topic::activated()
    {
        MWBase::Environment::get().getDialogueManager()->keywordSelected(Misc::StringUtils::lowerCase(mTopicId));
    }

    void Goodbye::activated()
    {
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

        static_cast<MyGUI::Window*>(mMainWidget)->eventWindowChangeCoord += MyGUI::newDelegate(this, &DialogueWindow::onWindowResize);
    }

    void DialogueWindow::onHistoryClicked(MyGUI::Widget* _sender)
    {
        /*
        MyGUI::ISubWidgetText* t = mHistory->getClient()->getSubWidgetText();
        if(t == NULL)
            return;

        const MyGUI::IntPoint& lastPressed = MyGUI::InputManager::getInstance().getLastPressedPosition(MyGUI::MouseButton::Left);

        size_t cursorPosition = t->getCursorPosition(lastPressed);
        MyGUI::UString color = mHistory->getColorAtPos(cursorPosition);

        if (!mEnabled && color == "#572D21")
            MWBase::Environment::get().getDialogueManager()->goodbyeSelected();

        if (!mEnabled)
            return;

        if(color != "#B29154")
        {
            MyGUI::UString key = mHistory->getColorTextAt(cursorPosition);

            if(color == "#686EBA")
            {
                std::map<size_t, HyperLink>::iterator i = mHyperLinks.upper_bound(cursorPosition);
                if( !mHyperLinks.empty() )
                {
                    --i;

                    if( i->first + i->second.mLength > cursorPosition)
                    {
                        MWBase::Environment::get().getDialogueManager()->keywordSelected(i->second.mTrueValue);
                    }
                }
                else
                {
                    // the link was colored, but it is not in  mHyperLinks.
                    // It means that those liunks are not marked with @# and found
                    // by topic name search
                    MWBase::Environment::get().getDialogueManager()->keywordSelected(lower_string(key));
                }
            }

            if(color == "#572D21")
                MWBase::Environment::get().getDialogueManager()->questionAnswered(lower_string(key));
        }
        */
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
        MWBase::Environment::get().getDialogueManager()->goodbyeSelected();
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
            {
                mPersuasionDialog.setVisible(true);
            }
            else if (topic == gmst.find("sCompanionShare")->getString())
            {
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Companion);
                MWBase::Environment::get().getWindowManager()->showCompanionWindow(mPtr);
            }
            else if (!MWBase::Environment::get().getDialogueManager()->checkServiceRefused())
            {
                if (topic == gmst.find("sBarter")->getString())
                {
                    MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Barter);
                    MWBase::Environment::get().getWindowManager()->getTradeWindow()->startTrade(mPtr);
                }
                else if (topic == gmst.find("sSpells")->getString())
                {
                    MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_SpellBuying);
                    MWBase::Environment::get().getWindowManager()->getSpellBuyingWindow()->startSpellBuying(mPtr);
                }
                else if (topic == gmst.find("sTravel")->getString())
                {
                    MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Travel);
                    MWBase::Environment::get().getWindowManager()->getTravelWindow()->startTravel(mPtr);
                }
                else if (topic == gmst.find("sSpellMakingMenuTitle")->getString())
                {
                    MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_SpellCreation);
                    MWBase::Environment::get().getWindowManager()->startSpellMaking (mPtr);
                }
                else if (topic == gmst.find("sEnchanting")->getString())
                {
                    MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Enchanting);
                    MWBase::Environment::get().getWindowManager()->startEnchanting (mPtr);
                }
                else if (topic == gmst.find("sServiceTrainingTitle")->getString())
                {
                    MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Training);
                    MWBase::Environment::get().getWindowManager()->startTraining (mPtr);
                }
                else if (topic == gmst.find("sRepair")->getString())
                {
                    MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_MerchantRepair);
                    MWBase::Environment::get().getWindowManager()->startRepair (mPtr);
                }
            }
        }
    }

    void DialogueWindow::startDialogue(MWWorld::Ptr actor, std::string npcName)
    {
        mGoodbye = false;
        mEnabled = true;
        mPtr = actor;
        mTopicsList->setEnabled(true);
        setTitle(npcName);

        mTopicsList->clear();
        mHyperLinks.clear();

        for (std::vector<DialogueText*>::iterator it = mHistoryContents.begin(); it != mHistoryContents.end(); ++it)
            delete (*it);
        mHistoryContents.clear();

        for (std::vector<Link*>::iterator it = mLinks.begin(); it != mLinks.end(); ++it)
            delete (*it);
        mLinks.clear();

        updateOptions();
    }

    void DialogueWindow::setKeywords(std::list<std::string> keyWords)
    {
        mTopicsList->clear();
        for (std::map<std::string, Link*>::iterator it = mTopicLinks.begin(); it != mTopicLinks.end(); ++it)
            delete it->second;
        mTopicLinks.clear();
        mKeywordSearch.clear();

        bool isCompanion = !MWWorld::Class::get(mPtr).getScript(mPtr).empty()
                && mPtr.getRefData().getLocals().getIntVar(MWWorld::Class::get(mPtr).getScript(mPtr), "companion");

        bool anyService = mServices > 0 || isCompanion || mPtr.getTypeName() == typeid(ESM::NPC).name();

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

        if (anyService)
            mTopicsList->addSeparator();


        for(std::list<std::string>::iterator it = keyWords.begin(); it != keyWords.end(); ++it)
        {
            mTopicsList->addItem(*it);

            Topic* t = new Topic(*it);
            mTopicLinks[*it] = t;

            mKeywordSearch.seed(Misc::StringUtils::lowerCase(*it), intptr_t(t));
        }
        mTopicsList->adjustSize();

        updateHistory();
    }

    std::string DialogueWindow::parseText(const std::string& text)
    {
        bool separatorReached = false; // only parse topics that are below the separator (this prevents actions like "Barter" that are not topics from getting blue-colored)

        std::vector<std::string> topics;

        bool hasSeparator = false;
        for (unsigned int i=0; i<mTopicsList->getItemCount(); ++i)
        {
            if (mTopicsList->getItemNameAt(i) == "")
                hasSeparator = true;
        }

        for(unsigned int i = 0;i<mTopicsList->getItemCount();i++)
        {
            std::string keyWord = mTopicsList->getItemNameAt(i);
            if (separatorReached || !hasSeparator)
                topics.push_back(keyWord);
            else if (keyWord == "")
                separatorReached = true;
        }

        std::vector<MWDialogue::HyperTextToken> hypertext = MWDialogue::ParseHyperText(text);

        /*
        size_t historySize = 0;
        if(mHistory->getClient()->getSubWidgetText() != NULL)
        {
            historySize = mHistory->getOnlyText().size();
        }
        */

        std::string result;

        /*
        size_t hypertextPos = 0;
        for (size_t i = 0; i < hypertext.size(); ++i)
        {
            if (hypertext[i].mLink)
            {
                size_t asterisk_count = MWDialogue::RemovePseudoAsterisks(hypertext[i].mText);
                std::string standardForm = hypertext[i].mText;
                for(; asterisk_count > 0; --asterisk_count)
                    standardForm.append("*");

                standardForm =
                    MWBase::Environment::get().getWindowManager()->
                    getTranslationDataStorage().topicStandardForm(standardForm);

                if( std::find(topics.begin(), topics.end(), std::string(standardForm) ) != topics.end() )
                {
                    result.append("#686EBA").append(hypertext[i].mText).append("#B29154");

                    mHyperLinks[historySize+hypertextPos].mLength = MyGUI::UString(hypertext[i].mText).length();
                    mHyperLinks[historySize+hypertextPos].mTrueValue = lower_string(standardForm);
                }
                else
                    result += hypertext[i].mText;
            }
            else
            {
                if( !MWBase::Environment::get().getWindowManager()->getTranslationDataStorage().hasTranslation() )
                {
                    for(std::vector<std::string>::const_iterator it = topics.begin(); it != topics.end(); ++it)
                    {
                        addColorInString(hypertext[i].mText, *it, "#686EBA", "#B29154");
                    }
                }

                result += hypertext[i].mText;
            }

            hypertextPos += MyGUI::UString(hypertext[i].mText).length();
        }
        */
        return result;
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

        BookTypesetter::Ptr typesetter = BookTypesetter::create (mHistory->getWidth(), std::numeric_limits<int>().max());

        for (std::vector<DialogueText*>::iterator it = mHistoryContents.begin(); it != mHistoryContents.end(); ++it)
            (*it)->write(typesetter, &mKeywordSearch, mTopicLinks);


        BookTypesetter::Style* body = typesetter->createStyle("EB Garamond", MyGUI::Colour::White);

        // choices
        const MyGUI::Colour linkHot    (223/255.f, 201/255.f, 159/255.f);
        const MyGUI::Colour linkNormal (150/255.f, 50/255.f, 30/255.f);
        const MyGUI::Colour linkActive (243/255.f, 237/255.f, 221/255.f);
        for (std::map<std::string, int>::iterator it = mChoices.begin(); it != mChoices.end(); ++it)
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
            std::string goodbye = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("sGoodbye")->getString();
            BookTypesetter::Style* questionStyle = typesetter->createHotStyle(body, linkNormal, linkHot, linkActive,
                                                                              TypesetBook::InteractiveId(mLinks.back()));
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
            mScrollBar->setTrackSize(viewHeight / static_cast<float>(book->getSize().second) * mScrollBar->getLineSize());
            onScrollbarMoved(mScrollBar, range-1);
        }
        else
        {
            // no scrollbar
            onScrollbarMoved(mScrollBar, 0);
        }
    }

    void DialogueWindow::notifyLinkClicked (TypesetBook::InteractiveId link)
    {
        reinterpret_cast<Link*>(link)->activated();
    }

    void DialogueWindow::onScrollbarMoved(MyGUI::ScrollBar *sender, size_t pos)
    {
        mHistory->setPosition(0,-pos);
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
                if (Misc::StringUtils::lowerCase(item) == title)
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
        mChoices[choice] = id;
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
        mHyperLinks.clear();

        if (mPtr.getTypeName() == typeid(ESM::NPC).name())
        {
            mDispositionBar->setProgressRange(100);
            mDispositionBar->setProgressPosition(MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(mPtr));
            mDispositionText->eraseText(0, mDispositionText->getTextLength());
            mDispositionText->addText("#B29154"+boost::lexical_cast<std::string>(MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(mPtr))+std::string("/100")+"#B29154");
        }
    }

    void DialogueWindow::goodbye()
    {
        mLinks.push_back(new Goodbye());
        mGoodbye = true;
        mTopicsList->setEnabled(false);
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
            mDispositionText->eraseText(0, mDispositionText->getTextLength());
            mDispositionText->addText("#B29154"+boost::lexical_cast<std::string>(disp)+std::string("/100")+"#B29154");
        }
    }
}
