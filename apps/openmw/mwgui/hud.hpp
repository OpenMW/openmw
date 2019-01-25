#ifndef OPENMW_GAME_MWGUI_HUD_H
#define OPENMW_GAME_MWGUI_HUD_H

#include "mapwindow.hpp"

#include "../mwmechanics/stat.hpp"

namespace MWWorld
{
    class Ptr;
}

namespace MWGui
{
    class DragAndDrop;
    class SpellIcons;
    class ItemWidget;
    class SpellWidget;

    class HUD : public WindowBase, public LocalMapBase
    {
    public:
        HUD(CustomMarkerCollection& customMarkers, DragAndDrop* dragAndDrop, MWRender::LocalMap* localMapRender);
        virtual ~HUD();
        void setValue (const std::string& id, const MWMechanics::DynamicStat<float>& value);

        /// Set time left for the player to start drowning
        /// @param time time left to start drowning
        /// @param maxTime how long we can be underwater (in total) until drowning starts
        void setDrowningTimeLeft(float time, float maxTime);
        void setDrowningBarVisible(bool visible);

        void setHmsVisible(bool visible);
        void setWeapVisible(bool visible);
        void setSpellVisible(bool visible);
        void setSneakVisible(bool visible);

        void setEffectVisible(bool visible);
        void setMinimapVisible(bool visible);

        void setSelectedSpell(const std::string& spellId, int successChancePercent);
        void setSelectedEnchantItem(const MWWorld::Ptr& item, int chargePercent);
        const MWWorld::Ptr& getSelectedEnchantItem();
        void setSelectedWeapon(const MWWorld::Ptr& item, int durabilityPercent);
        void unsetSelectedSpell();
        void unsetSelectedWeapon();

        void setCrosshairVisible(bool visible);
        void setCrosshairOwned(bool owned);

        void onFrame(float dt);

        void setCellName(const std::string& cellName);

        bool getWorldMouseOver() { return mWorldMouseOver; }

        MyGUI::Widget* getEffectBox() { return mEffectBox; }

        void setEnemy(const MWWorld::Ptr& enemy);
        void resetEnemy();

        void clear();

    private:
        MyGUI::ProgressBar *mHealth, *mMagicka, *mStamina, *mEnemyHealth, *mDrowning;
        MyGUI::Widget* mHealthFrame;
        MyGUI::Widget *mWeapBox, *mSpellBox, *mSneakBox;
        ItemWidget *mWeapImage;
        SpellWidget *mSpellImage;
        MyGUI::ProgressBar *mWeapStatus, *mSpellStatus;
        MyGUI::Widget *mEffectBox, *mMinimapBox;
        MyGUI::Button* mMinimapButton;
        MyGUI::ScrollView* mMinimap;
        MyGUI::ImageBox* mCrosshair;
        MyGUI::TextBox* mCellNameBox;
        MyGUI::TextBox* mWeaponSpellBox;
        MyGUI::Widget *mDrowningFrame, *mDrowningFlash;

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

        int mEnemyActorId;
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

        // LocalMapBase
        virtual void customMarkerCreated(MyGUI::Widget* marker);
        virtual void doorMarkerCreated(MyGUI::Widget* marker);

        void updateEnemyHealthBar();

        void updatePositions();
    };
}

#endif
