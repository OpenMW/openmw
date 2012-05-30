#include "hud.hpp"

#include <cmath>

#include <MyGUI.h>

#include <boost/lexical_cast.hpp>

#include "../mwbase/environment.hpp"
#include "../mwsound/soundmanager.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/world.hpp"
#include "../mwworld/player.hpp"

#include "window_manager.hpp"
#include "container.hpp"

using namespace MWGui;

HUD::HUD(int width, int height, int fpsLevel, DragAndDrop* dragAndDrop)
    : Layout("openmw_hud_layout.xml")
    , health(NULL)
    , magicka(NULL)
    , stamina(NULL)
    , weapImage(NULL)
    , spellImage(NULL)
    , weapStatus(NULL)
    , spellStatus(NULL)
    , effectBox(NULL)
    , effect1(NULL)
    , minimap(NULL)
    , compass(NULL)
    , crosshair(NULL)
    , fpsbox(NULL)
    , fpscounter(NULL)
    , trianglecounter(NULL)
    , batchcounter(NULL)
    , hmsBaseLeft(0)
    , weapBoxBaseLeft(0)
    , spellBoxBaseLeft(0)
    , effectBoxBaseRight(0)
    , minimapBoxBaseRight(0)
    , mDragAndDrop(dragAndDrop)
    , mCellNameTimer(0.0f)
    , mCellNameBox(NULL)
    , mMapVisible(true)
    , mWeaponVisible(true)
    , mSpellVisible(true)
{
    setCoord(0,0, width, height);

    // Energy bars
    getWidget(mHealthFrame, "HealthFrame");
    getWidget(health, "Health");
    getWidget(magicka, "Magicka");
    getWidget(stamina, "Stamina");

    hmsBaseLeft = mHealthFrame->getLeft();

    MyGUI::Widget *healthFrame, *magickaFrame, *fatigueFrame;
    getWidget(healthFrame, "HealthFrame");
    getWidget(magickaFrame, "MagickaFrame");
    getWidget(fatigueFrame, "FatigueFrame");
    healthFrame->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onHMSClicked);
    magickaFrame->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onHMSClicked);
    fatigueFrame->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onHMSClicked);

    const MyGUI::IntSize& viewSize = MyGUI::RenderManager::getInstance().getViewSize();

    // Item and spell images and status bars
    getWidget(weapBox, "WeapBox");
    getWidget(weapImage, "WeapImage");
    getWidget(weapStatus, "WeapStatus");
    weapBoxBaseLeft = weapBox->getLeft();
    weapBox->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onWeaponClicked);

    getWidget(spellBox, "SpellBox");
    getWidget(spellImage, "SpellImage");
    getWidget(spellStatus, "SpellStatus");
    spellBoxBaseLeft = spellBox->getLeft();
    spellBox->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onMagicClicked);

    getWidget(effectBox, "EffectBox");
    getWidget(effect1, "Effect1");
    effectBoxBaseRight = viewSize.width - effectBox->getRight();
    effectBox->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onMagicClicked);

    getWidget(minimapBox, "MiniMapBox");
    minimapBoxBaseRight = viewSize.width - minimapBox->getRight();
    minimapBox->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onMapClicked);
    getWidget(minimap, "MiniMap");
    getWidget(compass, "Compass");

    getWidget(mCellNameBox, "CellName");
    getWidget(mWeaponSpellBox, "WeaponSpellName");

    getWidget(crosshair, "Crosshair");

    setFpsLevel(fpsLevel);

    getWidget(trianglecounter, "TriangleCounter");
    getWidget(batchcounter, "BatchCounter");

    setEffect("icons\\s\\tx_s_chameleon.dds");

    LocalMapBase::init(minimap, compass, this);

    mMainWidget->eventMouseButtonClick += MyGUI::newDelegate(this, &HUD::onWorldClicked);
    mMainWidget->eventMouseMove += MyGUI::newDelegate(this, &HUD::onWorldMouseOver);
    mMainWidget->eventMouseLostFocus += MyGUI::newDelegate(this, &HUD::onWorldMouseLostFocus);
}

void HUD::setFpsLevel(int level)
{
    fpscounter = 0;

    MyGUI::Widget* fps;
    getWidget(fps, "FPSBoxAdv");
    fps->setVisible(false);
    getWidget(fps, "FPSBox");
    fps->setVisible(false);

    if (level == 2)
    {
        getWidget(fpsbox, "FPSBoxAdv");
        fpsbox->setVisible(true);
        getWidget(fpscounter, "FPSCounterAdv");
    }
    else if (level == 1)
    {
        getWidget(fpsbox, "FPSBox");
        fpsbox->setVisible(true);
        getWidget(fpscounter, "FPSCounter");
    }
}

void HUD::setFPS(float fps)
{
    if (fpscounter)
        fpscounter->setCaption(boost::lexical_cast<std::string>((int)fps));
}

void HUD::setTriangleCount(size_t count)
{
    trianglecounter->setCaption(boost::lexical_cast<std::string>(count));
}

void HUD::setBatchCount(size_t count)
{
    batchcounter->setCaption(boost::lexical_cast<std::string>(count));
}

void HUD::setEffect(const char *img)
{
    effect1->setImageTexture(img);
}

void HUD::setValue(const std::string& id, const MWMechanics::DynamicStat<int>& value)
{
    static const char *ids[] =
    {
        "HBar", "MBar", "FBar", 0
    };

    for (int i=0; ids[i]; ++i)
        if (ids[i]==id)
        {
            MyGUI::Widget* w;
            std::string valStr = boost::lexical_cast<std::string>(value.getCurrent()) + "/" + boost::lexical_cast<std::string>(value.getModified());
            switch (i)
            {
                case 0:
                    health->setProgressRange (value.getModified());
                    health->setProgressPosition (value.getCurrent());
                    getWidget(w, "HealthFrame");
                    w->setUserString("Caption_HealthDescription", "#{sHealthDesc}\n" + valStr);
                    break;
                case 1:
                    magicka->setProgressRange (value.getModified());
                    magicka->setProgressPosition (value.getCurrent());
                    getWidget(w, "MagickaFrame");
                    w->setUserString("Caption_HealthDescription", "#{sIntDesc}\n" + valStr);
                    break;
                case 2:
                    stamina->setProgressRange (value.getModified());
                    stamina->setProgressPosition (value.getCurrent());
                    getWidget(w, "FatigueFrame");
                    w->setUserString("Caption_HealthDescription", "#{sFatDesc}\n" + valStr);
                    break;
            }
        }
}

void HUD::setBottomLeftVisibility(bool hmsVisible, bool weapVisible, bool spellVisible)
{
    int weapDx = 0, spellDx = 0;
    if (!hmsVisible)
        spellDx = weapDx = weapBoxBaseLeft - hmsBaseLeft;

    if (!weapVisible)
        spellDx += spellBoxBaseLeft - weapBoxBaseLeft;

    mWeaponVisible = weapVisible;
    mSpellVisible = spellVisible;
    if (!mWeaponVisible && !mSpellVisible)
        mWeaponSpellBox->setVisible(false);

    health->setVisible(hmsVisible);
    stamina->setVisible(hmsVisible);
    magicka->setVisible(hmsVisible);
    weapBox->setPosition(weapBoxBaseLeft - weapDx, weapBox->getTop());
    weapBox->setVisible(weapVisible);
    spellBox->setPosition(spellBoxBaseLeft - spellDx, spellBox->getTop());
    spellBox->setVisible(spellVisible);
}

void HUD::setBottomRightVisibility(bool effectBoxVisible, bool minimapBoxVisible)
{
    const MyGUI::IntSize& viewSize = MyGUI::RenderManager::getInstance().getViewSize();

    // effect box can have variable width -> variable left coordinate
    int effectsDx = 0;
    if (!minimapBoxVisible)
        effectsDx = (viewSize.width - minimapBoxBaseRight) - (viewSize.width - effectBoxBaseRight);

    mMapVisible = minimapBoxVisible;
    minimapBox->setVisible(minimapBoxVisible);
    effectBox->setPosition((viewSize.width - effectBoxBaseRight) - effectBox->getWidth() + effectsDx, effectBox->getTop());
    effectBox->setVisible(effectBoxVisible);
}

void HUD::onWorldClicked(MyGUI::Widget* _sender)
{
    if (mDragAndDrop->mIsOnDragAndDrop)
    {
        // drop item into the gameworld
        MWWorld::Ptr object = *mDragAndDrop->mDraggedWidget->getUserData<MWWorld::Ptr>();

        MWWorld::World* world = MWBase::Environment::get().getWorld();

        MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        MyGUI::IntPoint cursorPosition = MyGUI::InputManager::getInstance().getMousePosition();
        float mouseX = cursorPosition.left / float(viewSize.width);
        float mouseY = cursorPosition.top / float(viewSize.height);

        int origCount = object.getRefData().getCount();
        object.getRefData().setCount(mDragAndDrop->mDraggedCount);

        if (world->canPlaceObject(mouseX, mouseY))
            world->placeObject(object, mouseX, mouseY);
        else
            world->dropObjectOnGround(object);

        MyGUI::PointerManager::getInstance().setPointer("arrow");

        std::string sound = MWWorld::Class::get(object).getDownSoundId(object);
        MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);

        // remove object from the container it was coming from
        object.getRefData().setCount(origCount - mDragAndDrop->mDraggedCount);

        mDragAndDrop->mIsOnDragAndDrop = false;
        MyGUI::Gui::getInstance().destroyWidget(mDragAndDrop->mDraggedWidget);
        mDragAndDrop->mDraggedWidget = 0;

        MWBase::Environment::get().getWindowManager()->setDragDrop(false);
    }
}

void HUD::onWorldMouseOver(MyGUI::Widget* _sender, int x, int y)
{
    if (mDragAndDrop->mIsOnDragAndDrop)
    {
        MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        MyGUI::IntPoint cursorPosition = MyGUI::InputManager::getInstance().getMousePosition();
        float mouseX = cursorPosition.left / float(viewSize.width);
        float mouseY = cursorPosition.top / float(viewSize.height);

        MWWorld::World* world = MWBase::Environment::get().getWorld();

        // if we can't drop the object at the wanted position, show the "drop on ground" cursor.
        bool canDrop = world->canPlaceObject(mouseX, mouseY);

        if (!canDrop)
            MyGUI::PointerManager::getInstance().setPointer("drop_ground");
        else
            MyGUI::PointerManager::getInstance().setPointer("arrow");

    }
    else
    {
        MyGUI::PointerManager::getInstance().setPointer("arrow");
        /// \todo make it possible to pick up objects with the mouse, if inventory or container window is open
    }
}

void HUD::onWorldMouseLostFocus(MyGUI::Widget* _sender, MyGUI::Widget* _new)
{
    MyGUI::PointerManager::getInstance().setPointer("arrow");
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
    MWBase::Environment::get().getWindowManager()->toggleVisible(GW_Inventory);
}

void HUD::onMagicClicked(MyGUI::Widget* _sender)
{
    MWBase::Environment::get().getWindowManager()->toggleVisible(GW_Magic);
}

void HUD::setCellName(const std::string& cellName)
{
    if (mCellName != cellName)
    {
        mCellNameTimer = 5.0f;
        mCellName = cellName;

        mCellNameBox->setCaption(mCellName);
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
}

void HUD::onResChange(int width, int height)
{
    setCoord(0, 0, width, height);
}

void HUD::setSelectedSpell(const std::string& spellId, int successChancePercent)
{
    const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().spells.find(spellId);
    std::string spellName = spell->name;
    if (spellName != mSpellName && mSpellVisible)
    {
        mWeaponSpellTimer = 5.0f;
        mSpellName = spellName;
        mWeaponSpellBox->setCaption(mSpellName);
        mWeaponSpellBox->setVisible(true);
    }

    spellStatus->setProgressRange(100);
    spellStatus->setProgressPosition(successChancePercent);

    if (spellImage->getChildCount())
        MyGUI::Gui::getInstance().destroyWidget(spellImage->getChildAt(0));

    spellBox->setUserString("ToolTipType", "Spell");
    spellBox->setUserString("Spell", spellId);

    // use the icon of the first effect
    const ESM::MagicEffect* effect = MWBase::Environment::get().getWorld()->getStore().magicEffects.find(spell->effects.list.front().effectID);
    std::string icon = effect->icon;
    int slashPos = icon.find("\\");
    icon.insert(slashPos+1, "b_");
    icon = std::string("icons\\") + icon;
    Widgets::fixTexturePath(icon);
    spellImage->setImageTexture(icon);
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

    spellStatus->setProgressRange(100);
    spellStatus->setProgressPosition(chargePercent);

    if (spellImage->getChildCount())
        MyGUI::Gui::getInstance().destroyWidget(spellImage->getChildAt(0));

    spellBox->setUserString("ToolTipType", "ItemPtr");
    spellBox->setUserData(item);

    spellImage->setImageTexture("textures\\menu_icon_magic_mini.dds");
    MyGUI::ImageBox* itemBox = spellImage->createWidgetReal<MyGUI::ImageBox>("ImageBox", MyGUI::FloatCoord(0,0,1,1)
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

    weapBox->setUserString("ToolTipType", "ItemPtr");
    weapBox->setUserData(item);

    weapStatus->setProgressRange(100);
    weapStatus->setProgressPosition(durabilityPercent);

    if (weapImage->getChildCount())
        MyGUI::Gui::getInstance().destroyWidget(weapImage->getChildAt(0));

    std::string path = std::string("icons\\");
    path+=MWWorld::Class::get(item).getInventoryIcon(item);
    Widgets::fixTexturePath(path);

    if (MWWorld::Class::get(item).getEnchantment(item) != "")
    {
        weapImage->setImageTexture("textures\\menu_icon_magic_mini.dds");
        MyGUI::ImageBox* itemBox = weapImage->createWidgetReal<MyGUI::ImageBox>("ImageBox", MyGUI::FloatCoord(0,0,1,1)
            , MyGUI::Align::Stretch);
        itemBox->setImageTexture(path);
        itemBox->setNeedMouseFocus(false);
    }
    else
        weapImage->setImageTexture(path);
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

    if (spellImage->getChildCount())
        MyGUI::Gui::getInstance().destroyWidget(spellImage->getChildAt(0));
    spellStatus->setProgressRange(100);
    spellStatus->setProgressPosition(0);
    spellImage->setImageTexture("");
    spellBox->clearUserStrings();
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

    if (weapImage->getChildCount())
        MyGUI::Gui::getInstance().destroyWidget(weapImage->getChildAt(0));
    weapStatus->setProgressRange(100);
    weapStatus->setProgressPosition(0);
    weapImage->setImageTexture("icons\\k\\stealth_handtohand.dds");
    weapBox->clearUserStrings();
}
