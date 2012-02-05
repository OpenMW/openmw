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
    std::string lower;
    std::transform(str.begin(), str.end(), std::back_inserter(lower), std::tolower);
    return lower;
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

        //std::cout << "Clicked on key: " << key << std::endl;
        if(color == "#686EBA")
        {
            mEnvironment.mDialogueManager->keywordSelected(lower_string(key));
            displayTopicText(lower_string(key));
        }
        if(color == "#572D21") 
        {
            //TODO: send back the answere to the question!
            mEnvironment.mDialogueManager->questionAnswered(key);
            //std::cout << "and the ansere is..."<< key;
        }
    }
}

void DialogueWindow::open()
{
    //updateOptions();
    topicsList->removeAllItems();
    pTopicsText.clear();
    history->eraseText(0,history->getTextLength());
    updateOptions();
    setVisible(true);
}

void DialogueWindow::onByeClicked(MyGUI::Widget* _sender)
{
    //eventBye();
    mEnvironment.mDialogueManager->goodbyeSelected();
}

void DialogueWindow::onSelectTopic(MyGUI::List* _sender, size_t _index)
{
    if (_index == MyGUI::ITEM_NONE)
        return;
    std::string topic =  _sender->getItem(_index);
    mEnvironment.mDialogueManager->keywordSelected(lower_string(topic));
    displayTopicText(topic);

    //const std::string* theTopic  = topicsList->getItemDataAt<std::string>(_index);
    //std::cout << "Selected: "<< theTopic << std::endl;
    //eventTopicSelected(key);
}

void DialogueWindow::startDialogue(std::string npcName)
{
    setText("NpcName", npcName);
}

void DialogueWindow::addKeyword(std::string keyWord,std::string topicText)
{
    if(topicsList->findItemIndexWith(keyWord) == MyGUI::ITEM_NONE)
    {
        topicsList->addItem(keyWord);
        pTopicsText[keyWord] = topicText; 
    }
}

void DialogueWindow::removeKeyword(std::string keyWord)
{
    if(topicsList->findItemIndexWith(keyWord) != MyGUI::ITEM_NONE)
    {
        std::cout << topicsList->findItem(keyWord);
        topicsList->removeItemAt(topicsList->findItem(keyWord));
        pTopicsText.erase(keyWord);
    }
}

void addColorInString(std::string& str, const std::string& keyword,std::string color1, std::string color2)
{
    size_t pos = 0;
    while((pos = find_str_ci(str,keyword, pos)) != std::string::npos)
    {
        //str.replace(pos, oldStr.length(), "#686EBA"+str.get);
        str.insert(pos,color1);
        pos += color1.length();
        pos += keyword.length();
        str.insert(pos,color2);
        pos+= color2.length();
    }
}

std::string DialogueWindow::parseText(std::string text)
{
    //topicsList->geti
    for(int i = 0;i<topicsList->getItemCount();i++)
    {
        std::string keyWord = topicsList->getItem(i);
        //std::string newKeyWord = "#686EBA"+keyWord+"#B29154";
        addColorInString(text,keyWord,"#686EBA","#B29154");
    }
    return text;
}

void DialogueWindow::displayTopicText(std::string topic)
{
    if(topicsList->findItemIndexWith(topic) != MyGUI::ITEM_NONE)
    {
        history->addDialogHeading(topic);
        history->addDialogText(parseText(pTopicsText[topic]));
    }
    else
    {
        std::cout << "topic not found!";
    }
}

void DialogueWindow::addText(std::string text)
{
    history->addDialogText(parseText(text));
}

void DialogueWindow::askQuestion(std::string question,std::list<std::string> answers)
{
    history->addDialogText(parseText(question));
    for(std::list<std::string>::iterator it = answers.begin();it!=answers.end();it++)
    {
        history->addDialogText("#572D21"+(*it)+"#B29154"+" ");
    }
}

void DialogueWindow::updateOptions()
{
    //FIXME Add this properly
    /*history->addDialogText("Through the translucent surface of the orb, you see shifting images of distant locations...");
    for(int z = 0; z < 10; z++)
    {
    history->addDialogHeading("Fort Frostmoth");
    history->addDialogText("The image in the orb flickers, and you see.... The cold courtyard of #FF0000Fort Frostmoth#FFFFFF, battered bu werewolf attack, but still standing, still projecting Imperial might even to this distant and cold corner of the world.");
    }*/

    //Clear the list of topics
    topicsList->removeAllItems();
    pTopicsText.clear();
    history->eraseText(0,history->getTextLength());

    /*addKeyword("gus","gus is working on the dialogue system");
    displayTopicText("gus");*/

    pDispositionBar->setProgressRange(100);
    pDispositionBar->setProgressPosition(40);
    pDispositionText->eraseText(0,pDispositionText->getTextLength());
    pDispositionText->addText("#B29154"+std::string("40/100")+"#B29154");

    /*std::list<std::string> test;
    test.push_back("option 1");
    test.push_back("option 2");
    askQuestion("is gus cooking?",test);*/
    /*topicsList->addItem("Ald'ruhn", i++);
    topicsList->addItem("Balmora", i++);
    topicsList->addItem("Sadrith Mora", i++);
    topicsList->addItem("Vivec", i++);
    topicsList->addItem("Ald Velothi", i++);
    topicsList->addItem("Caldera", i++);
    topicsList->addItem("Dagon Fel ", i++);
    topicsList->addItem("Gnaar Mok", i++);
    topicsList->addItem("Gnisis", i++);
    topicsList->addItem("Hla Oad", i++);
    topicsList->addItem("Khuul", i++);
    topicsList->addItem("Maar Gan", i++);
    topicsList->addItem("Molag Mar", i++);
    topicsList->addItem("Pelagiad", i++);
    topicsList->addItem("Seyda Neen", i++);
    topicsList->addItem("Suran", i++);
    topicsList->addItem("Tel Aruhn", i++);
    topicsList->addItem("Tel Branora", i++);
    topicsList->addItem("Tel Fyr", i++);
    topicsList->addItem("Tel Mora", i++);
    topicsList->addItem("Tel Vos", i++);
    topicsList->addItem("Vos", i++);*/
}

