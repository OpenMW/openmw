#include <openengine/gui/layout.hpp>

#include "imagebutton.hpp"

namespace MWGui
{

    class SaveGameDialog;

    class MainMenu : public OEngine::GUI::Layout
    {
            int mWidth;
            int mHeight;

        public:

            MainMenu(int w, int h);
            ~MainMenu();

            void onResChange(int w, int h);

            virtual void setVisible (bool visible);

        private:

            MyGUI::Widget* mButtonBox;

            std::map<std::string, MWGui::ImageButton*> mButtons;

            void onButtonClicked (MyGUI::Widget* sender);

            void updateMenu();

            SaveGameDialog* mSaveGameDialog;
    };

}
