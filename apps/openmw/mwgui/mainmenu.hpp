#include <openengine/gui/layout.hpp>

namespace MWGui
{

    class MainMenu : public OEngine::GUI::Layout
    {
    public:
        MainMenu(int w, int h)
        : Layout("openmw_mainmenu.layout")
        {
            setCoord(0,0,w,h);
        }
    };

}
