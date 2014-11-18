#ifndef OPENMW_GAME_MWGUI_MAINMENU_H
#define OPENMW_GAME_MWGUI_MAINMENU_H

#include <openengine/gui/layout.hpp>

namespace Gui
{
    class ImageButton;
}

namespace MWGui
{

    class BackgroundImage;
    class SaveGameDialog;
    class VideoWidget;

    class MainMenu : public OEngine::GUI::Layout
    {
            int mWidth;
            int mHeight;

            bool mHasAnimatedMenu;

        public:

            MainMenu(int w, int h);
            ~MainMenu();

            void onResChange(int w, int h);

            virtual void setVisible (bool visible);

            void update(float dt);

        private:

            MyGUI::Widget* mButtonBox;
            MyGUI::TextBox* mVersionText;

            BackgroundImage* mBackground;

            MyGUI::ImageBox* mVideoBackground;
            VideoWidget* mVideo; // For animated main menus

            std::map<std::string, Gui::ImageButton*> mButtons;

            void onButtonClicked (MyGUI::Widget* sender);
            void onNewGameConfirmed();
            void onExitConfirmed();

            void showBackground(bool show);

            void updateMenu();

            SaveGameDialog* mSaveGameDialog;
    };

}

#endif
