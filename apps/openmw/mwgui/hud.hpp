#ifndef OPENMW_GAME_MWGUI_HUD_H
#define OPENMW_GAME_MWGUI_HUD_H

#include <memory>

#include "mapwindow.hpp"
#include "spellicons.hpp"
#include "statswatcher.hpp"

namespace MWWorld
{
    class Ptr;
}

namespace MWGui
{
    class DragAndDrop;
    class ItemWidget;
    class SpellWidget;

    class HUD : public WindowBase, public LocalMapBase, public StatsListener
    {
    public:
        HUD(CustomMarkerCollection& customMarkers, DragAndDrop* dragAndDrop, MWRender::LocalMap* localMapRender);
        virtual ~HUD();
        void setValue(std::string_view id, const MWMechanics::DynamicStat<float>& value) override;

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

        void setSelectedSpell(const ESM::RefId& spellId, int successChancePercent);
        void setSelectedEnchantItem(const MWWorld::Ptr& item, int chargePercent);
        const MWWorld::Ptr& getSelectedEnchantItem();
        void setSelectedWeapon(const MWWorld::Ptr& item, int durabilityPercent);
        void unsetSelectedSpell();
        void unsetSelectedWeapon();

        void setCrosshairVisible(bool visible);
        void setCrosshairOwned(bool owned);

        void onFrame(float dt) override;

        void setCellName(const std::string& cellName);

        bool getWorldMouseOver() { return mWorldMouseOver; }

        MyGUI::Widget* getEffectBox() { return mEffectBox; }

        void setEnemy(const MWWorld::Ptr& enemy);

        void clear() override;

        void dropDraggedItem(float mouseX, float mouseY);

    private:
        MyGUI::ProgressBar *mHealth = nullptr, *mMagicka = nullptr, *mStamina = nullptr, *mEnemyHealth = nullptr,
                           *mDrowning = nullptr;
        MyGUI::Widget* mHealthFrame = nullptr;
        MyGUI::Widget *mWeapBox = nullptr, *mSpellBox = nullptr, *mSneakBox = nullptr;
        ItemWidget* mWeapImage = nullptr;
        SpellWidget* mSpellImage = nullptr;
        MyGUI::ProgressBar *mWeapStatus = nullptr, *mSpellStatus = nullptr;
        MyGUI::Widget *mEffectBox = nullptr, *mMinimapBox = nullptr;
        MyGUI::Button* mMinimapButton = nullptr;
        MyGUI::ScrollView* mMinimap = nullptr;
        MyGUI::ImageBox* mCrosshair = nullptr;
        MyGUI::TextBox* mCellNameBox = nullptr;
        MyGUI::TextBox* mWeaponSpellBox = nullptr;
        MyGUI::Widget *mDrowningBar = nullptr, *mDrowningFrame = nullptr, *mDrowningFlash = nullptr;
        DragAndDrop* mDragAndDrop;
        std::string mCellName;
        std::string mWeaponName;
        std::string mSpellName;
        std::unique_ptr<SpellIcons> mSpellIcons;
        ESM::RefNum mEnemyActor;

        // bottom left elements
        int mHealthManaStaminaBaseLeft, mWeapBoxBaseLeft, mSpellBoxBaseLeft, mSneakBoxBaseLeft;
        // bottom right elements
        int mMinimapBoxBaseRight, mEffectBoxBaseRight;

        float mCellNameTimer = 0.f;
        float mWeaponSpellTimer = 0.f;
        float mEnemyHealthTimer = -1;
        float mDrowningFlashTheta = 0.f;

        bool mMapVisible = true;
        bool mWeaponVisible = true;
        bool mSpellVisible = true;
        bool mWorldMouseOver = false;
        bool mIsDrowning = false;

        void onWorldClicked(MyGUI::Widget* sender);
        void onWorldMouseOver(MyGUI::Widget* sender, int x, int y);
        void onWorldMouseLostFocus(MyGUI::Widget* sender, MyGUI::Widget* newWidget);
        void onHMSClicked(MyGUI::Widget* sender);
        void onWeaponClicked(MyGUI::Widget* sender);
        void onMagicClicked(MyGUI::Widget* sender);
        void onMapClicked(MyGUI::Widget* sender);

        // LocalMapBase
        void customMarkerCreated(MyGUI::Widget* marker) override;
        void doorMarkerCreated(MyGUI::Widget* marker) override;

        void updateEnemyHealthBar();

        void updatePositions();
    };
}

#endif
