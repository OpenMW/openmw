#include "dialogue.hpp"
#include "dialogue_history.hpp"
#include "window_manager.hpp"
#include "widgets.hpp"
#include "components/esm_store/store.hpp"
#include "../mwworld/environment.hpp"
#include "../mwdialogue/dialoguemanager.hpp"

#include <assert.h>
#include <iostream>
#include <iterator>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

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


DialogueWindow::DialogueWindow(WindowManager& parWindowManager,MWWorld::Environment& environment)
    : WindowBase("openmw_dialogue_window_layout.xml", parWindowManager),
    mEnvironment(environment)
{
    // Centre dialog
    center();

    //WindowManager *wm = environment.mWindowManager;
    setText("NpcName", "Name of character");

    //History view
    getWidget(history, "History");
    history->setOverflowToTheLeft(true);
    history->getClient()->eventMouseButtonClick = MyGUI::newDelegate(this, &DialogueWindow::onHistoryClicked);
    history->setMaxTextLength(1000000);
    //Topics list 
    getWidget(topicsList, "TopicsList");
    topicsList->setScrollVisible(true);
    //topicsList->eventListSelectAccept      = MyGUI::newDelegate(this, &DialogueWindow::onSelectTopic);
    topicsList->eventListMouseItemActivate = MyGUI::newDelegate(this, &DialogueWindow::onSelectTopic);
    //topicsList->eventListChangePosition    = MyGUI::newDelegate(this, &DialogueWindow::onSelectTopic);

    MyGUI::ButtonPtr byeButton;
    getWidget(byeButton, "ByeButton");
    byeButton->eventMouseButtonClick = MyGUI::newDelegate(this, &DialogueWindow::onByeClicked);

    getWidget(pDispositionBar, "Disposition");
    getWidget(pDispositionText,"DispositionText");
    std::cout << "creation dialogue";
}

void DialogueWindow::onHistoryClicked(MyGUI::Widget* _sender)
{
    ISubWidgetText* t = history->getSubWidgetText();
    if(t == nullptr)
        return;

    const IntPoint& lastPressed = InputManager::getInstance().getLastLeftPressed();

    size_t cursorPosition = t->getCursorPosition(lastPressed);
    MyGUI::UString color = history->getColorAtPos(cursorPosition);
    if(color != "#B29154")
    {
        UString key = history->getColorTextAt(cursorPosition);
        if(color == "#686EBA") mEnvironment.mDialogueManager->keywordSelected(lower_string(key));

        if(color == "#572D21") mEnvironment.mDialogueManager->questionAnswered(key);
    }
}

void DialogueWindow::open()
{
    topicsList->removeAllItems();
    pTopicsText.clear();
    history->eraseText(0,history->getTextLength());
    updateOptions();
    setVisible(true);
}

void DialogueWindow::onByeClicked(MyGUI::Widget* _sender)
{
    mEnvironment.mDialogueManager->goodbyeSelected();
}

void DialogueWindow::onSelectTopic(MyGUI::List* _sender, size_t _index)
{
    if (_index == MyGUI::ITEM_NONE)
        return;
    std::string topic =  _sender->getItemNameAt(_index);
    mEnvironment.mDialogueManager->keywordSelected(lower_string(topic));
}

void DialogueWindow::startDialogue(std::string npcName)
{
    setText("NpcName", npcName);
}

void DialogueWindow::addKeyword(std::string keyWord)
{
    if(topicsList->findItemIndexWith(keyWord) == MyGUI::ITEM_NONE)
    {
        topicsList->addItem(keyWord);
        pTopicsText[keyWord] = " "; 
    }
}

void DialogueWindow::removeKeyword(std::string keyWord)
{
    if(topicsList->findItemIndexWith(keyWord) != MyGUI::ITEM_NONE)
    {
        std::cout << topicsList->findItemIndexWith(keyWord);
        topicsList->removeItemAt(topicsList->findItemIndexWith(keyWord));
        pTopicsText.erase(keyWord);
    }
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
        else if(str.substr(pos -1,1) == " ")
        {
            str.insert(pos,color1);
            pos += color1.length();
            pos += keyword.length();
            str.insert(pos,color2);
            pos+= color2.length();
        }
    }
}

std::string DialogueWindow::parseText(std::string text)
{
    for(unsigned int i = 0;i<topicsList->getItemCount();i++)
    {
        std::string keyWord = topicsList->getItemNameAt(i);
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
    history->addDialogHeading(text);
}

void DialogueWindow::askQuestion(std::string question)
{
    history->addDialogText("#572D21"+question+"#B29154"+" ");
}

void DialogueWindow::updateOptions()
{
    //Clear the list of topics
    topicsList->removeAllItems();
    pTopicsText.clear();
    history->eraseText(0,history->getTextLength());

    pDispositionBar->setProgressRange(100);
    pDispositionBar->setProgressPosition(40);
    pDispositionText->eraseText(0,pDispositionText->getTextLength());
    pDispositionText->addText("#B29154"+std::string("40/100")+"#B29154");
}

