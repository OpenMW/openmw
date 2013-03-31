#include "companionwindow.hpp"

#include <boost/lexical_cast.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/dialoguemanager.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "messagebox.hpp"

namespace MWGui
{

CompanionWindow::CompanionWindow(MWBase::WindowManager &parWindowManager, DragAndDrop *dragAndDrop, MessageBoxManager* manager)
    : ContainerBase(dragAndDrop)
    , WindowBase("openmw_companion_window.layout", parWindowManager)
    , mMessageBoxManager(manager)
{
    MyGUI::ScrollView* itemView;
    MyGUI::Widget* containerWidget;
    getWidget(containerWidget, "Items");
    getWidget(itemView, "ItemView");
    setWidgets(containerWidget, itemView);

    getWidget(mCloseButton, "CloseButton");
    getWidget(mProfitLabel, "ProfitLabel");
    getWidget(mEncumbranceBar, "EncumbranceBar");

    mCloseButton->eventMouseButtonClick += MyGUI::newDelegate(this, &CompanionWindow::onCloseButtonClicked);

    setCoord(200,0,600,300);
}

void CompanionWindow::open(MWWorld::Ptr npc)
{
    openContainer(npc);
    setTitle(MWWorld::Class::get(npc).getName(npc));
    drawItems();
    updateEncumbranceBar();
}

void CompanionWindow::notifyItemDragged(MWWorld::Ptr item, int count)
{
    if (mPtr.getTypeName() == typeid(ESM::NPC).name())
    {
        MWMechanics::NpcStats& stats = MWWorld::Class::get(mPtr).getNpcStats(mPtr);
        stats.modifyProfit(MWWorld::Class::get(item).getValue(item) * count);
    }
    updateEncumbranceBar();
}

void CompanionWindow::updateEncumbranceBar()
{
    float capacity = MWWorld::Class::get(mPtr).getCapacity(mPtr);
    float encumbrance = MWWorld::Class::get(mPtr).getEncumbrance(mPtr);
    mEncumbranceBar->setValue(encumbrance, capacity);

    if (mPtr.getTypeName() != typeid(ESM::NPC).name())
        mProfitLabel->setCaption("");
    else
    {
        MWMechanics::NpcStats& stats = MWWorld::Class::get(mPtr).getNpcStats(mPtr);
        mProfitLabel->setCaptionWithReplacing("#{sProfitValue} " + boost::lexical_cast<std::string>(stats.getProfit()));
    }
}

void CompanionWindow::onWindowResize(MyGUI::Window* window)
{
    drawItems();
}

void CompanionWindow::onCloseButtonClicked(MyGUI::Widget* _sender)
{
    if (mPtr.getTypeName() == typeid(ESM::NPC).name() && MWWorld::Class::get(mPtr).getNpcStats(mPtr).getProfit() < 0)
    {
        std::vector<std::string> buttons;
        buttons.push_back("#{sCompanionWarningButtonOne}");
        buttons.push_back("#{sCompanionWarningButtonTwo}");
        mMessageBoxManager->createInteractiveMessageBox("#{sCompanionWarningMessage}", buttons);
        mMessageBoxManager->eventButtonPressed += MyGUI::newDelegate(this, &CompanionWindow::onMessageBoxButtonClicked);
    }
    else
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Companion);
}

void CompanionWindow::onMessageBoxButtonClicked(int button)
{
    if (button == 0)
    {
        mPtr.getRefData().getLocals().setVarByInt(MWWorld::Class::get(mPtr).getScript(mPtr),
            "minimumProfit", MWWorld::Class::get(mPtr).getNpcStats(mPtr).getProfit());

        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Companion);
        MWBase::Environment::get().getDialogueManager()->startDialogue (mPtr);
    }
}

void CompanionWindow::onReferenceUnavailable()
{
    MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Companion);
}



}
