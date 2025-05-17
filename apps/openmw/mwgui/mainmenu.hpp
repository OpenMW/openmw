#ifndef OPENMW_GAME_MWGUI_MAINMENU_H
#define OPENMW_GAME_MWGUI_MAINMENU_H

#include <memory>
#include <optional>
#include <thread>

#include "savegamedialog.hpp"
#include "windowbase.hpp"

namespace Gui
{
    class ImageButton;
}

namespace VFS
{
    class Manager;
}

namespace MWGui
{

    class BackgroundImage;
    class VideoWidget;
    class MenuVideo
    {
        MyGUI::ImageBox* mVideoBackground;
        VideoWidget* mVideo;
        std::thread mThread;
        bool mRunning;

        void run();

    public:
        MenuVideo(const VFS::Manager* vfs);
        void resize(int w, int h);
        ~MenuVideo();
    };

    class MainMenu : public WindowBase
    {
        int mWidth;
        int mHeight;

        bool mHasAnimatedMenu;

    public:
        MainMenu(int w, int h, const VFS::Manager* vfs, const std::string& versionDescription);

        void onResChange(int w, int h) override;
        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;

        void setVisible(bool visible) override;

        bool exit() override;

        std::string getButtonStr() override;

    private:
        const VFS::Manager* mVFS;

        MyGUI::Widget* mButtonBox;
        MyGUI::TextBox* mVersionText;

        BackgroundImage* mBackground;

        std::optional<MenuVideo> mVideo; // For animated main menus

        std::map<std::string, Gui::ImageButton*, std::less<>> mButtons;

        void onButtonClicked(MyGUI::Widget* sender);
        void onNewGameConfirmed();
        void onExitConfirmed();

        void showBackground(bool show);

        void updateMenu();

        std::unique_ptr<SaveGameDialog> mSaveGameDialog;
    };

}

#endif
