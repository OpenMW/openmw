#ifndef OPENMW_GAME_MWGUI_HUD_H
#define OPENMW_GAME_MWGUI_HUD_H

#include "mapwindow.hpp"

#include "../mwmechanics/stat.hpp"
#include "../mwworld/ptr.hpp"

namespace MWGui
{
    class DragAndDrop;
    class SpellIcons;

    class HUD : public OEngine::GUI::Layout, public LocalMapBase
    {
    public:
        HUD(int width, int height, int fpsLevel, DragAndDrop* dragAndDrop);
        virtual ~HUD();
        void setValue (const std::string& id, const MWMechanics::DynamicStat<float>& value);
        void setFPS(float fps);
        void setTriangleCount(unsigned int count);
        void setBatchCount(unsigned int count);

        /// Set time left for the player to start drowning
        /// @param time value from [0,20]
        void setDrowningTimeLeft(float time);
        void setDrowningBarVisible(bool visible);

        void setHmsVisible(bool visible);
        void setWeapVisible(bool visible);
        void setSpellVisible(bool visible);
        void setSneakVisible(bool visible);

        void setEffectVisible(bool visible);
        void setMinimapVisible(bool visible);

        void setFpsLevel(const int level);

        void setSelectedSpell(const std::string& spellId, int successChancePercent);
        void setSelectedEnchantItem(const MWWorld::Ptr& item, int chargePercent);
        void setSelectedWeapon(const MWWorld::Ptr& item, int durabilityPercent);
        void unsetSelectedSpell();
        void unsetSelectedWeapon();

        void setCrosshairVisible(bool visible);

        void onFrame(float dt);
        void onResChange(int width, int height);

        void setCellName(const std::string& cellName);

        bool getWorldMouseOver() { return mWorldMouseOver; }

        MyGUI::Widget* getEffectBox() { return mEffectBox; }

        void update();

        void setEnemy(const MWWorld::Ptr& enemy);

    private:
        MyGUI::ProgressBar *mHealth, *mMagicka, *mStamina, *mEnemyHealth, *mDrowning;
        MyGUI::Widget* mHealthFrame;
        MyGUI::Widget *mWeapBox, *mSpellBox, *mSneakBox;
        MyGUI::ImageBox *mWeapImage, *mSpellImage;
        MyGUI::ProgressBar *mWeapStatus, *mSpellStatus;
        MyGUI::Widget *mEffectBox, *mMinimapBox;
        MyGUI::Button* mMinimapButton;
        MyGUI::ScrollView* mMinimap;
        MyGUI::ImageBox* mCompass;
        MyGUI::ImageBox* mCrosshair;
        MyGUI::TextBox* mCellNameBox;
        MyGUI::TextBox* mWeaponSpellBox;
        MyGUI::Widget *mDrowningFrame, *mDrowningFlash;

        MyGUI::Widget* mDummy;

        MyGUI::Widget* mFpsBox;
        MyGUI::TextBox* mFpsCounter;
        MyGUI::TextBox* mTriangleCounter;
        MyGUI::TextBox* mBatchCounter;

        // bottom left elements
        int mHealthManaStaminaBaseLeft, mWeapBoxBaseLeft, mSpellBoxBaseLeft, mSneakBoxBaseLeft;
        // bottom right elements
        int mMinimapBoxBaseRight, mEffectBoxBaseRight;

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

        SpellIcons* mSpellIcons;

        MWWorld::Ptr mEnemy;
        float mEnemyHealthTimer;

        bool  mIsDrowning;
        float mDrowningFlashTheta;

        void onWorldClicked(MyGUI::Widget* _sender);
        void onWorldMouseOver(MyGUI::Widget* _sender, int x, int y);
        void onWorldMouseLostFocus(MyGUI::Widget* _sender, MyGUI::Widget* _new);
        void onHMSClicked(MyGUI::Widget* _sender);
        void onWeaponClicked(MyGUI::Widget* _sender);
        void onMagicClicked(MyGUI::Widget* _sender);
        void onMapClicked(MyGUI::Widget* _sender);

        void updateEnemyHealthBar();

        void updatePositions();
    };
}

#endif
