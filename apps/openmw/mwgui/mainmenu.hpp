#include <openengine/gui/layout.hpp>

namespace MWGui
{

    class MainMenu : public OEngine::GUI::Layout
    {
    public:
        MainMenu(int w, int h);

    private:
        MyGUI::Button* mReturn;
        MyGUI::Button* mNewGame;
        MyGUI::Button* mLoadGame;
        MyGUI::Button* mSaveGame;
        MyGUI::Button* mOptions;
        MyGUI::Button* mCredits;
        MyGUI::Button* mExitGame;

        MyGUI::Widget* mButtonBox;

        void returnToGame(MyGUI::Widget* sender);
        void showOptions(MyGUI::Widget* sender);
        void exitGame(MyGUI::Widget* sender);
    };

}
