#include "map_window.hpp"

#include <openengine/gui/layout.hpp>

#include "../mwmechanics/stat.hpp"

namespace MWGui
{
    class DragAndDrop;

    class HUD : public OEngine::GUI::Layout, public LocalMapBase
    {
    public:
        HUD(int width, int height, int fpsLevel, DragAndDrop* dragAndDrop);
        void setWeapIcon(const char *str);
        void setSpellIcon(const char *str);
        void setWeapStatus(int s, int smax);
        void setSpellStatus(int s, int smax);
        void setEffect(const char *img);
        void setValue (const std::string& id, const MWMechanics::DynamicStat<int>& value);
        void setFPS(float fps);
        void setTriangleCount(size_t count);
        void setBatchCount(size_t count);
        void setBottomLeftVisibility(bool hmsVisible, bool weapVisible, bool spellVisible);
        void setBottomRightVisibility(bool effectBoxVisible, bool minimapVisible);
        void setFpsLevel(const int level);

        void onFrame(float dt);

        void setCellName(const std::string& cellName);

        MyGUI::ProgressPtr health, magicka, stamina;
        MyGUI::Widget *weapBox, *spellBox;
        MyGUI::ImageBox *weapImage, *spellImage;
        MyGUI::ProgressPtr weapStatus, spellStatus;
        MyGUI::Widget *effectBox, *minimapBox;
        MyGUI::ImageBox* effect1;
        MyGUI::ScrollView* minimap;
        MyGUI::ImageBox* compass;
        MyGUI::ImageBox* crosshair;
        MyGUI::TextBox* mCellNameBox;

        MyGUI::WidgetPtr fpsbox;
        MyGUI::TextBox* fpscounter;
        MyGUI::TextBox* trianglecounter;
        MyGUI::TextBox* batchcounter;

    private:
        // bottom left elements
        int hmsBaseLeft, weapBoxBaseLeft, spellBoxBaseLeft;
        // bottom right elements
        int minimapBoxBaseRight, effectBoxBaseRight;

        DragAndDrop* mDragAndDrop;

        std::string mCellName;
        float mCellNameTimer;

        void onWorldClicked(MyGUI::Widget* _sender);
        void onWorldMouseOver(MyGUI::Widget* _sender, int x, int y);
        void onWorldMouseLostFocus(MyGUI::Widget* _sender, MyGUI::Widget* _new);
        void onHMSClicked(MyGUI::Widget* _sender);
        void onWeaponClicked(MyGUI::Widget* _sender);
        void onMagicClicked(MyGUI::Widget* _sender);
        void onMapClicked(MyGUI::Widget* _sender);
    };
}
