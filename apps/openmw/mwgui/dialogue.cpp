#include "dialogue.hpp"
#include "dialogue_history.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"
#include "window_manager.hpp"
#include "widgets.hpp"
#include "components/esm_store/store.hpp"

#include <assert.h>
#include <iostream>
#include <iterator>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using namespace MWGui;
using namespace Widgets;

DialogueWindow::DialogueWindow(MWWorld::Environment& environment)
  : Layout("openmw_dialogue_window_layout.xml")
  , environment(environment)
{
    // Centre dialog
    MyGUI::IntSize gameWindowSize = environment.mWindowManager->getGui()->getViewSize();
    MyGUI::IntCoord coord = mMainWidget->getCoord();
    coord.left = (gameWindowSize.width - coord.width)/2;
    coord.top = (gameWindowSize.height - coord.height)/2;
    mMainWidget->setCoord(coord);

    //WindowManager *wm = environment.mWindowManager;
    setText("NpcName", "Name of character");
  
    //History view
    getWidget(history, "History");
    history->setOverflowToTheLeft(true);
    history->getClient()->eventMouseButtonClick = MyGUI::newDelegate(this, &DialogueWindow::onHistoryClicked);
   
    //Topics list 
    getWidget(topicsList, "TopicsList");
    topicsList->setScrollVisible(true);
    topicsList->eventListSelectAccept      = MyGUI::newDelegate(this, &DialogueWindow::onSelectTopic);
    topicsList->eventListMouseItemActivate = MyGUI::newDelegate(this, &DialogueWindow::onSelectTopic);
    topicsList->eventListChangePosition    = MyGUI::newDelegate(this, &DialogueWindow::onSelectTopic);

    MyGUI::ButtonPtr byeButton;
    getWidget(byeButton, "ByeButton");
    byeButton->eventMouseButtonClick = MyGUI::newDelegate(this, &DialogueWindow::onByeClicked);

    updateOptions();
}

void DialogueWindow::onHistoryClicked(MyGUI::Widget* _sender)
{
    ISubWidgetText* t = history->getSubWidgetText();
    if(t == nullptr)
        return;

    const IntPoint& lastPressed = InputManager::getInstance().getLastLeftPressed();

    size_t cursorPosition = t->getCursorPosition(lastPressed);
    if(history->getColorAtPos(cursorPosition) != "#FFFFFF")
    {
        UString key = history->getColorTextAt(cursorPosition);
        std::cout << "Clicked on key: " << key << std::endl;
        //eventTopicSelected(key);
    }
}

void DialogueWindow::open()
{
    updateOptions();
    setVisible(true);
}

void DialogueWindow::onByeClicked(MyGUI::Widget* _sender)
{
    eventBye();
}

void DialogueWindow::onSelectTopic(MyGUI::List* _sender, size_t _index)
{
    if (_index == MyGUI::ITEM_NONE)
        return;

    //const std::string* theTopic  = topicsList->getItemDataAt<std::string>(_index);
    //std::cout << "Selected: "<< theTopic << std::endl;
    //eventTopicSelected(key);
}


void DialogueWindow::updateOptions()
{
    //FIXME Add this properly
    history->addDialogText("Through the translucent surface of the orb, you see shifting images of distant locations...");
    for(int z = 0; z < 10; z++)
    {
        history->addDialogHeading("Fort Frostmoth");
        history->addDialogText("The image in the orb flickers, and you see.... The cold courtyard of #FF0000Fort Frostmoth#FFFFFF, battered bu werewolf attack, but still standing, still projecting Imperial might even to this distant and cold corner of the world.");
    }

    //Clear the list of topics
    topicsList->removeAllItems();
    int i = 0;
    topicsList->addItem("Ald'ruhn", i++);
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
    topicsList->addItem("Vos", i++);
}

