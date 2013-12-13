#include <openengine/gui/layout.hpp>

#include "imagebutton.hpp"

namespace MWGui
{

    class MainMenu : public OEngine::GUI::Layout
    {
    public:
        MainMenu(int w, int h);

        void onResChange(int w, int h);

        void setNoReturn(bool bNoReturn);

    private:
        MyGUI::Widget* mButtonBox;

        bool mNoReturn;

        std::map<std::string, MWGui::ImageButton*> mButtons;

        void onButtonClicked (MyGUI::Widget* sender);
    };

}
