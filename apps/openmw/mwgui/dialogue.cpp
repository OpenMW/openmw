#include "dialogue.hpp"

#include <assert.h>
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
    : WindowBase("openmw_dialogue_window_layout.xml", parWindowManager)
    , mEnabled(true)
    , mShowTrade(false)
{
    // Centre dialog
    center();

    //History view
    getWidget(history, "History");
    history->setOverflowToTheLeft(true);
    history->setMaxTextLength(1000000);
    Widget* eventbox;

    //An EditBox cannot receive mouse click events, so we use an
    //invisible widget on top of the editbox to receive them
    getWidget(eventbox, "EventBox");
    eventbox->eventMouseButtonClick += MyGUI::newDelegate(this, &DialogueWindow::onHistoryClicked);
    eventbox->eventMouseWheel += MyGUI::newDelegate(this, &DialogueWindow::onMouseWheel);

    //Topics list
    getWidget(topicsList, "TopicsList");
    topicsList->eventItemSelected += MyGUI::newDelegate(this, &DialogueWindow::onSelectTopic);

    MyGUI::ButtonPtr byeButton;
    getWidget(byeButton, "ByeButton");
    byeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DialogueWindow::onByeClicked);
    byeButton->setCaption(MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sGoodbye")->str);

    getWidget(pDispositionBar, "Disposition");
    getWidget(pDispositionText,"DispositionText");

    static_cast<MyGUI::Window*>(mMainWidget)->eventWindowChangeCoord += MyGUI::newDelegate(this, &DialogueWindow::onWindowResize);
}

void DialogueWindow::onHistoryClicked(MyGUI::Widget* _sender)
{
    ISubWidgetText* t = history->getClient()->getSubWidgetText();
    if(t == nullptr)
        return;

    const IntPoint& lastPressed = InputManager::getInstance().getLastPressedPosition(MyGUI::MouseButton::Left);

    size_t cursorPosition = t->getCursorPosition(lastPressed);
    MyGUI::UString color = history->getColorAtPos(cursorPosition);

    if (!mEnabled && color == "#572D21")
        MWBase::Environment::get().getDialogueManager()->goodbyeSelected();

    if(color != "#B29154")
    {
        UString key = history->getColorTextAt(cursorPosition);
        if(color == "#686EBA") MWBase::Environment::get().getDialogueManager()->keywordSelected(lower_string(key));

        if(color == "#572D21") MWBase::Environment::get().getDialogueManager()->questionAnswered(lower_string(key));
    }
}

void DialogueWindow::onWindowResize(MyGUI::Window* _sender)
{
    topicsList->adjustSize();
}

void DialogueWindow::onMouseWheel(MyGUI::Widget* _sender, int _rel)
{
    if (history->getVScrollPosition() - _rel*0.3 < 0)
        history->setVScrollPosition(0);
    else
        history->setVScrollPosition(history->getVScrollPosition() - _rel*0.3);
}

void DialogueWindow::open()
{
    topicsList->clear();
    pTopicsText.clear();
    history->eraseText(0,history->getTextLength());
    updateOptions();
    setVisible(true);
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
        mWindowManager.setGuiMode(GM_Barter);
        mWindowManager.getTradeWindow()->startTrade(mActor);
    }

    else
        MWBase::Environment::get().getDialogueManager()->keywordSelected(lower_string(topic));
}

void DialogueWindow::startDialogue(MWWorld::Ptr actor, std::string npcName)
{
    mEnabled = true;
    mActor = actor;
    topicsList->setEnabled(true);
    setTitle(npcName);
}

void DialogueWindow::setKeywords(std::list<std::string> keyWords)
{
    topicsList->clear();

    bool anyService = mShowTrade;

    if (mShowTrade)
        topicsList->addItem(MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sBarter")->str);

    if (anyService)
        topicsList->addSeparator();

    for(std::list<std::string>::iterator it = keyWords.begin(); it != keyWords.end(); it++)
    {
        topicsList->addItem(*it);
    }
    topicsList->adjustSize();
}

void DialogueWindow::removeKeyword(std::string keyWord)
{
    if(topicsList->hasItem(keyWord))
    {
        topicsList->removeItem(keyWord);
        pTopicsText.erase(keyWord);
    }
    topicsList->adjustSize();
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
    for(unsigned int i = 0;i<topicsList->getItemCount();i++)
    {
        std::string keyWord = topicsList->getItemNameAt(i);
        if (keyWord != "")
            addColorInString(text,keyWord,"#686EBA","#B29154");
    }
    return text;
}

void DialogueWindow::addText(std::string text)
{
    history->addDialogText("#B29154"+parseText(text)+"#B29154");
}

void DialogueWindow::addTitle(std::string text)
{
    // This is called from the dialogue manager, so text is
    // case-smashed - thus we have to retrieve the correct case
    // of the text through the topic list.
    for (size_t i=0; i<topicsList->getItemCount(); ++i)
    {
        std::string item = topicsList->getItemNameAt(i);
        if (lower_string(item) == text)
            text = item;
    }

    history->addDialogHeading(text);
}

void DialogueWindow::askQuestion(std::string question)
{
    history->addDialogText("#572D21"+question+"#B29154"+" ");
}

void DialogueWindow::updateOptions()
{
    //Clear the list of topics
    topicsList->clear();
    pTopicsText.clear();
    history->eraseText(0,history->getTextLength());

    pDispositionBar->setProgressRange(100);
    pDispositionBar->setProgressPosition(40);
    pDispositionText->eraseText(0,pDispositionText->getTextLength());
    pDispositionText->addText("#B29154"+std::string("40/100")+"#B29154");
}

void DialogueWindow::goodbye()
{
    history->addDialogText("\n#572D21" + MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sGoodbye")->str);
    topicsList->setEnabled(false);
    mEnabled = false;
}
