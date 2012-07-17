#include "dialogue.hpp"

#include <iostream>
#include <iterator>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <components/esm_store/store.hpp>

#include "../mwbase/environment.hpp"
#include "../mwdialogue/dialoguemanager.hpp"

#include "dialogue_history.hpp"
#include "window_manager.hpp"
#include "widgets.hpp"
#include "list.hpp"
#include "tradewindow.hpp"
#include "inventorywindow.hpp"

using namespace MWGui;
using namespace Widgets;

/**
*Copied from the internet.
*/

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


DialogueWindow::DialogueWindow(WindowManager& parWindowManager)
    : WindowBase("openmw_dialogue_window.layout", parWindowManager)
    , mEnabled(true)
    , mShowTrade(false)
{
    // Centre dialog
    center();

    //History view
    getWidget(mHistory, "History");
    mHistory->setOverflowToTheLeft(true);
    mHistory->setMaxTextLength(1000000);
    Widget* eventbox;

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
    ISubWidgetText* t = mHistory->getClient()->getSubWidgetText();
    if(t == nullptr)
        return;

    const IntPoint& lastPressed = InputManager::getInstance().getLastPressedPosition(MyGUI::MouseButton::Left);

    size_t cursorPosition = t->getCursorPosition(lastPressed);
    MyGUI::UString color = mHistory->getColorAtPos(cursorPosition);

    if (!mEnabled && color == "#572D21")
        MWBase::Environment::get().getDialogueManager()->goodbyeSelected();

    if(color != "#B29154")
    {
        UString key = mHistory->getColorTextAt(cursorPosition);
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

    if (topic == MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sBarter")->str)
    {
        /// \todo check if the player is allowed to trade with this actor (e.g. faction rank high enough)?
        mWindowManager.pushGuiMode(GM_Barter);
        mWindowManager.getTradeWindow()->startTrade(mPtr);
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
    mHistory->eraseText(0,mHistory->getTextLength());
    updateOptions();
}

void DialogueWindow::setKeywords(std::list<std::string> keyWords)
{
    mTopicsList->clear();

    bool anyService = mShowTrade;

    if (mShowTrade)
        mTopicsList->addItem(MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sBarter")->str);

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

    mDispositionBar->setProgressRange(100);
    mDispositionBar->setProgressPosition(40);
    mDispositionText->eraseText(0, mDispositionText->getTextLength());
    mDispositionText->addText("#B29154"+std::string("40/100")+"#B29154");
}

void DialogueWindow::goodbye()
{
    mHistory->addDialogText("\n#572D21" + MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sGoodbye")->str);
    mTopicsList->setEnabled(false);
    mEnabled = false;
}

void DialogueWindow::onReferenceUnavailable()
{
    mWindowManager.removeGuiMode(GM_Dialogue);
}
