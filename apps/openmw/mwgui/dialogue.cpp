#include "dialogue.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_LanguageManager.h>
#include <MyGUI_ProgressBar.h>
#include <MyGUI_ScrollBar.h>
#include <MyGUI_UString.h>
#include <MyGUI_Window.h>

#include <components/debug/debuglog.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/settings/values.hpp>
#include <components/translation/translation.hpp>
#include <components/widgets/box.hpp>
#include <components/widgets/list.hpp>

#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/player.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "bookpage.hpp"
#include "textcolours.hpp"

#include "journalbooks.hpp" // to_utf8_span

namespace MWGui
{
    void ResponseCallback::addResponse(std::string_view title, std::string_view text)
    {
        mWindow->addResponse(title, text, mNeedMargin);
    }

    void ResponseCallback::updateTopics() const
    {
        mWindow->updateTopics();
    }

    PersuasionDialog::PersuasionDialog(std::unique_ptr<ResponseCallback> callback)
        : WindowModal("openmw_persuasion_dialog.layout")
        , mCallback(std::move(callback))
        , mInitialGoldLabelWidth(0)
        , mInitialMainWidgetWidth(0)
    {
        getWidget(mCancelButton, "CancelButton");
        getWidget(mAdmireButton, "AdmireButton");
        getWidget(mIntimidateButton, "IntimidateButton");
        getWidget(mTauntButton, "TauntButton");
        getWidget(mBribe10Button, "Bribe10Button");
        getWidget(mBribe100Button, "Bribe100Button");
        getWidget(mBribe1000Button, "Bribe1000Button");
        getWidget(mGoldLabel, "GoldLabel");
        getWidget(mActionsBox, "ActionsBox");

        int totalHeight = 3;
        adjustAction(mAdmireButton, totalHeight);
        adjustAction(mIntimidateButton, totalHeight);
        adjustAction(mTauntButton, totalHeight);
        adjustAction(mBribe10Button, totalHeight);
        adjustAction(mBribe100Button, totalHeight);
        adjustAction(mBribe1000Button, totalHeight);
        totalHeight += 3;

        int diff = totalHeight - mActionsBox->getSize().height;
        if (diff > 0)
        {
            auto mainWidgetSize = mMainWidget->getSize();
            mMainWidget->setSize(mainWidgetSize.width, mainWidgetSize.height + diff);
        }

        mInitialGoldLabelWidth = mActionsBox->getSize().width - mCancelButton->getSize().width - 8;
        mInitialMainWidgetWidth = mMainWidget->getSize().width;

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PersuasionDialog::onCancel);
        mAdmireButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PersuasionDialog::onPersuade);
        mIntimidateButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PersuasionDialog::onPersuade);
        mTauntButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PersuasionDialog::onPersuade);
        mBribe10Button->eventMouseButtonClick += MyGUI::newDelegate(this, &PersuasionDialog::onPersuade);
        mBribe100Button->eventMouseButtonClick += MyGUI::newDelegate(this, &PersuasionDialog::onPersuade);
        mBribe1000Button->eventMouseButtonClick += MyGUI::newDelegate(this, &PersuasionDialog::onPersuade);
    }

    void PersuasionDialog::adjustAction(MyGUI::Widget* action, int& totalHeight)
    {
        const int lineHeight = Settings::gui().mFontSize + 2;
        auto currentCoords = action->getCoord();
        action->setCoord(currentCoords.left, totalHeight, currentCoords.width, lineHeight);
        totalHeight += lineHeight;
    }

    void PersuasionDialog::onCancel(MyGUI::Widget* sender)
    {
        setVisible(false);
    }

    void PersuasionDialog::onPersuade(MyGUI::Widget* sender)
    {
        MWBase::MechanicsManager::PersuasionType type;
        if (sender == mAdmireButton)
            type = MWBase::MechanicsManager::PT_Admire;
        else if (sender == mIntimidateButton)
            type = MWBase::MechanicsManager::PT_Intimidate;
        else if (sender == mTauntButton)
            type = MWBase::MechanicsManager::PT_Taunt;
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

        mBribe10Button->setEnabled(playerGold >= 10);
        mBribe100Button->setEnabled(playerGold >= 100);
        mBribe1000Button->setEnabled(playerGold >= 1000);

        mGoldLabel->setCaptionWithReplacing("#{sGold}: " + MyGUI::utility::toString(playerGold));

        int diff = mGoldLabel->getRequestedSize().width - mInitialGoldLabelWidth;
        if (diff > 0)
            mMainWidget->setSize(mInitialMainWidgetWidth + diff, mMainWidget->getSize().height);
        else
            mMainWidget->setSize(mInitialMainWidgetWidth, mMainWidget->getSize().height);

        WindowModal::onOpen();
    }

    MyGUI::Widget* PersuasionDialog::getDefaultKeyFocus()
    {
        return mAdmireButton;
    }

    // --------------------------------------------------------------------------------------------------

    Response::Response(std::string_view text, std::string_view title, bool needMargin)
        : mTitle(title)
        , mNeedMargin(needMargin)
    {
        mText = text;
    }

    void Response::write(BookTypesetter::Ptr typesetter, KeywordSearchT* keywordSearch,
        std::map<std::string, std::unique_ptr<Link>>& topicLinks) const
    {
        typesetter->sectionBreak(mNeedMargin ? 9 : 0);
        auto windowManager = MWBase::Environment::get().getWindowManager();

        if (!mTitle.empty())
        {
            const MyGUI::Colour& headerColour = windowManager->getTextColours().header;
            BookTypesetter::Style* title = typesetter->createStyle({}, headerColour, false);
            typesetter->write(title, to_utf8_span(mTitle));
            typesetter->sectionBreak();
        }

        typedef std::pair<size_t, size_t> Range;
        std::map<Range, intptr_t> hyperLinks;

        // We need this copy for when @# hyperlinks are replaced
        std::string text = mText;

        size_t pos_end = std::string::npos;
        for (;;)
        {
            size_t pos_begin = text.find('@');
            if (pos_begin != std::string::npos)
                pos_end = text.find('#', pos_begin);

            if (pos_begin != std::string::npos && pos_end != std::string::npos)
            {
                std::string link = text.substr(pos_begin + 1, pos_end - pos_begin - 1);
                const char specialPseudoAsteriskCharacter = 127;
                std::replace(link.begin(), link.end(), specialPseudoAsteriskCharacter, '*');
                std::string topicName
                    = Misc::StringUtils::lowerCase(windowManager->getTranslationDataStorage().topicStandardForm(link));

                std::string displayName = std::move(link);
                while (displayName[displayName.size() - 1] == '*')
                    displayName.erase(displayName.size() - 1, 1);

                text.replace(pos_begin, pos_end + 1 - pos_begin, displayName);

                if (topicLinks.find(topicName) != topicLinks.end())
                    hyperLinks[std::make_pair(pos_begin, pos_begin + displayName.size())]
                        = intptr_t(topicLinks[topicName].get());
            }
            else
                break;
        }

        typesetter->addContent(to_utf8_span(text));

        if (hyperLinks.size()
            && MWBase::Environment::get().getWindowManager()->getTranslationDataStorage().hasTranslation())
        {
            const TextColours& textColours = MWBase::Environment::get().getWindowManager()->getTextColours();

            BookTypesetter::Style* style = typesetter->createStyle({}, textColours.normal, false);
            size_t formatted = 0; // points to the first character that is not laid out yet
            for (auto& hyperLink : hyperLinks)
            {
                intptr_t topicId = hyperLink.second;
                BookTypesetter::Style* hotStyle = typesetter->createHotStyle(
                    style, textColours.link, textColours.linkOver, textColours.linkPressed, topicId);
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

            std::string::const_iterator i = text.begin();
            for (KeywordSearchT::Match& match : matches)
            {
                if (i != match.mBeg)
                    addTopicLink(typesetter, 0, i - text.begin(), match.mBeg - text.begin());

                addTopicLink(typesetter, match.mValue, match.mBeg - text.begin(), match.mEnd - text.begin());

                i = match.mEnd;
            }
            if (i != text.end())
                addTopicLink(std::move(typesetter), 0, i - text.begin(), text.size());
        }
    }

    void Response::addTopicLink(BookTypesetter::Ptr typesetter, intptr_t topicId, size_t begin, size_t end) const
    {
        const TextColours& textColours = MWBase::Environment::get().getWindowManager()->getTextColours();

        BookTypesetter::Style* style = typesetter->createStyle({}, textColours.normal, false);

        if (topicId)
            style = typesetter->createHotStyle(
                style, textColours.link, textColours.linkOver, textColours.linkPressed, topicId);
        typesetter->write(style, begin, end);
    }

    Message::Message(std::string_view text)
    {
        mText = text;
    }

    void Message::write(BookTypesetter::Ptr typesetter, KeywordSearchT* keywordSearch,
        std::map<std::string, std::unique_ptr<Link>>& topicLinks) const
    {
        const MyGUI::Colour& textColour = MWBase::Environment::get().getWindowManager()->getTextColours().notify;
        BookTypesetter::Style* title = typesetter->createStyle({}, textColour, false);
        typesetter->sectionBreak(9);
        typesetter->write(title, to_utf8_span(mText));
    }

    // --------------------------------------------------------------------------------------------------

    void Choice::activated()
    {
        MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Menu Click"));
        eventChoiceActivated(mChoiceId);
    }

    void Topic::activated()
    {
        MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Menu Click"));
        eventTopicActivated(mTopicId);
    }

    void Goodbye::activated()
    {
        MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Menu Click"));
        eventActivated();
    }

    // --------------------------------------------------------------------------------------------------

    DialogueWindow::DialogueWindow()
        : WindowBase("openmw_dialogue_window.layout")
        , mIsCompanion(false)
        , mGoodbye(false)
        , mPersuasionDialog(std::make_unique<ResponseCallback>(this))
        , mCallback(std::make_unique<ResponseCallback>(this))
        , mGreetingCallback(std::make_unique<ResponseCallback>(this, false))
    {
        // Centre dialog
        center();

        mPersuasionDialog.setVisible(false);

        // History view
        getWidget(mHistory, "History");

        // Topics list
        getWidget(mTopicsList, "TopicsList");
        mTopicsList->eventItemSelected += MyGUI::newDelegate(this, &DialogueWindow::onSelectListItem);

        getWidget(mGoodbyeButton, "ByeButton");
        mGoodbyeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DialogueWindow::onByeClicked);

        getWidget(mDispositionBar, "Disposition");
        getWidget(mDispositionText, "DispositionText");
        getWidget(mScrollBar, "VScroll");

        mScrollBar->eventScrollChangePosition += MyGUI::newDelegate(this, &DialogueWindow::onScrollbarMoved);
        mHistory->eventMouseWheel += MyGUI::newDelegate(this, &DialogueWindow::onMouseWheel);

        BookPage::ClickCallback callback = [this](TypesetBook::InteractiveId link) { notifyLinkClicked(link); };
        mHistory->adviseLinkClicked(std::move(callback));

        mMainWidget->castType<MyGUI::Window>()->eventWindowChangeCoord
            += MyGUI::newDelegate(this, &DialogueWindow::onWindowResize);
    }

    void DialogueWindow::onTradeComplete()
    {
        MyGUI::UString message = MyGUI::LanguageManager::getInstance().replaceTags("#{sBarterDialog5}");
        addResponse({}, message);
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
        if (mCurrentWindowSize == _sender->getSize())
            return;

        redrawTopicsList();
        updateHistory();
        mCurrentWindowSize = _sender->getSize();
    }

    void DialogueWindow::onMouseWheel(MyGUI::Widget* _sender, int _rel)
    {
        if (!mScrollBar->getVisible())
            return;
        mScrollBar->setScrollPosition(
            std::clamp<int>(mScrollBar->getScrollPosition() - _rel * 0.3, 0, mScrollBar->getScrollRange() - 1));
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
        MWBase::DialogueManager* dialogueManager = MWBase::Environment::get().getDialogueManager();

        if (mGoodbye || dialogueManager->isInChoice())
            return;

        const MWWorld::Store<ESM::GameSetting>& gmst
            = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();

        const std::string& sPersuasion = gmst.find("sPersuasion")->mValue.getString();
        const std::string& sCompanionShare = gmst.find("sCompanionShare")->mValue.getString();
        const std::string& sBarter = gmst.find("sBarter")->mValue.getString();
        const std::string& sSpells = gmst.find("sSpells")->mValue.getString();
        const std::string& sTravel = gmst.find("sTravel")->mValue.getString();
        const std::string& sSpellMakingMenuTitle = gmst.find("sSpellMakingMenuTitle")->mValue.getString();
        const std::string& sEnchanting = gmst.find("sEnchanting")->mValue.getString();
        const std::string& sServiceTrainingTitle = gmst.find("sServiceTrainingTitle")->mValue.getString();
        const std::string& sRepair = gmst.find("sRepair")->mValue.getString();

        if (topic != sPersuasion && topic != sCompanionShare && topic != sBarter && topic != sSpells && topic != sTravel
            && topic != sSpellMakingMenuTitle && topic != sEnchanting && topic != sServiceTrainingTitle
            && topic != sRepair)
        {
            onTopicActivated(topic);
            if (mGoodbyeButton->getEnabled())
                MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mGoodbyeButton);
        }
        else if (topic == sPersuasion)
            mPersuasionDialog.setVisible(true);
        else if (topic == sCompanionShare)
            MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Companion, mPtr);
        else if (!dialogueManager->checkServiceRefused(mCallback.get()))
        {
            if (topic == sBarter
                && !dialogueManager->checkServiceRefused(mCallback.get(), MWBase::DialogueManager::Barter))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Barter, mPtr);
            else if (topic == sSpells
                && !dialogueManager->checkServiceRefused(mCallback.get(), MWBase::DialogueManager::Spells))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_SpellBuying, mPtr);
            else if (topic == sTravel
                && !dialogueManager->checkServiceRefused(mCallback.get(), MWBase::DialogueManager::Travel))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Travel, mPtr);
            else if (topic == sSpellMakingMenuTitle
                && !dialogueManager->checkServiceRefused(mCallback.get(), MWBase::DialogueManager::Spellmaking))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_SpellCreation, mPtr);
            else if (topic == sEnchanting
                && !dialogueManager->checkServiceRefused(mCallback.get(), MWBase::DialogueManager::Enchanting))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Enchanting, mPtr);
            else if (topic == sServiceTrainingTitle
                && !dialogueManager->checkServiceRefused(mCallback.get(), MWBase::DialogueManager::Training))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Training, mPtr);
            else if (topic == sRepair
                && !dialogueManager->checkServiceRefused(mCallback.get(), MWBase::DialogueManager::Repair))
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_MerchantRepair, mPtr);
        }
        else
            updateTopics();
    }

    void DialogueWindow::setPtr(const MWWorld::Ptr& actor)
    {
        if (actor.isEmpty() || !actor.getClass().isActor())
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
            for (auto& link : mLinks)
                mDeleteLater.push_back(
                    std::move(link)); // Links are not deleted right away to prevent issues with event handlers
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
        MWMechanics::CreatureStats& sellerStats = mPtr.getClass().getCreatureStats(mPtr);
        float delay = MWBase::Environment::get()
                          .getESMStore()
                          ->get<ESM::GameSetting>()
                          .find("fBarterGoldResetDelay")
                          ->mValue.getFloat();

        // Gold is restocked every 24h
        if (MWBase::Environment::get().getWorld()->getTimeStamp() >= sellerStats.getLastRestockTime() + delay)
        {
            sellerStats.setGoldPool(mPtr.getClass().getBaseGold(mPtr));

            sellerStats.setLastRestockTime(MWBase::Environment::get().getWorld()->getTimeStamp());
        }
    }

    void DialogueWindow::deleteLater()
    {
        mDeleteLater.clear();
    }

    void DialogueWindow::onClose()
    {
        if (MWBase::Environment::get().getWindowManager()->containsMode(GM_Dialogue))
            return;
        // Reset history
        mHistoryContents.clear();
    }

    bool DialogueWindow::setKeywords(const std::list<std::string>& keyWords)
    {
        if (mKeywords == keyWords && isCompanion() == mIsCompanion)
            return false;
        mIsCompanion = isCompanion();
        mKeywords = keyWords;
        updateTopicsPane();
        return true;
    }

    void DialogueWindow::redrawTopicsList()
    {
        mTopicsList->adjustSize();

        // The topics list has been regenerated so topic formatting needs to be updated
        updateTopicFormat();
    }

    void DialogueWindow::updateTopicsPane()
    {
        mTopicsList->clear();
        for (auto& linkPair : mTopicLinks)
            mDeleteLater.push_back(std::move(linkPair.second));
        mTopicLinks.clear();
        mKeywordSearch.clear();

        int services = mPtr.getClass().getServices(mPtr);

        bool travel = (mPtr.getType() == ESM::NPC::sRecordId && !mPtr.get<ESM::NPC>()->mBase->getTransport().empty())
            || (mPtr.getType() == ESM::Creature::sRecordId
                && !mPtr.get<ESM::Creature>()->mBase->getTransport().empty());

        const MWWorld::Store<ESM::GameSetting>& gmst
            = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();

        if (mPtr.getType() == ESM::NPC::sRecordId)
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

        // Morrowind uses 3 px invisible borders for padding topics
        constexpr int verticalPadding = 3;

        for (const auto& keyword : mKeywords)
        {
            std::string topicId = Misc::StringUtils::lowerCase(keyword);
            mTopicsList->addItem(keyword, verticalPadding);

            auto t = std::make_unique<Topic>(keyword);
            mKeywordSearch.seed(topicId, intptr_t(t.get()));
            t->eventTopicActivated += MyGUI::newDelegate(this, &DialogueWindow::onTopicActivated);
            mTopicLinks[topicId] = std::move(t);
        }

        redrawTopicsList();
        updateHistory();
    }

    void DialogueWindow::updateHistory(bool scrollbar)
    {
        if (!scrollbar && mScrollBar->getVisible())
        {
            mHistory->setSize(mHistory->getSize() + MyGUI::IntSize(mScrollBar->getWidth(), 0));
            mScrollBar->setVisible(false);
        }
        if (scrollbar && !mScrollBar->getVisible())
        {
            mHistory->setSize(mHistory->getSize() - MyGUI::IntSize(mScrollBar->getWidth(), 0));
            mScrollBar->setVisible(true);
        }

        BookTypesetter::Ptr typesetter = BookTypesetter::create(mHistory->getWidth(), std::numeric_limits<int>::max());

        for (const auto& text : mHistoryContents)
            text->write(typesetter, &mKeywordSearch, mTopicLinks);

        BookTypesetter::Style* body = typesetter->createStyle({}, MyGUI::Colour::White, false);

        typesetter->sectionBreak(9);
        // choices
        const TextColours& textColours = MWBase::Environment::get().getWindowManager()->getTextColours();
        mChoices = MWBase::Environment::get().getDialogueManager()->getChoices();
        for (std::pair<std::string, int>& choice : mChoices)
        {
            auto link = std::make_unique<Choice>(choice.second);
            link->eventChoiceActivated += MyGUI::newDelegate(this, &DialogueWindow::onChoiceActivated);
            auto interactiveId = TypesetBook::InteractiveId(link.get());
            mLinks.push_back(std::move(link));

            typesetter->lineBreak();
            BookTypesetter::Style* questionStyle = typesetter->createHotStyle(
                body, textColours.answer, textColours.answerOver, textColours.answerPressed, interactiveId);
            typesetter->write(questionStyle, to_utf8_span(choice.first));
        }

        mGoodbye = MWBase::Environment::get().getDialogueManager()->isGoodbye();
        if (mGoodbye)
        {
            auto link = std::make_unique<Goodbye>();
            link->eventActivated += MyGUI::newDelegate(this, &DialogueWindow::onGoodbyeActivated);
            auto interactiveId = TypesetBook::InteractiveId(link.get());
            mLinks.push_back(std::move(link));
            const std::string& goodbye = MWBase::Environment::get()
                                             .getESMStore()
                                             ->get<ESM::GameSetting>()
                                             .find("sGoodbye")
                                             ->mValue.getString();
            BookTypesetter::Style* questionStyle = typesetter->createHotStyle(
                body, textColours.answer, textColours.answerOver, textColours.answerPressed, interactiveId);
            typesetter->lineBreak();
            typesetter->write(questionStyle, to_utf8_span(goodbye));
        }

        TypesetBook::Ptr book = typesetter->complete();
        mHistory->showPage(book, 0);
        size_t viewHeight = mHistory->getParent()->getHeight();
        if (!scrollbar && book->getSize().second > viewHeight)
            updateHistory(true);
        else if (scrollbar)
        {
            mHistory->setSize(MyGUI::IntSize(mHistory->getWidth(), book->getSize().second));
            // Scroll range should be >= 2 to enable scrolling and prevent a crash
            size_t range = std::max(book->getSize().second - viewHeight, size_t(2));
            mScrollBar->setScrollRange(range);
            mScrollBar->setScrollPosition(range - 1);
            mScrollBar->setTrackSize(
                static_cast<int>(viewHeight / static_cast<float>(book->getSize().second) * mScrollBar->getLineSize()));
            onScrollbarMoved(mScrollBar, range - 1);
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

    void DialogueWindow::notifyLinkClicked(TypesetBook::InteractiveId link)
    {
        reinterpret_cast<Link*>(link)->activated();
    }

    void DialogueWindow::onTopicActivated(const std::string& topicId)
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

    void DialogueWindow::onScrollbarMoved(MyGUI::ScrollBar* sender, size_t pos)
    {
        mHistory->setPosition(0, static_cast<int>(pos) * -1);
    }

    void DialogueWindow::addResponse(std::string_view title, std::string_view text, bool needMargin)
    {
        mHistoryContents.push_back(std::make_unique<Response>(text, title, needMargin));
        updateHistory();
    }

    void DialogueWindow::addMessageBox(std::string_view text)
    {
        mHistoryContents.push_back(std::make_unique<Message>(text));
        updateHistory();
    }

    void DialogueWindow::updateDisposition()
    {
        bool dispositionVisible = false;
        if (!mPtr.isEmpty() && mPtr.getClass().isNpc())
        {
            // If actor was a witness to a crime which was payed off,
            // restore original disposition immediately.
            MWMechanics::NpcStats& npcStats = mPtr.getClass().getNpcStats(mPtr);
            if (npcStats.getCrimeId() != -1 && npcStats.getCrimeDispositionModifier() != 0)
            {
                if (npcStats.getCrimeId() <= MWBase::Environment::get().getWorld()->getPlayer().getCrimeId())
                    npcStats.setCrimeDispositionModifier(0);
            }

            dispositionVisible = true;
            mDispositionBar->setProgressRange(100);
            mDispositionBar->setProgressPosition(
                MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(mPtr));
            mDispositionText->setCaption(
                MyGUI::utility::toString(MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(mPtr))
                + std::string("/100"));
        }

        if (mDispositionBar->getVisible() != dispositionVisible)
        {
            mDispositionBar->setVisible(dispositionVisible);
            const int offset = (mDispositionBar->getHeight() + 5) * (dispositionVisible ? 1 : -1);
            mTopicsList->setCoord(mTopicsList->getCoord() + MyGUI::IntCoord(0, offset, 0, -offset));
            redrawTopicsList();
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

    void DialogueWindow::updateTopicFormat()
    {
        if (!Settings::gui().mColorTopicEnable)
            return;

        for (const std::string& keyword : mKeywords)
        {
            int flag = MWBase::Environment::get().getDialogueManager()->getTopicFlag(ESM::RefId::stringRefId(keyword));
            MyGUI::Button* button = mTopicsList->getItemWidget(keyword);
            const auto oldCaption = button->getCaption();
            const MyGUI::IntSize oldSize = button->getSize();

            bool changed = false;
            if (flag & MWBase::DialogueManager::TopicType::Specific)
            {
                button->changeWidgetSkin("MW_ListLine_Specific");
                changed = true;
            }
            else if (flag & MWBase::DialogueManager::TopicType::Exhausted)
            {
                button->changeWidgetSkin("MW_ListLine_Exhausted");
                changed = true;
            }

            if (changed)
            {
                button->setCaption(oldCaption);
                button->setTextAlign(MyGUI::Align::Left);
                MyGUI::ISubWidgetText* text = button->getSubWidgetText();
                if (text != nullptr)
                    text->setWordWrap(true);
                button->setSize(oldSize);
            }
        }
    }

    void DialogueWindow::updateTopics()
    {
        // Topic formatting needs to be updated regardless of whether the topic list has changed
        if (!setKeywords(MWBase::Environment::get().getDialogueManager()->getAvailableTopics()))
            updateTopicFormat();
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
