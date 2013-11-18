#include "mainmenu.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/statemanager.hpp"

#include "savegamedialog.hpp"

namespace MWGui
{

    MainMenu::MainMenu(int w, int h)
        : OEngine::GUI::Layout("openmw_mainmenu.layout")
        , mButtonBox(0), mWidth (w), mHeight (h)
    {
        updateMenu();
    }

    void MainMenu::onResChange(int w, int h)
    {
        mWidth = w;
        mHeight = h;

        updateMenu();
    }

    void MainMenu::setVisible (bool visible)
    {
        if (visible)
            updateMenu();

        OEngine::GUI::Layout::setVisible (visible);
    }

    void MainMenu::onButtonClicked(MyGUI::Widget *sender)
    {
        MWBase::Environment::get().getSoundManager()->playSound("Menu Click", 1.f, 1.f);
        if (sender == mButtons["return"])
        {
            MWBase::Environment::get().getSoundManager ()->resumeSounds (MWBase::SoundManager::Play_TypeSfx);
            MWBase::Environment::get().getWindowManager ()->removeGuiMode (GM_MainMenu);
        }
        else if (sender == mButtons["options"])
            MWBase::Environment::get().getWindowManager ()->pushGuiMode (GM_Settings);
        else if (sender == mButtons["exitgame"])
            MWBase::Environment::get().getStateManager()->requestQuit();
        else if (sender == mButtons["newgame"])
        {
            MWBase::Environment::get().getStateManager()->newGame();
        }

        else if (sender == mButtons["loadgame"])
        {
            MWGui::SaveGameDialog* dialog = new MWGui::SaveGameDialog();
            dialog->setLoadOrSave(true);
            dialog->setVisible(true);
        }
        else if (sender == mButtons["savegame"])
        {
            MWGui::SaveGameDialog* dialog = new MWGui::SaveGameDialog();
            dialog->setLoadOrSave(false);
            dialog->setVisible(true);
        }
    }

    void MainMenu::updateMenu()
    {
        setCoord(0,0, mWidth, mHeight);


        if (mButtonBox)
            MyGUI::Gui::getInstance ().destroyWidget(mButtonBox);

        mButtonBox = mMainWidget->createWidget<MyGUI::Widget>("", MyGUI::IntCoord(0, 0, 0, 0), MyGUI::Align::Default);
        int curH = 0;

        MWBase::StateManager::State state = MWBase::Environment::get().getStateManager()->getState();

        std::vector<std::string> buttons;

        if (state==MWBase::StateManager::State_Running)
            buttons.push_back("return");

        buttons.push_back("newgame");

        /// \todo hide, if no saved game is available
        buttons.push_back("loadgame");

        if (state==MWBase::StateManager::State_Running)
            buttons.push_back("savegame");

        buttons.push_back("options");
        //buttons.push_back("credits");
        buttons.push_back("exitgame");

        int maxwidth = 0;

        mButtons.clear();
        for (std::vector<std::string>::iterator it = buttons.begin(); it != buttons.end(); ++it)
        {
            MWGui::ImageButton* button = mButtonBox->createWidget<MWGui::ImageButton>
                    ("ImageBox", MyGUI::IntCoord(0, curH, 0, 0), MyGUI::Align::Default);
            button->setProperty("ImageHighlighted", "textures\\menu_" + *it + "_over.dds");
            button->setProperty("ImageNormal", "textures\\menu_" + *it + ".dds");
            button->setProperty("ImagePushed", "textures\\menu_" + *it + "_pressed.dds");
            MyGUI::IntSize requested = button->getRequestedSize();
            button->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenu::onButtonClicked);
            mButtons[*it] = button;
            curH += requested.height;

            if (requested.width > maxwidth)
                maxwidth = requested.width;
        }
        for (std::map<std::string, MWGui::ImageButton*>::iterator it = mButtons.begin(); it != mButtons.end(); ++it)
        {
            MyGUI::IntSize requested = it->second->getRequestedSize();
            it->second->setCoord((maxwidth-requested.width) / 2, it->second->getTop(), requested.width, requested.height);
        }

        mButtonBox->setCoord (mWidth/2 - maxwidth/2, mHeight/2 - curH/2, maxwidth, curH);

    }
}
