#include "mainmenu.hpp"

#include <OgreRoot.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

namespace MWGui
{

    MainMenu::MainMenu(int w, int h)
        : OEngine::GUI::Layout("openmw_mainmenu.layout")
        , mButtonBox(0)
    {
        onResChange(w,h);
    }

    void MainMenu::onResChange(int w, int h)
    {
        setCoord(0,0,w,h);


        if (mButtonBox)
            MyGUI::Gui::getInstance ().destroyWidget(mButtonBox);

        mButtonBox = mMainWidget->createWidget<MyGUI::Widget>("", MyGUI::IntCoord(0, 0, 0, 0), MyGUI::Align::Default);
        int curH = 0;

        std::vector<std::string> buttons;
        buttons.push_back("return");
        //buttons.push_back("newgame");
        //buttons.push_back("loadgame");
        //buttons.push_back("savegame");
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

        mButtonBox->setCoord (w/2 - maxwidth/2, h/2 - curH/2, maxwidth, curH);
    }

    void MainMenu::onButtonClicked(MyGUI::Widget *sender)
    {
        if (sender == mButtons["return"])
            MWBase::Environment::get().getWindowManager ()->removeGuiMode (GM_MainMenu);
        else if (sender == mButtons["options"])
            MWBase::Environment::get().getWindowManager ()->pushGuiMode (GM_Settings);
        else if (sender == mButtons["exitgame"])
            Ogre::Root::getSingleton ().queueEndRendering ();
    }

}
