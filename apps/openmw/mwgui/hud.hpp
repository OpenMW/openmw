#include "map_window.hpp"

#include <openengine/gui/layout.hpp>

#include "../mwmechanics/stat.hpp"
#include "../mwworld/ptr.hpp"

namespace MWGui
{
    class DragAndDrop;

    class HUD : public OEngine::GUI::Layout, public LocalMapBase
    {
    public:
        HUD(int width, int height, int fpsLevel, DragAndDrop* dragAndDrop);
        void setEffect(const char *img);
        void setValue (const std::string& id, const MWMechanics::DynamicStat<int>& value);
        void setFPS(float fps);
        void setTriangleCount(unsigned int count);
        void setBatchCount(unsigned int count);
        void setBottomLeftVisibility(bool hmsVisible, bool weapVisible, bool spellVisible);
        void setBottomRightVisibility(bool effectBoxVisible, bool minimapVisible);
        void setFpsLevel(const int level);

        void setSelectedSpell(const std::string& spellId, int successChancePercent);
        void setSelectedEnchantItem(const MWWorld::Ptr& item, int chargePercent);
        void setSelectedWeapon(const MWWorld::Ptr& item, int durabilityPercent);
        void unsetSelectedSpell();
        void unsetSelectedWeapon();

        void onFrame(float dt);
        void onResChange(int width, int height);

        void setCellName(const std::string& cellName);

        bool getWorldMouseOver() { return mWorldMouseOver; }

        MyGUI::ProgressPtr health, magicka, stamina;
        MyGUI::Widget* mHealthFrame;
        MyGUI::Widget *weapBox, *spellBox;
        MyGUI::ImageBox *weapImage, *spellImage;
        MyGUI::ProgressPtr weapStatus, spellStatus;
        MyGUI::Widget *effectBox, *minimapBox;
        MyGUI::ImageBox* effect1;
        MyGUI::ScrollView* minimap;
        MyGUI::ImageBox* compass;
        MyGUI::ImageBox* crosshair;
        MyGUI::TextBox* mCellNameBox;
        MyGUI::TextBox* mWeaponSpellBox;

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

        std::string mWeaponName;
        std::string mSpellName;
        float mWeaponSpellTimer;

        bool mMapVisible;
        bool mWeaponVisible;
        bool mSpellVisible;

        bool mWorldMouseOver;

        void onWorldClicked(MyGUI::Widget* _sender);
        void onWorldMouseOver(MyGUI::Widget* _sender, int x, int y);
        void onWorldMouseLostFocus(MyGUI::Widget* _sender, MyGUI::Widget* _new);
        void onHMSClicked(MyGUI::Widget* _sender);
        void onWeaponClicked(MyGUI::Widget* _sender);
        void onMagicClicked(MyGUI::Widget* _sender);
        void onMapClicked(MyGUI::Widget* _sender);
    };
}
