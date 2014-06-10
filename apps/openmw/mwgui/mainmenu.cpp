#include "mainmenu.hpp"

#include <OgreResourceGroupManager.h>

#include <components/version/version.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/statemanager.hpp"

#include "../mwstate/character.hpp"

#include "savegamedialog.hpp"
#include "confirmationdialog.hpp"
#include "imagebutton.hpp"
#include "backgroundimage.hpp"
#include "videowidget.hpp"

namespace MWGui
{

    MainMenu::MainMenu(int w, int h)
        : OEngine::GUI::Layout("openmw_mainmenu.layout")
        , mButtonBox(0), mWidth (w), mHeight (h)
        , mSaveGameDialog(NULL)
        , mBackground(NULL)
        , mVideo(NULL)
    {
        getWidget(mVersionText, "VersionText");
        std::stringstream sstream;
        sstream << "OpenMW Version: " << OPENMW_VERSION;

        // adding info about git hash if available
        std::string rev = OPENMW_VERSION_COMMITHASH;
        std::string tag = OPENMW_VERSION_TAGHASH;
        if (!rev.empty() && !tag.empty())
        {
                rev = rev.substr(0,10);
                sstream << "\nRevision: " <<  rev;
        }
        
        std::string output = sstream.str();
        mVersionText->setCaption(output);

        mHasAnimatedMenu = (Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup("video\\menu_background.bik"));

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

        showBackground(
            MWBase::Environment::get().getWindowManager()->containsMode(MWGui::GM_MainMenu) &&
            MWBase::Environment::get().getStateManager()->getState() == MWBase::StateManager::State_NoGame);

        OEngine::GUI::Layout::setVisible (visible);
    }

    void MainMenu::onNewGameConfirmed()
    {
        MWBase::Environment::get().getStateManager()->newGame();
    }

    void MainMenu::onExitConfirmed()
    {
        MWBase::Environment::get().getStateManager()->requestQuit();
    }

    void MainMenu::onButtonClicked(MyGUI::Widget *sender)
    {
        std::string name = *sender->getUserData<std::string>();
        MWBase::Environment::get().getSoundManager()->playSound("Menu Click", 1.f, 1.f);
        if (name == "return")
        {
            MWBase::Environment::get().getSoundManager ()->resumeSounds (MWBase::SoundManager::Play_TypeSfx);
            MWBase::Environment::get().getWindowManager ()->removeGuiMode (GM_MainMenu);
        }
        else if (name == "options")
            MWBase::Environment::get().getWindowManager ()->pushGuiMode (GM_Settings);
        else if (name == "credits")
            MWBase::Environment::get().getWindowManager()->playVideo("mw_credits.bik", true);
        else if (name == "exitgame")
        {
            if (MWBase::Environment::get().getStateManager()->getState() == MWBase::StateManager::State_NoGame)
                onExitConfirmed();
            else
            {
                ConfirmationDialog* dialog = MWBase::Environment::get().getWindowManager()->getConfirmationDialog();
                dialog->open("#{sMessage2}");
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
                ConfirmationDialog* dialog = MWBase::Environment::get().getWindowManager()->getConfirmationDialog();
                dialog->open("#{sNotifyMessage54}");
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
            mVideoBackground = NULL;
            mVideo = NULL;
        }
        if (mBackground && !show)
        {
            MyGUI::Gui::getInstance().destroyWidget(mBackground);
            mBackground = NULL;
        }

        if (!show)
            return;

        if (mHasAnimatedMenu)
        {
            if (!mVideo)
            {
                // Use black background to correct aspect ratio
                mVideoBackground = MyGUI::Gui::getInstance().createWidgetReal<MyGUI::ImageBox>("ImageBox", 0,0,1,1,
                    MyGUI::Align::Default, "Menu");
                mVideoBackground->setImageTexture("black.png");

                mVideo = mVideoBackground->createWidget<VideoWidget>("ImageBox", 0,0,1,1,
                    MyGUI::Align::Stretch, "Menu");

                mVideo->playVideo("video\\menu_background.bik");
            }

            MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
            int screenWidth = viewSize.width;
            int screenHeight = viewSize.height;
            mVideoBackground->setSize(screenWidth, screenHeight);

            double imageaspect = static_cast<double>(mVideo->getVideoWidth())/mVideo->getVideoHeight();

            int leftPadding = std::max(0.0, (screenWidth - screenHeight * imageaspect) / 2);
            int topPadding = std::max(0.0, (screenHeight - screenWidth / imageaspect) / 2);

            mVideo->setCoord(leftPadding, topPadding,
                                   screenWidth - leftPadding*2, screenHeight - topPadding*2);

            mVideo->setVisible(true);
        }
        else
        {
            if (!mBackground)
            {
                mBackground = MyGUI::Gui::getInstance().createWidgetReal<BackgroundImage>("ImageBox", 0,0,1,1,
                    MyGUI::Align::Stretch, "Menu");
                mBackground->setBackgroundImage("textures\\menu_morrowind.dds");
            }
            mBackground->setVisible(true);
        }
    }

    void MainMenu::update(float dt)
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
        for (std::vector<std::string>::iterator it = buttons.begin(); it != buttons.end(); ++it)
        {
            if (mButtons.find(*it) == mButtons.end())
            {
                MWGui::ImageButton* button = mButtonBox->createWidget<MWGui::ImageButton>
                        ("ImageBox", MyGUI::IntCoord(0, curH, 0, 0), MyGUI::Align::Default);
                button->setProperty("ImageHighlighted", "textures\\menu_" + *it + "_over.dds");
                button->setProperty("ImageNormal", "textures\\menu_" + *it + ".dds");
                button->setProperty("ImagePushed", "textures\\menu_" + *it + "_pressed.dds");
                button->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenu::onButtonClicked);
                button->setUserData(std::string(*it));
                mButtons[*it] = button;
            }
        }

        // Start by hiding all buttons
        int maxwidth = 0;
        for (std::map<std::string, MWGui::ImageButton*>::iterator it = mButtons.begin(); it != mButtons.end(); ++it)
        {
            it->second->setVisible(false);
            MyGUI::IntSize requested = it->second->getRequestedSize();
            if (requested.width > maxwidth)
                maxwidth = requested.width;
        }

        // Now show and position the ones we want
        for (std::vector<std::string>::iterator it = buttons.begin(); it != buttons.end(); ++it)
        {
            assert(mButtons.find(*it) != mButtons.end());
            MWGui::ImageButton* button = mButtons[*it];
            button->setVisible(true);

            MyGUI::IntSize requested = button->getRequestedSize();

            // Trim off some of the excessive padding
            // TODO: perhaps do this within ImageButton?
            int trim = 8;
            button->setImageCoord(MyGUI::IntCoord(0, trim, requested.width, requested.height-trim));
            int height = requested.height-trim*2;
            button->setImageTile(MyGUI::IntSize(requested.width, height));
            button->setCoord((maxwidth-requested.width) / 2, curH, requested.width, height);
            curH += height;
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
