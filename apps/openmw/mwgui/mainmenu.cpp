#include "mainmenu.hpp"

#include <OgreRoot.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "confirmationdialog.hpp"

namespace MWGui
{

    MainMenu::MainMenu(MWBase::WindowManager& parWindowManager, int w, int h)
        : OEngine::GUI::Layout("openmw_mainmenu.layout")
        , mButtonBox(0)
        , mDialog(parWindowManager)
    {
        onResChange(w,h);
    }

    void MainMenu::onResChange(int w, int h)
    {
        setCoord(0,0,w,h);

        int height = 64 * 4;

        if (mButtonBox)
            MyGUI::Gui::getInstance ().destroyWidget(mButtonBox);

        mButtonBox = mMainWidget->createWidget<MyGUI::Widget>("", MyGUI::IntCoord(w/2 - 64, h/2 - height/2, 128, height), MyGUI::Align::Default);
        int curH = 0;

        mReturn = mButtonBox->createWidget<MyGUI::Button> ("ButtonImage", MyGUI::IntCoord(0, curH, 128, 64), MyGUI::Align::Default);
        mReturn->setImageResource ("Menu_Return");
        mReturn->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenu::returnToGame);
        curH += 64;

        mNewGame = mButtonBox->createWidget<MyGUI::Button> ("ButtonImage", MyGUI::IntCoord(0, curH, 128, 64), MyGUI::Align::Default);
        mNewGame->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenu::newGame);
        mNewGame->setImageResource ("Menu_NewGame");
        curH += 64;
/*
        mLoadGame = mButtonBox->createWidget<MyGUI::Button> ("ButtonImage", MyGUI::IntCoord(0, curH, 128, 64), MyGUI::Align::Default);
        mLoadGame->setImageResource ("Menu_LoadGame");
        curH += 64;


        mSaveGame = mButtonBox->createWidget<MyGUI::Button> ("ButtonImage", MyGUI::IntCoord(0, curH, 128, 64), MyGUI::Align::Default);
        mSaveGame->setImageResource ("Menu_SaveGame");
        curH += 64;
        */

        mOptions = mButtonBox->createWidget<MyGUI::Button> ("ButtonImage", MyGUI::IntCoord(0, curH, 128, 64), MyGUI::Align::Default);
        mOptions->setImageResource ("Menu_Options");
        mOptions->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenu::showOptions);
        curH += 64;

        /*
        mCredits = mButtonBox->createWidget<MyGUI::Button> ("ButtonImage", MyGUI::IntCoord(0, curH, 128, 64), MyGUI::Align::Default);
        mCredits->setImageResource ("Menu_Credits");
        curH += 64;
        */

        mExitGame = mButtonBox->createWidget<MyGUI::Button> ("ButtonImage", MyGUI::IntCoord(0, curH, 128, 64), MyGUI::Align::Default);
        mExitGame->setImageResource ("Menu_ExitGame");
        mExitGame->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenu::exitGame);
        curH += 64;
    }

    void MainMenu::returnToGame(MyGUI::Widget* sender)
    {
        MWBase::Environment::get().getWindowManager ()->removeGuiMode (GM_MainMenu);
    }

    void MainMenu::showOptions(MyGUI::Widget* sender)
    {
        MWBase::Environment::get().getWindowManager ()->pushGuiMode (GM_Settings);
    }

    void MainMenu::exitGame(MyGUI::Widget* sender)
    {
        Ogre::Root::getSingleton ().queueEndRendering ();
    }

    void MainMenu::newGame(MyGUI::Widget* sender)
    {
        mDialog.open ("#{sNotifyMessage54}");
        mDialog.eventOkClicked.clear();
        mDialog.eventCancelClicked.clear();
        mDialog.eventOkClicked += MyGUI::newDelegate(this, &MainMenu::newGameConfirmed);
    }

    void MainMenu::newGameConfirmed()
    {
        MWBase::Environment::get().getWindowManager ()->removeGuiMode (GM_MainMenu);
        MWBase::Environment::get().getWindowManager ()->newGame ();
        MWBase::Environment::get().getWorld ()->newGame();
    }
}
