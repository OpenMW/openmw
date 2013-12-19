#include "hud.hpp"

#include <boost/lexical_cast.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "inventorywindow.hpp"
#include "console.hpp"
#include "spellicons.hpp"
#include "itemmodel.hpp"
#include "container.hpp"

namespace MWGui
{

    HUD::HUD(int width, int height, int fpsLevel, DragAndDrop* dragAndDrop)
        : Layout("openmw_hud.layout")
        , mHealth(NULL)
        , mMagicka(NULL)
        , mStamina(NULL)
        , mDrowning(NULL)
        , mDrowningFrame(NULL)
        , mDrowningFlash(NULL)
        , mWeapImage(NULL)
        , mSpellImage(NULL)
        , mWeapStatus(NULL)
        , mSpellStatus(NULL)
        , mEffectBox(NULL)
        , mMinimap(NULL)
        , mCompass(NULL)
        , mCrosshair(NULL)
        , mFpsBox(NULL)
        , mFpsCounter(NULL)
        , mTriangleCounter(NULL)
        , mBatchCounter(NULL)
        , mHealthManaStaminaBaseLeft(0)
        , mWeapBoxBaseLeft(0)
        , mSpellBoxBaseLeft(0)
        , mEffectBoxBaseRight(0)
        , mMinimapBoxBaseRight(0)
        , mDragAndDrop(dragAndDrop)
        , mCellNameTimer(0.0f)
        , mCellNameBox(NULL)
        , mMapVisible(true)
        , mWeaponVisible(true)
        , mSpellVisible(true)
        , mWorldMouseOver(false)
        , mEnemyHealthTimer(0)
        , mIsDrowning(false)
    {
        setCoord(0,0, width, height);

        // Energy bars
        getWidget(mHealthFrame, "HealthFrame");
        getWidget(mHealth, "Health");
        getWidget(mMagicka, "Magicka");
        getWidget(mStamina, "Stamina");
        getWidget(mEnemyHealth, "EnemyHealth");
        mHealthManaStaminaBaseLeft = mHealthFrame->getLeft();

        MyGUI::Widget *healthFrame, *magickaFrame, *fatigueFrame;
        getWidget(healthFrame, "HealthFrame");
        getWidget(magickaFrame, "MagickaFrame");
        getWidget(fatigueFrame, "FatigueFrame");
        healthFrame->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onHMSClicked);
        magickaFrame->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onHMSClicked);
        fatigueFrame->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onHMSClicked);

        //Drowning bar
        getWidget(mDrowningFrame, "DrowningFrame");
        getWidget(mDrowning, "Drowning");
        getWidget(mDrowningFlash, "Flash");
        mDrowning->setProgressRange(200);

        const MyGUI::IntSize& viewSize = MyGUI::RenderManager::getInstance().getViewSize();

        // Item and spell images and status bars
        getWidget(mWeapBox, "WeapBox");
        getWidget(mWeapImage, "WeapImage");
        getWidget(mWeapStatus, "WeapStatus");
        mWeapBoxBaseLeft = mWeapBox->getLeft();
        mWeapBox->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onWeaponClicked);

        getWidget(mSpellBox, "SpellBox");
        getWidget(mSpellImage, "SpellImage");
        getWidget(mSpellStatus, "SpellStatus");
        mSpellBoxBaseLeft = mSpellBox->getLeft();
        mSpellBox->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onMagicClicked);

        getWidget(mSneakBox, "SneakBox");
        mSneakBoxBaseLeft = mSneakBox->getLeft();

        getWidget(mEffectBox, "EffectBox");
        mEffectBoxBaseRight = viewSize.width - mEffectBox->getRight();

        getWidget(mMinimapBox, "MiniMapBox");
        mMinimapBoxBaseRight = viewSize.width - mMinimapBox->getRight();
        getWidget(mMinimap, "MiniMap");
        getWidget(mCompass, "Compass");
        getWidget(mMinimapButton, "MiniMapButton");
        mMinimapButton->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onMapClicked);

        getWidget(mCellNameBox, "CellName");
        getWidget(mWeaponSpellBox, "WeaponSpellName");

        getWidget(mCrosshair, "Crosshair");

        setFpsLevel(fpsLevel);

        getWidget(mTriangleCounter, "TriangleCounter");
        getWidget(mBatchCounter, "BatchCounter");

        LocalMapBase::init(mMinimap, mCompass, this);

        mMainWidget->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onWorldClicked);
        mMainWidget->eventMouseMove += MyGUI::newDelegate(this, &HUD::onWorldMouseOver);
        mMainWidget->eventMouseLostFocus += MyGUI::newDelegate(this, &HUD::onWorldMouseLostFocus);

        mSpellIcons = new SpellIcons();
    }

    HUD::~HUD()
    {
        delete mSpellIcons;
    }

    void HUD::setFpsLevel(int level)
    {
        mFpsCounter = 0;

        MyGUI::Widget* fps;
        getWidget(fps, "FPSBoxAdv");
        fps->setVisible(false);
        getWidget(fps, "FPSBox");
        fps->setVisible(false);

        if (level == 2)
        {
            getWidget(mFpsBox, "FPSBoxAdv");
            mFpsBox->setVisible(true);
            getWidget(mFpsCounter, "FPSCounterAdv");
        }
        else if (level == 1)
        {
            getWidget(mFpsBox, "FPSBox");
            mFpsBox->setVisible(true);
            getWidget(mFpsCounter, "FPSCounter");
        }
    }

    void HUD::setFPS(float fps)
    {
        if (mFpsCounter)
            mFpsCounter->setCaption(boost::lexical_cast<std::string>((int)fps));
    }

    void HUD::setTriangleCount(unsigned int count)
    {
        mTriangleCounter->setCaption(boost::lexical_cast<std::string>(count));
    }

    void HUD::setBatchCount(unsigned int count)
    {
        mBatchCounter->setCaption(boost::lexical_cast<std::string>(count));
    }

    void HUD::setValue(const std::string& id, const MWMechanics::DynamicStat<float>& value)
    {
        int current = std::max(0, static_cast<int>(value.getCurrent()));
        int modified = static_cast<int>(value.getModified());

        MyGUI::Widget* w;
        std::string valStr = boost::lexical_cast<std::string>(current) + "/" + boost::lexical_cast<std::string>(modified);
        if (id == "HBar")
        {
            mHealth->setProgressRange(modified);
            mHealth->setProgressPosition(current);
            getWidget(w, "HealthFrame");
            w->setUserString("Caption_HealthDescription", "#{sHealthDesc}\n" + valStr);
        }
        else if (id == "MBar")
        {
            mMagicka->setProgressRange (modified);
            mMagicka->setProgressPosition (current);
            getWidget(w, "MagickaFrame");
            w->setUserString("Caption_HealthDescription", "#{sIntDesc}\n" + valStr);
        }
        else if (id == "FBar")
        {
            mStamina->setProgressRange (modified);
            mStamina->setProgressPosition (current);
            getWidget(w, "FatigueFrame");
            w->setUserString("Caption_HealthDescription", "#{sFatDesc}\n" + valStr);
        }
    }

    void HUD::setDrowningTimeLeft(float time)
    {
        size_t progress = time/20.0*200.0;
        mDrowning->setProgressPosition(progress);

        bool isDrowning = (progress == 0);
        if (isDrowning && !mIsDrowning) // Just started drowning
            mDrowningFlashTheta = 0.0f; // Start out on bright red every time.

        mDrowningFlash->setVisible(isDrowning);
        mIsDrowning = isDrowning;
    }

    void HUD::setDrowningBarVisible(bool visible)
    {
        mDrowningFrame->setVisible(visible);
    }

    void HUD::onWorldClicked(MyGUI::Widget* _sender)
    {
        if (!MWBase::Environment::get().getWindowManager ()->isGuiMode ())
            return;

        if (mDragAndDrop->mIsOnDragAndDrop)
        {
            // drop item into the gameworld
            MWWorld::Ptr object = mDragAndDrop->mItem.mBase;

            MWBase::World* world = MWBase::Environment::get().getWorld();

            MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
            MyGUI::IntPoint cursorPosition = MyGUI::InputManager::getInstance().getMousePosition();
            float mouseX = cursorPosition.left / float(viewSize.width);
            float mouseY = cursorPosition.top / float(viewSize.height);

            if (world->canPlaceObject(mouseX, mouseY))
                world->placeObject(object, mouseX, mouseY, mDragAndDrop->mDraggedCount);
            else
                world->dropObjectOnGround(world->getPlayer().getPlayer(), object, mDragAndDrop->mDraggedCount);

            MWBase::Environment::get().getWindowManager()->changePointer("arrow");

            std::string sound = MWWorld::Class::get(object).getDownSoundId(object);
            MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);

            // remove object from the container it was coming from
            mDragAndDrop->mSourceModel->removeItem(mDragAndDrop->mItem, mDragAndDrop->mDraggedCount);
            mDragAndDrop->finish();
            mDragAndDrop->mSourceModel->update();
        }
        else
        {
            GuiMode mode = MWBase::Environment::get().getWindowManager()->getMode();

            if ( (mode != GM_Console) && (mode != GM_Container) && (mode != GM_Inventory) )
                return;

            MWWorld::Ptr object = MWBase::Environment::get().getWorld()->getFacedObject();

            if (mode == GM_Console)
                MWBase::Environment::get().getWindowManager()->getConsole()->setSelectedObject(object);
            else if ((mode == GM_Container) || (mode == GM_Inventory))
            {
                // pick up object
                MWBase::Environment::get().getWindowManager()->getInventoryWindow()->pickUpObject(object);
            }
        }
    }

    void HUD::onWorldMouseOver(MyGUI::Widget* _sender, int x, int y)
    {
        if (mDragAndDrop->mIsOnDragAndDrop)
        {
            mWorldMouseOver = false;

            MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
            MyGUI::IntPoint cursorPosition = MyGUI::InputManager::getInstance().getMousePosition();
            float mouseX = cursorPosition.left / float(viewSize.width);
            float mouseY = cursorPosition.top / float(viewSize.height);

            MWBase::World* world = MWBase::Environment::get().getWorld();

            // if we can't drop the object at the wanted position, show the "drop on ground" cursor.
            bool canDrop = world->canPlaceObject(mouseX, mouseY);

            if (!canDrop)
                MWBase::Environment::get().getWindowManager()->changePointer("drop_ground");
            else
                MWBase::Environment::get().getWindowManager()->changePointer("arrow");

        }
        else
        {
            MWBase::Environment::get().getWindowManager()->changePointer("arrow");
            mWorldMouseOver = true;
        }
    }

    void HUD::onWorldMouseLostFocus(MyGUI::Widget* _sender, MyGUI::Widget* _new)
    {
        MWBase::Environment::get().getWindowManager()->changePointer("arrow");
        mWorldMouseOver = false;
    }

    void HUD::onHMSClicked(MyGUI::Widget* _sender)
    {
        MWBase::Environment::get().getWindowManager()->toggleVisible(GW_Stats);
    }

    void HUD::onMapClicked(MyGUI::Widget* _sender)
    {
        MWBase::Environment::get().getWindowManager()->toggleVisible(GW_Map);
    }

    void HUD::onWeaponClicked(MyGUI::Widget* _sender)
    {
        const MWWorld::Ptr& player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        if (MWWorld::Class::get(player).getNpcStats(player).isWerewolf())
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sWerewolfRefusal}");
            return;
        }

        MWBase::Environment::get().getWindowManager()->toggleVisible(GW_Inventory);
    }

    void HUD::onMagicClicked(MyGUI::Widget* _sender)
    {
        const MWWorld::Ptr& player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        if (MWWorld::Class::get(player).getNpcStats(player).isWerewolf())
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sWerewolfRefusal}");
            return;
        }

        MWBase::Environment::get().getWindowManager()->toggleVisible(GW_Magic);
    }

    void HUD::setCellName(const std::string& cellName)
    {
        if (mCellName != cellName)
        {
            mCellNameTimer = 5.0f;
            mCellName = cellName;

            mCellNameBox->setCaptionWithReplacing("#{sCell=" + mCellName + "}");
            mCellNameBox->setVisible(mMapVisible);
        }
    }

    void HUD::onFrame(float dt)
    {
        mCellNameTimer -= dt;
        mWeaponSpellTimer -= dt;
        if (mCellNameTimer < 0)
            mCellNameBox->setVisible(false);
        if (mWeaponSpellTimer < 0)
            mWeaponSpellBox->setVisible(false);

        mEnemyHealthTimer -= dt;
        if (mEnemyHealth->getVisible() && mEnemyHealthTimer < 0)
        {
            mEnemyHealth->setVisible(false);
            mWeaponSpellBox->setPosition(mWeaponSpellBox->getPosition() + MyGUI::IntPoint(0,20));
        }

        if (mIsDrowning)
            mDrowningFlashTheta += dt * Ogre::Math::TWO_PI;
    }

    void HUD::onResChange(int width, int height)
    {
        setCoord(0, 0, width, height);
    }

    void HUD::setSelectedSpell(const std::string& spellId, int successChancePercent)
    {
        const ESM::Spell* spell =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellId);

        std::string spellName = spell->mName;
        if (spellName != mSpellName && mSpellVisible)
        {
            mWeaponSpellTimer = 5.0f;
            mSpellName = spellName;
            mWeaponSpellBox->setCaption(mSpellName);
            mWeaponSpellBox->setVisible(true);
        }

        mSpellStatus->setProgressRange(100);
        mSpellStatus->setProgressPosition(successChancePercent);

        if (mSpellImage->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mSpellImage->getChildAt(0));

        mSpellBox->setUserString("ToolTipType", "Spell");
        mSpellBox->setUserString("Spell", spellId);

        // use the icon of the first effect
        const ESM::MagicEffect* effect =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(spell->mEffects.mList.front().mEffectID);

        std::string icon = effect->mIcon;
        int slashPos = icon.find("\\");
        icon.insert(slashPos+1, "b_");
        icon = std::string("icons\\") + icon;
        Widgets::fixTexturePath(icon);
        mSpellImage->setImageTexture(icon);
    }

    void HUD::setSelectedEnchantItem(const MWWorld::Ptr& item, int chargePercent)
    {
        std::string itemName = MWWorld::Class::get(item).getName(item);
        if (itemName != mSpellName && mSpellVisible)
        {
            mWeaponSpellTimer = 5.0f;
            mSpellName = itemName;
            mWeaponSpellBox->setCaption(mSpellName);
            mWeaponSpellBox->setVisible(true);
        }

        mSpellStatus->setProgressRange(100);
        mSpellStatus->setProgressPosition(chargePercent);

        if (mSpellImage->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mSpellImage->getChildAt(0));

        mSpellBox->setUserString("ToolTipType", "ItemPtr");
        mSpellBox->setUserData(item);

        mSpellImage->setImageTexture("textures\\menu_icon_magic_mini.dds");
        MyGUI::ImageBox* itemBox = mSpellImage->createWidgetReal<MyGUI::ImageBox>("ImageBox", MyGUI::FloatCoord(0,0,1,1)
            , MyGUI::Align::Stretch);

        std::string path = std::string("icons\\");
        path+=MWWorld::Class::get(item).getInventoryIcon(item);
        Widgets::fixTexturePath(path);
        itemBox->setImageTexture(path);
        itemBox->setNeedMouseFocus(false);
    }

    void HUD::setSelectedWeapon(const MWWorld::Ptr& item, int durabilityPercent)
    {
        std::string itemName = MWWorld::Class::get(item).getName(item);
        if (itemName != mWeaponName && mWeaponVisible)
        {
            mWeaponSpellTimer = 5.0f;
            mWeaponName = itemName;
            mWeaponSpellBox->setCaption(mWeaponName);
            mWeaponSpellBox->setVisible(true);
        }

        mWeapBox->setUserString("ToolTipType", "ItemPtr");
        mWeapBox->setUserData(item);

        mWeapStatus->setProgressRange(100);
        mWeapStatus->setProgressPosition(durabilityPercent);

        if (mWeapImage->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mWeapImage->getChildAt(0));

        std::string path = std::string("icons\\");
        path+=MWWorld::Class::get(item).getInventoryIcon(item);
        Widgets::fixTexturePath(path);

        if (MWWorld::Class::get(item).getEnchantment(item) != "")
        {
            mWeapImage->setImageTexture("textures\\menu_icon_magic_mini.dds");
            MyGUI::ImageBox* itemBox = mWeapImage->createWidgetReal<MyGUI::ImageBox>("ImageBox", MyGUI::FloatCoord(0,0,1,1)
                , MyGUI::Align::Stretch);
            itemBox->setImageTexture(path);
            itemBox->setNeedMouseFocus(false);
        }
        else
            mWeapImage->setImageTexture(path);
    }

    void HUD::unsetSelectedSpell()
    {
        std::string spellName = "#{sNone}";
        if (spellName != mSpellName && mSpellVisible)
        {
            mWeaponSpellTimer = 5.0f;
            mSpellName = spellName;
            mWeaponSpellBox->setCaptionWithReplacing(mSpellName);
            mWeaponSpellBox->setVisible(true);
        }

        if (mSpellImage->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mSpellImage->getChildAt(0));
        mSpellStatus->setProgressRange(100);
        mSpellStatus->setProgressPosition(0);
        mSpellImage->setImageTexture("");
        mSpellBox->clearUserStrings();
    }

    void HUD::unsetSelectedWeapon()
    {
        std::string itemName = "#{sSkillHandtohand}";
        if (itemName != mWeaponName && mWeaponVisible)
        {
            mWeaponSpellTimer = 5.0f;
            mWeaponName = itemName;
            mWeaponSpellBox->setCaptionWithReplacing(mWeaponName);
            mWeaponSpellBox->setVisible(true);
        }

        if (mWeapImage->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mWeapImage->getChildAt(0));
        mWeapStatus->setProgressRange(100);
        mWeapStatus->setProgressPosition(0);

        MWBase::World *world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr player = world->getPlayer().getPlayer();
        if (MWWorld::Class::get(player).getNpcStats(player).isWerewolf())
            mWeapImage->setImageTexture("icons\\k\\tx_werewolf_hand.dds");
        else
            mWeapImage->setImageTexture("icons\\k\\stealth_handtohand.dds");

        mWeapBox->clearUserStrings();
    }

    void HUD::setCrosshairVisible(bool visible)
    {
        mCrosshair->setVisible (visible);
    }

    void HUD::setHmsVisible(bool visible)
    {
        mHealth->setVisible(visible);
        mMagicka->setVisible(visible);
        mStamina->setVisible(visible);
        updatePositions();
    }

    void HUD::setWeapVisible(bool visible)
    {
        mWeapBox->setVisible(visible);
        updatePositions();
    }

    void HUD::setSpellVisible(bool visible)
    {
        mSpellBox->setVisible(visible);
        updatePositions();
    }

    void HUD::setSneakVisible(bool visible)
    {
        mSneakBox->setVisible(visible);
        updatePositions();
    }

    void HUD::setEffectVisible(bool visible)
    {
        mEffectBox->setVisible (visible);
        updatePositions();
    }

    void HUD::setMinimapVisible(bool visible)
    {
        mMinimapBox->setVisible (visible);
        updatePositions();
    }

    void HUD::updatePositions()
    {
        int weapDx = 0, spellDx = 0, sneakDx = 0;
        if (!mHealth->getVisible())
            sneakDx = spellDx = weapDx = mWeapBoxBaseLeft - mHealthManaStaminaBaseLeft;

        if (!mWeapBox->getVisible())
        {
            spellDx += mSpellBoxBaseLeft - mWeapBoxBaseLeft;
            sneakDx = spellDx;
        }

        if (!mSpellBox->getVisible())
            sneakDx += mSneakBoxBaseLeft - mSpellBoxBaseLeft;

        mWeaponVisible = mWeapBox->getVisible();
        mSpellVisible = mSpellBox->getVisible();
        if (!mWeaponVisible && !mSpellVisible)
            mWeaponSpellBox->setVisible(false);

        mWeapBox->setPosition(mWeapBoxBaseLeft - weapDx, mWeapBox->getTop());
        mSpellBox->setPosition(mSpellBoxBaseLeft - spellDx, mSpellBox->getTop());
        mSneakBox->setPosition(mSneakBoxBaseLeft - sneakDx, mSneakBox->getTop());

        const MyGUI::IntSize& viewSize = MyGUI::RenderManager::getInstance().getViewSize();

        // effect box can have variable width -> variable left coordinate
        int effectsDx = 0;
        if (!mMinimapBox->getVisible ())
            effectsDx = (viewSize.width - mMinimapBoxBaseRight) - (viewSize.width - mEffectBoxBaseRight);

        mMapVisible = mMinimapBox->getVisible ();
        mEffectBox->setPosition((viewSize.width - mEffectBoxBaseRight) - mEffectBox->getWidth() + effectsDx, mEffectBox->getTop());
    }

    void HUD::update()
    {
        mSpellIcons->updateWidgets(mEffectBox, true);

        if (!mEnemy.isEmpty() && mEnemyHealth->getVisible())
        {
            MWMechanics::CreatureStats& stats = MWWorld::Class::get(mEnemy).getCreatureStats(mEnemy);
            mEnemyHealth->setProgressRange(100);
            mEnemyHealth->setProgressPosition(stats.getHealth().getCurrent() / stats.getHealth().getModified() * 100);
        }

        if (mIsDrowning)
        {
            float intensity = (cos(mDrowningFlashTheta) + 1.0f) / 2.0f;
            mDrowningFlash->setColour(MyGUI::Colour(intensity, intensity, intensity));
        }
    }

    void HUD::setEnemy(const MWWorld::Ptr &enemy)
    {
        mEnemy = enemy;
        mEnemyHealthTimer = 5;
        if (!mEnemyHealth->getVisible())
            mWeaponSpellBox->setPosition(mWeaponSpellBox->getPosition() - MyGUI::IntPoint(0,20));
        mEnemyHealth->setVisible(true);
        MWMechanics::CreatureStats& stats = MWWorld::Class::get(mEnemy).getCreatureStats(mEnemy);
        mEnemyHealth->setProgressRange(100);
        mEnemyHealth->setProgressPosition(stats.getHealth().getCurrent() / stats.getHealth().getModified() * 100);
    }

}
