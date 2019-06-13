#include "mainmenu.hpp"

#include <MyGUI_TextBox.h>
#include <MyGUI_Gui.h>
#include <MyGUI_RenderManager.h>

#include <components/widgets/imagebutton.hpp>
#include <components/settings/settings.hpp>
#include <components/vfs/manager.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/statemanager.hpp"

#include "savegamedialog.hpp"
#include "confirmationdialog.hpp"
#include "backgroundimage.hpp"
#include "videowidget.hpp"

namespace MWGui
{

    MainMenu::MainMenu(int w, int h, const VFS::Manager* vfs, const std::string& versionDescription)
        : WindowBase("openmw_mainmenu.layout")
        , mWidth (w), mHeight (h)
        , mVFS(vfs), mButtonBox(0)
        , mBackground(nullptr)
        , mVideoBackground(nullptr)
        , mVideo(nullptr)
        , mSaveGameDialog(nullptr)
    {
        getWidget(mVersionText, "VersionText");
        mVersionText->setCaption(versionDescription);

        mHasAnimatedMenu = mVFS->exists("video/menu_background.bik");

        updateMenu();
    }

    MainMenu::~MainMenu()
    {
        delete mSaveGameDialog;
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

        bool isMainMenu =
                MWBase::Environment::get().getWindowManager()->containsMode(MWGui::GM_MainMenu) &&
                MWBase::Environment::get().getStateManager()->getState() == MWBase::StateManager::State_NoGame;

        showBackground(isMainMenu);

        if (visible)
        {
            if (isMainMenu)
            {
                if (mButtons["loadgame"]->getVisible())
                    MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mButtons["loadgame"]);
                else
                    MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mButtons["newgame"]);
            }
            else
                MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mButtons["return"]);
        }

        Layout::setVisible (visible);
    }

    void MainMenu::onNewGameConfirmed()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode (MWGui::GM_MainMenu);
        MWBase::Environment::get().getStateManager()->newGame();
    }

    void MainMenu::onExitConfirmed()
    {
        MWBase::Environment::get().getStateManager()->requestQuit();
    }

    void MainMenu::onButtonClicked(MyGUI::Widget *sender)
    {
        MWBase::WindowManager *winMgr = MWBase::Environment::get().getWindowManager();

        std::string name = *sender->getUserData<std::string>();
        winMgr->playSound("Menu Click");
        if (name == "return")
        {
            winMgr->removeGuiMode (GM_MainMenu);
        }
        else if (name == "options")
            winMgr->pushGuiMode (GM_Settings);
        else if (name == "credits")
            winMgr->playVideo("mw_credits.bik", true);
        else if (name == "exitgame")
        {
            if (MWBase::Environment::get().getStateManager()->getState() == MWBase::StateManager::State_NoGame)
                onExitConfirmed();
            else
            {
                ConfirmationDialog* dialog = winMgr->getConfirmationDialog();
                dialog->askForConfirmation("#{sMessage2}");
                dialog->eventOkClicked.clear();
                dialog->eventOkClicked += MyGUI::newDelegate(this, &MainMenu::onExitConfirmed);
                dialog->eventCancelClicked.clear();
            }
        }
        else if (name == "newgame")
        {
            if (MWBase::Environment::get().getStateManager()->getState() == MWBase::StateManager::State_NoGame)
                onNewGameConfirmed();
            else
            {
                ConfirmationDialog* dialog = winMgr->getConfirmationDialog();
                dialog->askForConfirmation("#{sNotifyMessage54}");
                dialog->eventOkClicked.clear();
                dialog->eventOkClicked += MyGUI::newDelegate(this, &MainMenu::onNewGameConfirmed);
                dialog->eventCancelClicked.clear();
            }
        }

        else
        {
            if (!mSaveGameDialog)
                mSaveGameDialog = new SaveGameDialog();
            if (name == "loadgame")
                mSaveGameDialog->setLoadOrSave(true);
            else if (name == "savegame")
                mSaveGameDialog->setLoadOrSave(false);
            mSaveGameDialog->setVisible(true);
        }
    }

    void MainMenu::showBackground(bool show)
    {
        if (mVideo && !show)
        {
            MyGUI::Gui::getInstance().destroyWidget(mVideoBackground);
            mVideoBackground = nullptr;
            mVideo = nullptr;
        }
        if (mBackground && !show)
        {
            MyGUI::Gui::getInstance().destroyWidget(mBackground);
            mBackground = nullptr;
        }

        if (!show)
            return;

        bool stretch = Settings::Manager::getBool("stretch menu background", "GUI");

        if (mHasAnimatedMenu)
        {
            if (!mVideo)
            {
                // Use black background to correct aspect ratio
                mVideoBackground = MyGUI::Gui::getInstance().createWidgetReal<MyGUI::ImageBox>("ImageBox", 0,0,1,1,
                    MyGUI::Align::Default, "Menu");
                mVideoBackground->setImageTexture("black");

                mVideo = mVideoBackground->createWidget<VideoWidget>("ImageBox", 0,0,1,1,
                    MyGUI::Align::Stretch, "Menu");
                mVideo->setVFS(mVFS);

                mVideo->playVideo("video\\menu_background.bik");
            }

            MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
            int screenWidth = viewSize.width;
            int screenHeight = viewSize.height;
            mVideoBackground->setSize(screenWidth, screenHeight);

            mVideo->autoResize(stretch);

            mVideo->setVisible(true);
        }
        else
        {
            if (!mBackground)
            {
                mBackground = MyGUI::Gui::getInstance().createWidgetReal<BackgroundImage>("ImageBox", 0,0,1,1,
                    MyGUI::Align::Stretch, "Menu");
                mBackground->setBackgroundImage("textures\\menu_morrowind.dds", true, stretch);
            }
            mBackground->setVisible(true);
        }
    }

    void MainMenu::onFrame(float dt)
    {
        if (mVideo)
        {
            if (!mVideo->update())
            {
                // If finished playing, start again
                mVideo->playVideo("video\\menu_background.bik");
            }
        }
    }

    bool MainMenu::exit()
    {
        return MWBase::Environment::get().getStateManager()->getState() == MWBase::StateManager::State_Running;
    }

    void MainMenu::updateMenu()
    {
        setCoord(0,0, mWidth, mHeight);

        if (!mButtonBox)
            mButtonBox = mMainWidget->createWidget<MyGUI::Widget>("", MyGUI::IntCoord(0, 0, 0, 0), MyGUI::Align::Default);

        int curH = 0;

        MWBase::StateManager::State state = MWBase::Environment::get().getStateManager()->getState();

        mVersionText->setVisible(state == MWBase::StateManager::State_NoGame);

        std::vector<std::string> buttons;

        if (state==MWBase::StateManager::State_Running)
            buttons.push_back("return");

        buttons.push_back("newgame");

        if (state==MWBase::StateManager::State_Running &&
            MWBase::Environment::get().getWorld()->getGlobalInt ("chargenstate")==-1 &&
                MWBase::Environment::get().getWindowManager()->isSavingAllowed())
            buttons.push_back("savegame");

        if (MWBase::Environment::get().getStateManager()->characterBegin()!=
            MWBase::Environment::get().getStateManager()->characterEnd())
            buttons.push_back("loadgame");

        buttons.push_back("options");

        if (state==MWBase::StateManager::State_NoGame)
            buttons.push_back("credits");

        buttons.push_back("exitgame");

        // Create new buttons if needed
        std::vector<std::string> allButtons { "return", "newgame", "savegame", "loadgame", "options", "credits", "exitgame"};
        for (std::string& buttonId : allButtons)
        {
            if (mButtons.find(buttonId) == mButtons.end())
            {
                Gui::ImageButton* button = mButtonBox->createWidget<Gui::ImageButton>
                        ("ImageBox", MyGUI::IntCoord(0, curH, 0, 0), MyGUI::Align::Default);
                button->setProperty("ImageHighlighted", "textures\\menu_" + buttonId + "_over.dds");
                button->setProperty("ImageNormal", "textures\\menu_" + buttonId + ".dds");
                button->setProperty("ImagePushed", "textures\\menu_" + buttonId + "_pressed.dds");
                button->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenu::onButtonClicked);
                button->setUserData(std::string(buttonId));
                mButtons[buttonId] = button;
            }
        }

        // Start by hiding all buttons
        int maxwidth = 0;
        for (auto& buttonPair : mButtons)
        {
            buttonPair.second->setVisible(false);
            MyGUI::IntSize requested = buttonPair.second->getRequestedSize();
            if (requested.width > maxwidth)
                maxwidth = requested.width;
        }

        // Now show and position the ones we want
        for (std::string& buttonId : buttons)
        {
            assert(mButtons.find(buttonId) != mButtons.end());
            Gui::ImageButton* button = mButtons[buttonId];
            button->setVisible(true);

            // By default, assume that all menu buttons textures should have 64 height.
            // If they have a different resolution, scale them.
            MyGUI::IntSize requested = button->getRequestedSize();
            float scale = requested.height / 64.f;

            button->setImageCoord(MyGUI::IntCoord(0, 0, requested.width, requested.height));
            // Trim off some of the excessive padding
            // TODO: perhaps do this within ImageButton?
            int height = requested.height;
            button->setImageTile(MyGUI::IntSize(requested.width, requested.height-16*scale));
            button->setCoord((maxwidth-requested.width/scale) / 2, curH, requested.width/scale, height/scale-16);
            curH += height/scale-16;
        }

        if (state == MWBase::StateManager::State_NoGame)
        {
            // Align with the background image
            int bottomPadding=24;
            mButtonBox->setCoord (mWidth/2 - maxwidth/2, mHeight - curH - bottomPadding, maxwidth, curH);
        }
        else
            mButtonBox->setCoord (mWidth/2 - maxwidth/2, mHeight/2 - curH/2, maxwidth, curH);

    }
}
