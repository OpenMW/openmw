#include "dialogue.hpp"

#include <iostream>
#include <iterator>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "dialogue_history.hpp"
#include "widgets.hpp"
#include "list.hpp"
#include "tradewindow.hpp"
#include "spellbuyingwindow.hpp"
#include "inventorywindow.hpp"
#include "travelwindow.hpp"

using namespace MWGui;
using namespace Widgets;

/**
*Copied from the internet.
*/

namespace {

std::string lower_string(const std::string& str)
{
        std::string lowerCase;

        std::transform (str.begin(), str.end(), std::back_inserter (lowerCase),
            (int(*)(int)) std::tolower);

        return lowerCase;
}

std::string::size_type find_str_ci(const std::string& str, const std::string& substr,size_t pos)
{
    return lower_string(str).find(lower_string(substr),pos);
}

}



PersuasionDialog::PersuasionDialog(MWBase::WindowManager &parWindowManager)
    : WindowModal("openmw_persuasion_dialog.layout", parWindowManager)
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
        mWindowManager.getTradeWindow()->addOrRemoveGold(-10);
        type = MWBase::MechanicsManager::PT_Bribe10;
    }
    else if (sender == mBribe100Button)
    {
        mWindowManager.getTradeWindow()->addOrRemoveGold(-100);
        type = MWBase::MechanicsManager::PT_Bribe100;
    }
    else /*if (sender == mBribe1000Button)*/
    {
        mWindowManager.getTradeWindow()->addOrRemoveGold(-1000);
        type = MWBase::MechanicsManager::PT_Bribe1000;
    }

    eventPersuade(type, true, 0);
    setVisible(false);
}

void PersuasionDialog::open()
{
    WindowModal::open();
    center();

    int playerGold = mWindowManager.getInventoryWindow()->getPlayerGold();

    mBribe10Button->setEnabled (playerGold >= 10);
    mBribe100Button->setEnabled (playerGold >= 100);
    mBribe1000Button->setEnabled (playerGold >= 1000);

    mGoldLabel->setCaptionWithReplacing("#{sGold}: " + boost::lexical_cast<std::string>(playerGold));
}

// --------------------------------------------------------------------------------------------------

DialogueWindow::DialogueWindow(MWBase::WindowManager& parWindowManager)
    : WindowBase("openmw_dialogue_window.layout", parWindowManager)
    , mPersuasionDialog(parWindowManager)
    , mEnabled(false)
    , mServices(0)
{
    // Centre dialog
    center();

    mPersuasionDialog.setVisible(false);
    mPersuasionDialog.eventPersuade += MyGUI::newDelegate(this, &DialogueWindow::onPersuade);

    //History view
    getWidget(mHistory, "History");
    mHistory->setOverflowToTheLeft(true);
    mHistory->setMaxTextLength(1000000);
    MyGUI::Widget* eventbox;

    //An EditBox cannot receive mouse click events, so we use an
    //invisible widget on top of the editbox to receive them
    getWidget(eventbox, "EventBox");
    eventbox->eventMouseButtonClick += MyGUI::newDelegate(this, &DialogueWindow::onHistoryClicked);
    eventbox->eventMouseWheel += MyGUI::newDelegate(this, &DialogueWindow::onMouseWheel);

    //Topics list
    getWidget(mTopicsList, "TopicsList");
    mTopicsList->eventItemSelected += MyGUI::newDelegate(this, &DialogueWindow::onSelectTopic);

    MyGUI::ButtonPtr byeButton;
    getWidget(byeButton, "ByeButton");
    byeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DialogueWindow::onByeClicked);

    getWidget(mDispositionBar, "Disposition");
    getWidget(mDispositionText,"DispositionText");

    static_cast<MyGUI::Window*>(mMainWidget)->eventWindowChangeCoord += MyGUI::newDelegate(this, &DialogueWindow::onWindowResize);
}

void DialogueWindow::onHistoryClicked(MyGUI::Widget* _sender)
{
    MyGUI::ISubWidgetText* t = mHistory->getClient()->getSubWidgetText();
    if(t == nullptr)
        return;

    const MyGUI::IntPoint& lastPressed = MyGUI::InputManager::getInstance().getLastPressedPosition(MyGUI::MouseButton::Left);

    size_t cursorPosition = t->getCursorPosition(lastPressed);
    MyGUI::UString color = mHistory->getColorAtPos(cursorPosition);

    if (!mEnabled && color == "#572D21")
        MWBase::Environment::get().getDialogueManager()->goodbyeSelected();

    if(color != "#B29154")
    {
        MyGUI::UString key = mHistory->getColorTextAt(cursorPosition);
        if(color == "#686EBA") MWBase::Environment::get().getDialogueManager()->keywordSelected(lower_string(key));

        if(color == "#572D21") MWBase::Environment::get().getDialogueManager()->questionAnswered(lower_string(key));
    }
}

void DialogueWindow::onWindowResize(MyGUI::Window* _sender)
{
    mTopicsList->adjustSize();
}

void DialogueWindow::onMouseWheel(MyGUI::Widget* _sender, int _rel)
{
    if (mHistory->getVScrollPosition() - _rel*0.3 < 0)
        mHistory->setVScrollPosition(0);
    else
        mHistory->setVScrollPosition(mHistory->getVScrollPosition() - _rel*0.3);
}

void DialogueWindow::onByeClicked(MyGUI::Widget* _sender)
{
    MWBase::Environment::get().getDialogueManager()->goodbyeSelected();
}

void DialogueWindow::onSelectTopic(std::string topic)
{
    if (!mEnabled) return;

    const MWWorld::Store<ESM::GameSetting> &gmst =
        MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

    if (topic == gmst.find("sBarter")->getString())
    {
        /// \todo check if the player is allowed to trade with this actor (e.g. faction rank high enough)?
        mWindowManager.pushGuiMode(GM_Barter);
        mWindowManager.getTradeWindow()->startTrade(mPtr);
    }
    if (topic == gmst.find("sPersuasion")->getString())
    {
        mPersuasionDialog.setVisible(true);
    }
    else if (topic == gmst.find("sSpells")->getString())
    {
        mWindowManager.pushGuiMode(GM_SpellBuying);
        mWindowManager.getSpellBuyingWindow()->startSpellBuying(mPtr);
    }
    else if (topic == gmst.find("sTravel")->getString())
    {
        mWindowManager.pushGuiMode(GM_Travel);
        mWindowManager.getTravelWindow()->startTravel(mPtr);
    }
    else if (topic == gmst.find("sSpellMakingMenuTitle")->getString())
    {
        mWindowManager.pushGuiMode(GM_SpellCreation);
        mWindowManager.startSpellMaking (mPtr);
    }
    else if (topic == gmst.find("sEnchanting")->getString())
    {
        mWindowManager.pushGuiMode(GM_Enchanting);
        mWindowManager.startEnchanting (mPtr);
    }
    else if (topic == gmst.find("sServiceTrainingTitle")->getString())
    {
        mWindowManager.pushGuiMode(GM_Training);
        mWindowManager.startTraining (mPtr);
    }
    else
        MWBase::Environment::get().getDialogueManager()->keywordSelected(lower_string(topic));
}

void DialogueWindow::startDialogue(MWWorld::Ptr actor, std::string npcName)
{
    mEnabled = true;
    mPtr = actor;
    mTopicsList->setEnabled(true);
    setTitle(npcName);

    mTopicsList->clear();
    mHistory->setCaption("");
    updateOptions();
}

void DialogueWindow::setKeywords(std::list<std::string> keyWords)
{
    mTopicsList->clear();

    bool anyService = mServices > 0;

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

//    if (mServices & Service_Enchant)
//        mTopicsList->addItem(gmst.find("sEnchanting")->getString());

    if (mServices & Service_Training)
        mTopicsList->addItem(gmst.find("sServiceTrainingTitle")->getString());

    if (anyService)
        mTopicsList->addSeparator();

    for(std::list<std::string>::iterator it = keyWords.begin(); it != keyWords.end(); ++it)
    {
        mTopicsList->addItem(*it);
    }
    mTopicsList->adjustSize();
}

void DialogueWindow::removeKeyword(std::string keyWord)
{
    if(mTopicsList->hasItem(keyWord))
    {
        mTopicsList->removeItem(keyWord);
    }
    mTopicsList->adjustSize();
}

void addColorInString(std::string& str, const std::string& keyword,std::string color1, std::string color2)
{
    size_t pos = 0;
    while((pos = find_str_ci(str,keyword, pos)) != std::string::npos)
    {
        if(pos==0)
        {
            str.insert(pos,color1);
            pos += color1.length();
            pos += keyword.length();
            str.insert(pos,color2);
            pos+= color2.length();
        }
        else
        {
            if(str.substr(pos -1,1) == " ")
            {
                str.insert(pos,color1);
                pos += color1.length();
                pos += keyword.length();
                str.insert(pos,color2);
                pos+= color2.length();
            }
            else
            {
                pos += keyword.length();
            }
        }
    }
}

std::string DialogueWindow::parseText(std::string text)
{
    bool separatorReached = false; // only parse topics that are below the separator (this prevents actions like "Barter" that are not topics from getting blue-colored)
    for(unsigned int i = 0;i<mTopicsList->getItemCount();i++)
    {
        std::string keyWord = mTopicsList->getItemNameAt(i);
        if (separatorReached && keyWord != "")
            addColorInString(text,keyWord,"#686EBA","#B29154");
        else
            separatorReached = true;
    }
    return text;
}

void DialogueWindow::addText(std::string text)
{
    mHistory->addDialogText("#B29154"+parseText(text)+"#B29154");
}

void DialogueWindow::addTitle(std::string text)
{
    // This is called from the dialogue manager, so text is
    // case-smashed - thus we have to retrieve the correct case
    // of the text through the topic list.
    for (size_t i=0; i<mTopicsList->getItemCount(); ++i)
    {
        std::string item = mTopicsList->getItemNameAt(i);
        if (lower_string(item) == text)
            text = item;
    }

    mHistory->addDialogHeading(text);
}

void DialogueWindow::askQuestion(std::string question)
{
    mHistory->addDialogText("#572D21"+question+"#B29154"+" ");
}

void DialogueWindow::updateOptions()
{
    //Clear the list of topics
    mTopicsList->clear();
    mHistory->eraseText(0, mHistory->getTextLength());

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
    // Apply temporary disposition change to NPC's base disposition

    mHistory->addDialogText("\n#572D21" + MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("sGoodbye")->getString());
    mTopicsList->setEnabled(false);
    mEnabled = false;
}

void DialogueWindow::onReferenceUnavailable()
{
    mWindowManager.removeGuiMode(GM_Dialogue);
}

void DialogueWindow::onFrame()
{
    if(mEnabled && mPtr.getTypeName() == typeid(ESM::NPC).name())
    {
        mDispositionBar->setProgressRange(100);
        mDispositionBar->setProgressPosition(MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(mPtr));
        mDispositionText->eraseText(0, mDispositionText->getTextLength());
        mDispositionText->addText("#B29154"+boost::lexical_cast<std::string>(MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(mPtr))+std::string("/100")+"#B29154");
    }
}

void DialogueWindow::onPersuade(int type, bool success, float dispositionChange)
{
    std::string text;

    if (type == MWBase::MechanicsManager::PT_Admire)
        text = "sAdmire";
    else if (type == MWBase::MechanicsManager::PT_Taunt)
        text = "sTaunt";
    else if (type == MWBase::MechanicsManager::PT_Intimidate)
        text = "sIntimidate";
    else
        text = "sBribe";

    text += success ? "Fail" : "Success";

    mHistory->addDialogHeading( MyGUI::LanguageManager::getInstance().replaceTags("#{"+text+"}"));

    /// \todo text from INFO record, how to get the ID?
    //mHistory->addDialogText();
}
