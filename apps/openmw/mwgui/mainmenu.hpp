#include <openengine/gui/layout.hpp>

#include "imagebutton.hpp"

namespace MWGui
{

    class MainMenu : public OEngine::GUI::Layout
    {
    public:
        MainMenu(int w, int h);

        void onResChange(int w, int h);

    private:
        MyGUI::Widget* mButtonBox;

        std::map<std::string, MWGui::ImageButton*> mButtons;

        void onButtonClicked (MyGUI::Widget* sender);
    };

}
