#include "tooltips.hpp"

#include <iomanip>

#include <MyGUI_Gui.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_ImageBox.h>

#include <components/settings/settings.hpp>
#include <components/widgets/box.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "mapwindow.hpp"
#include "inventorywindow.hpp"

#include "itemmodel.hpp"

namespace MWGui
{
    std::string ToolTips::sSchoolNames[] = {"#{sSchoolAlteration}", "#{sSchoolConjuration}", "#{sSchoolDestruction}", "#{sSchoolIllusion}", "#{sSchoolMysticism}", "#{sSchoolRestoration}"};

    ToolTips::ToolTips() :
        Layout("openmw_tooltips.layout")
        , mFocusToolTipX(0.0)
        , mFocusToolTipY(0.0)
        , mHorizontalScrollIndex(0)
        , mDelay(0.0)
        , mRemainingDelay(0.0)
        , mLastMouseX(0)
        , mLastMouseY(0)
        , mEnabled(true)
        , mFullHelp(false)
        , mShowOwned(0)
        , mFrameDuration(0.f)
    {
        getWidget(mDynamicToolTipBox, "DynamicToolTipBox");

        mDynamicToolTipBox->setVisible(false);

        // turn off mouse focus so that getMouseFocusWidget returns the correct widget,
        // even if the mouse is over the tooltip
        mDynamicToolTipBox->setNeedMouseFocus(false);
        mMainWidget->setNeedMouseFocus(false);

        mDelay = Settings::Manager::getFloat("tooltip delay", "GUI");
        mRemainingDelay = mDelay;

        for (unsigned int i=0; i < mMainWidget->getChildCount(); ++i)
        {
            mMainWidget->getChildAt(i)->setVisible(false);
        }
        
        mShowOwned = Settings::Manager::getInt("show owned", "Game");
    }

    void ToolTips::setEnabled(bool enabled)
    {
        mEnabled = enabled;
    }

    void ToolTips::onFrame(float frameDuration)
    {
        mFrameDuration = frameDuration;
    }

    void ToolTips::update(float frameDuration)
    {

        while (mDynamicToolTipBox->getChildCount())
        {
            MyGUI::Gui::getInstance().destroyWidget(mDynamicToolTipBox->getChildAt(0));
        }

        // start by hiding everything
        for (unsigned int i=0; i < mMainWidget->getChildCount(); ++i)
        {
            mMainWidget->getChildAt(i)->setVisible(false);
        }

        const MyGUI::IntSize &viewSize = MyGUI::RenderManager::getInstance().getViewSize();

        if (!mEnabled)
        {
            return;
        }

        MWBase::WindowManager *winMgr = MWBase::Environment::get().getWindowManager();
        bool guiMode = winMgr->isGuiMode();

        if (guiMode)
        {
            if (!winMgr->getCursorVisible())
                return;
            const MyGUI::IntPoint& mousePos = MyGUI::InputManager::getInstance().getMousePosition();

            if (winMgr->getWorldMouseOver() &&
                (winMgr->isConsoleMode() ||
                (winMgr->getMode() == GM_Container) ||
                (winMgr->getMode() == GM_Inventory)))
            {
                if (mFocusObject.isEmpty ())
                    return;

                const MWWorld::Class& objectclass = mFocusObject.getClass();

                MyGUI::IntSize tooltipSize;
                if (!objectclass.hasToolTip(mFocusObject) && winMgr->isConsoleMode())
                {
                    setCoord(0, 0, 300, 300);
                    mDynamicToolTipBox->setVisible(true);
                    ToolTipInfo info;
                    info.caption = mFocusObject.getClass().getName(mFocusObject);
                    if (info.caption.empty())
                        info.caption=mFocusObject.getCellRef().getRefId();
                    info.icon="";
                    tooltipSize = createToolTip(info, checkOwned());
                }
                else
                    tooltipSize = getToolTipViaPtr(mFocusObject.getRefData().getCount(), true);

                MyGUI::IntPoint tooltipPosition = MyGUI::InputManager::getInstance().getMousePosition();
                position(tooltipPosition, tooltipSize, viewSize);

                setCoord(tooltipPosition.left, tooltipPosition.top, tooltipSize.width, tooltipSize.height);
            }

            else
            {
                if (mousePos.left == mLastMouseX && mousePos.top == mLastMouseY)
                {
                    mRemainingDelay -= frameDuration;
                }
                else
                {
                    mHorizontalScrollIndex = 0;
                    mRemainingDelay = mDelay;
                }
                mLastMouseX = mousePos.left;
                mLastMouseY = mousePos.top;


                if (mRemainingDelay > 0)
                    return;

                MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getMouseFocusWidget();
                if (focus == 0)
                    return;

                MyGUI::IntSize tooltipSize;

                // try to go 1 level up until there is a widget that has tooltip
                // this is necessary because some skin elements are actually separate widgets
                int i=0;
                while (!focus->isUserString("ToolTipType"))
                {
                    focus = focus->getParent();
                    if (!focus)
                        return;
                    ++i;
                }

                std::string type = focus->getUserString("ToolTipType");

                if (type == "")
                {
                    return;
                }


                // special handling for markers on the local map: the tooltip should only be visible
                // if the marker is not hidden due to the fog of war.
                if (type == "MapMarker")
                {
                    LocalMapBase::MarkerUserData data = *focus->getUserData<LocalMapBase::MarkerUserData>();

                    if (!data.isPositionExplored())
                        return;

                    ToolTipInfo info;
                    info.text = data.caption;
                    info.notes = data.notes;
                    tooltipSize = createToolTip(info);
                }
                else if (type == "ItemPtr")
                {
                    mFocusObject = *focus->getUserData<MWWorld::Ptr>();
                    if (!mFocusObject)
                        return;

                    tooltipSize = getToolTipViaPtr(mFocusObject.getRefData().getCount(), false, checkOwned());
                }
                else if (type == "ItemModelIndex")
                {
                    std::pair<ItemModel::ModelIndex, ItemModel*> pair = *focus->getUserData<std::pair<ItemModel::ModelIndex, ItemModel*> >();
                    mFocusObject = pair.second->getItem(pair.first).mBase;
                    bool isAllowedToUse = pair.second->allowedToUseItems();
                    tooltipSize = getToolTipViaPtr(pair.second->getItem(pair.first).mCount, false, !isAllowedToUse);
                }
                else if (type == "ToolTipInfo")
                {
                    tooltipSize = createToolTip(*focus->getUserData<MWGui::ToolTipInfo>());
                }
                else if (type == "AvatarItemSelection")
                {
                    MyGUI::IntCoord avatarPos = focus->getAbsoluteCoord();
                    MyGUI::IntPoint relMousePos = MyGUI::InputManager::getInstance ().getMousePosition () - MyGUI::IntPoint(avatarPos.left, avatarPos.top);
                    MWWorld::Ptr item = winMgr->getInventoryWindow ()->getAvatarSelectedItem (relMousePos.left, relMousePos.top);

                    mFocusObject = item;
                    if (!mFocusObject.isEmpty ())
                        tooltipSize = getToolTipViaPtr(mFocusObject.getRefData().getCount(), false);
                }
                else if (type == "Spell")
                {
                    ToolTipInfo info;

                    const ESM::Spell *spell =
                        MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(focus->getUserString("Spell"));
                    info.caption = spell->mName;
                    Widgets::SpellEffectList effects;
                    for (const ESM::ENAMstruct& spellEffect : spell->mEffects.mList)
                    {
                        Widgets::SpellEffectParams params;
                        params.mEffectID = spellEffect.mEffectID;
                        params.mSkill = spellEffect.mSkill;
                        params.mAttribute = spellEffect.mAttribute;
                        params.mDuration = spellEffect.mDuration;
                        params.mMagnMin = spellEffect.mMagnMin;
                        params.mMagnMax = spellEffect.mMagnMax;
                        params.mRange = spellEffect.mRange;
                        params.mArea = spellEffect.mArea;
                        params.mIsConstant = (spell->mData.mType == ESM::Spell::ST_Ability);
                        params.mNoTarget = false;
                        effects.push_back(params);
                    }
                    if (MWMechanics::spellIncreasesSkill(spell)) // display school of spells that contribute to skill progress
                    {
                        MWWorld::Ptr player = MWMechanics::getPlayer();
                        int school = MWMechanics::getSpellSchool(spell, player);
                        info.text = "#{sSchool}: " + sSchoolNames[school];
                    }
                    std::string cost = focus->getUserString("SpellCost");
                    if (cost != "" && cost != "0")
                        info.text += MWGui::ToolTips::getValueString(spell->mData.mCost, "#{sCastCost}");
                    info.effects = effects;
                    tooltipSize = createToolTip(info);
                }
                else if (type == "Layout")
                {
                    // tooltip defined in the layout
                    MyGUI::Widget* tooltip;
                    getWidget(tooltip, focus->getUserString("ToolTipLayout"));

                    tooltip->setVisible(true);

                    std::map<std::string, std::string> userStrings = focus->getUserStrings();
                    for (auto& userStringPair : userStrings)
                    {
                        size_t underscorePos = userStringPair.first.find("_");
                        if (underscorePos == std::string::npos)
                            continue;
                        std::string key = userStringPair.first.substr(0, underscorePos);
                        std::string widgetName = userStringPair.first.substr(underscorePos+1, userStringPair.first.size()-(underscorePos+1));

                        type = "Property";
                        size_t caretPos = key.find("^");
                        if (caretPos != std::string::npos)
                        {
                            type = key.substr(0, caretPos);
                            key.erase(key.begin(), key.begin() + caretPos + 1);
                        }

                        MyGUI::Widget* w;
                        getWidget(w, widgetName);
                        if (type == "Property")
                            w->setProperty(key, userStringPair.second);
                        else if (type == "UserData")
                            w->setUserString(key, userStringPair.second);
                    }

                    tooltipSize = tooltip->getSize();

                    tooltip->setCoord(0, 0, tooltipSize.width, tooltipSize.height);
                }
                else
                    throw std::runtime_error ("unknown tooltip type");

                MyGUI::IntPoint tooltipPosition = MyGUI::InputManager::getInstance().getMousePosition();

                position(tooltipPosition, tooltipSize, viewSize);

                setCoord(tooltipPosition.left, tooltipPosition.top, tooltipSize.width, tooltipSize.height);
            }
        }
        else
        {
            if (!mFocusObject.isEmpty())
            {
                MyGUI::IntSize tooltipSize = getToolTipViaPtr(mFocusObject.getRefData().getCount(), true, checkOwned());

                setCoord(viewSize.width/2 - tooltipSize.width/2,
                        std::max(0, int(mFocusToolTipY*viewSize.height - tooltipSize.height)),
                        tooltipSize.width,
                        tooltipSize.height);

                mDynamicToolTipBox->setVisible(true);
            }
        }
    }

    void ToolTips::position(MyGUI::IntPoint& position, MyGUI::IntSize size, MyGUI::IntSize viewportSize)
    {
        position += MyGUI::IntPoint(0, 32)
        - MyGUI::IntPoint(static_cast<int>(MyGUI::InputManager::getInstance().getMousePosition().left / float(viewportSize.width) * size.width), 0);

        if ((position.left + size.width) > viewportSize.width)
        {
            position.left = viewportSize.width - size.width;
        }
        if ((position.top + size.height) > viewportSize.height)
        {
            position.top = MyGUI::InputManager::getInstance().getMousePosition().top - size.height - 8;
        }
    }

    void ToolTips::clear()
    {
        mFocusObject = MWWorld::Ptr();

        while (mDynamicToolTipBox->getChildCount())
        {
            MyGUI::Gui::getInstance().destroyWidget(mDynamicToolTipBox->getChildAt(0));
        }

        for (unsigned int i=0; i < mMainWidget->getChildCount(); ++i)
        {
            mMainWidget->getChildAt(i)->setVisible(false);
        }
    }

    void ToolTips::setFocusObject(const MWWorld::Ptr& focus)
    {
        mFocusObject = focus;

        update(mFrameDuration);
    }

    MyGUI::IntSize ToolTips::getToolTipViaPtr (int count, bool image, bool isOwned)
    {
        // this the maximum width of the tooltip before it starts word-wrapping
        setCoord(0, 0, 300, 300);

        MyGUI::IntSize tooltipSize;

        const MWWorld::Class& object = mFocusObject.getClass();
        if (!object.hasToolTip(mFocusObject))
        {
            mDynamicToolTipBox->setVisible(false);
        }
        else
        {
            mDynamicToolTipBox->setVisible(true);

            ToolTipInfo info = object.getToolTipInfo(mFocusObject, count);
            if (!image)
                info.icon = "";
            tooltipSize = createToolTip(info, isOwned);
        }

        return tooltipSize;
    }
    
    bool ToolTips::checkOwned()
    {
        if(mFocusObject.isEmpty())
            return false;

        MWWorld::Ptr ptr = MWMechanics::getPlayer();
        MWWorld::Ptr victim;

        MWBase::MechanicsManager* mm = MWBase::Environment::get().getMechanicsManager();
        return !mm->isAllowedToUse(ptr, mFocusObject, victim);
    }

    MyGUI::IntSize ToolTips::createToolTip(const MWGui::ToolTipInfo& info, bool isOwned)
    {
        mDynamicToolTipBox->setVisible(true);
        
        if((mShowOwned == 1 || mShowOwned == 3) && isOwned)
            mDynamicToolTipBox->changeWidgetSkin(MWBase::Environment::get().getWindowManager()->isGuiMode() ? "HUD_Box_NoTransp_Owned" : "HUD_Box_Owned");
        else
            mDynamicToolTipBox->changeWidgetSkin(MWBase::Environment::get().getWindowManager()->isGuiMode() ? "HUD_Box_NoTransp" : "HUD_Box");

        std::string caption = info.caption;
        std::string image = info.icon;
        int imageSize = (image != "") ? info.imageSize : 0;
        std::string text = info.text;

        // remove the first newline (easier this way)
        if (text.size() > 0 && text[0] == '\n')
            text.erase(0, 1);

        const ESM::Enchantment* enchant = 0;
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
        if (info.enchant != "")
        {
            enchant = store.get<ESM::Enchantment>().search(info.enchant);
            if (enchant)
            {
                if (enchant->mData.mType == ESM::Enchantment::CastOnce)
                    text += "\n#{sItemCastOnce}";
                else if (enchant->mData.mType == ESM::Enchantment::WhenStrikes)
                    text += "\n#{sItemCastWhenStrikes}";
                else if (enchant->mData.mType == ESM::Enchantment::WhenUsed)
                    text += "\n#{sItemCastWhenUsed}";
                else if (enchant->mData.mType == ESM::Enchantment::ConstantEffect)
                    text += "\n#{sItemCastConstant}";
            }
        }

        // this the maximum width of the tooltip before it starts word-wrapping
        setCoord(0, 0, 300, 300);

        const MyGUI::IntPoint padding(8, 8);

        const int imageCaptionHPadding = (caption != "" ? 8 : 0);
        const int imageCaptionVPadding = (caption != "" ? 4 : 0);

        const int maximumWidth = MyGUI::RenderManager::getInstance().getViewSize().width - imageCaptionHPadding * 2;

        std::string realImage = MWBase::Environment::get().getWindowManager()->correctIconPath(image);

        Gui::EditBox* captionWidget = mDynamicToolTipBox->createWidget<Gui::EditBox>("NormalText", MyGUI::IntCoord(0, 0, 300, 300), MyGUI::Align::Left | MyGUI::Align::Top, "ToolTipCaption");
        captionWidget->setEditStatic(true);
        captionWidget->setNeedKeyFocus(false);
        captionWidget->setCaptionWithReplacing(caption);
        MyGUI::IntSize captionSize = captionWidget->getTextSize();

        int captionHeight = std::max(caption != "" ? captionSize.height : 0, imageSize);

        Gui::EditBox* textWidget = mDynamicToolTipBox->createWidget<Gui::EditBox>("SandText", MyGUI::IntCoord(0, captionHeight+imageCaptionVPadding, 300, 300-captionHeight-imageCaptionVPadding), MyGUI::Align::Stretch, "ToolTipText");
        textWidget->setEditStatic(true);
        textWidget->setEditMultiLine(true);
        textWidget->setEditWordWrap(info.wordWrap);
        textWidget->setCaptionWithReplacing(text);
        textWidget->setTextAlign(MyGUI::Align::HCenter | MyGUI::Align::Top);
        textWidget->setNeedKeyFocus(false);
        MyGUI::IntSize textSize = textWidget->getTextSize();

        captionSize += MyGUI::IntSize(imageSize, 0); // adjust for image
        MyGUI::IntSize totalSize = MyGUI::IntSize( std::min(std::max(textSize.width,captionSize.width + ((image != "") ? imageCaptionHPadding : 0)),maximumWidth),
            ((text != "") ? textSize.height + imageCaptionVPadding : 0) + captionHeight );

        for (const std::string& note : info.notes)
        {
            MyGUI::ImageBox* icon = mDynamicToolTipBox->createWidget<MyGUI::ImageBox>("MarkerButton",
                MyGUI::IntCoord(padding.left, totalSize.height+padding.top, 8, 8), MyGUI::Align::Default);
            icon->setColour(MyGUI::Colour(1.0f, 0.3f, 0.3f));
            Gui::EditBox* edit = mDynamicToolTipBox->createWidget<Gui::EditBox>("SandText",
                MyGUI::IntCoord(padding.left+8+4, totalSize.height+padding.top, 300-padding.left-8-4, 300-totalSize.height),
                                                                                    MyGUI::Align::Default);
            edit->setEditMultiLine(true);
            edit->setEditWordWrap(true);
            edit->setCaption(note);
            edit->setSize(edit->getWidth(), edit->getTextSize().height);
            icon->setPosition(icon->getLeft(),(edit->getTop()+edit->getBottom())/2-icon->getHeight()/2);
            totalSize.height += std::max(edit->getHeight(), icon->getHeight());
            totalSize.width = std::max(totalSize.width, edit->getWidth()+8+4);
        }

        if (!info.effects.empty())
        {
            MyGUI::Widget* effectArea = mDynamicToolTipBox->createWidget<MyGUI::Widget>("",
                MyGUI::IntCoord(padding.left, totalSize.height, 300-padding.left, 300-totalSize.height),
                MyGUI::Align::Stretch);

            MyGUI::IntCoord coord(0, 6, totalSize.width, 24);

            Widgets::MWEffectListPtr effectsWidget = effectArea->createWidget<Widgets::MWEffectList>
                ("MW_StatName", coord, MyGUI::Align::Default);
            effectsWidget->setEffectList(info.effects);

            std::vector<MyGUI::Widget*> effectItems;
            int flag = info.isPotion ? Widgets::MWEffectList::EF_NoTarget : 0;
            flag |= info.isIngredient ? Widgets::MWEffectList::EF_NoMagnitude : 0;
            effectsWidget->createEffectWidgets(effectItems, effectArea, coord, true, flag);
            totalSize.height += coord.top-6;
            totalSize.width = std::max(totalSize.width, coord.width);
        }

        if (enchant)
        {
            MyGUI::Widget* enchantArea = mDynamicToolTipBox->createWidget<MyGUI::Widget>("",
                MyGUI::IntCoord(padding.left, totalSize.height, 300-padding.left, 300-totalSize.height),
                MyGUI::Align::Stretch);

            MyGUI::IntCoord coord(0, 6, totalSize.width, 24);

            Widgets::MWEffectListPtr enchantWidget = enchantArea->createWidget<Widgets::MWEffectList>
                ("MW_StatName", coord, MyGUI::Align::Default);
            enchantWidget->setEffectList(Widgets::MWEffectList::effectListFromESM(&enchant->mEffects));

            std::vector<MyGUI::Widget*> enchantEffectItems;
            int flag = (enchant->mData.mType == ESM::Enchantment::ConstantEffect) ? Widgets::MWEffectList::EF_Constant : 0;
            enchantWidget->createEffectWidgets(enchantEffectItems, enchantArea, coord, true, flag);
            totalSize.height += coord.top-6;
            totalSize.width = std::max(totalSize.width, coord.width);

            if (enchant->mData.mType == ESM::Enchantment::WhenStrikes
                || enchant->mData.mType == ESM::Enchantment::WhenUsed)
            {
                int maxCharge = enchant->mData.mCharge;
                int charge = (info.remainingEnchantCharge == -1) ? maxCharge : info.remainingEnchantCharge;

                const int chargeWidth = 204;

                MyGUI::TextBox* chargeText = enchantArea->createWidget<MyGUI::TextBox>("SandText", MyGUI::IntCoord(0, 0, 10, 18), MyGUI::Align::Default, "ToolTipEnchantChargeText");
                chargeText->setCaptionWithReplacing("#{sCharges}");

                const int chargeTextWidth = chargeText->getTextSize().width + 5;

                const int chargeAndTextWidth = chargeWidth + chargeTextWidth;

                totalSize.width = std::max(totalSize.width, chargeAndTextWidth);

                chargeText->setCoord((totalSize.width - chargeAndTextWidth)/2, coord.top+6, chargeTextWidth, 18);

                MyGUI::IntCoord chargeCoord;
                if (totalSize.width < chargeWidth)
                {
                    totalSize.width = chargeWidth;
                    chargeCoord = MyGUI::IntCoord(0, coord.top+6, chargeWidth, 18);
                }
                else
                {
                    chargeCoord = MyGUI::IntCoord((totalSize.width - chargeAndTextWidth)/2 + chargeTextWidth, coord.top+6, chargeWidth, 18);
                }
                Widgets::MWDynamicStatPtr chargeWidget = enchantArea->createWidget<Widgets::MWDynamicStat>
                    ("MW_ChargeBar", chargeCoord, MyGUI::Align::Default);
                chargeWidget->setValue(charge, maxCharge);
                totalSize.height += 24;
            }
        }

        captionWidget->setCoord( (totalSize.width - captionSize.width)/2 + imageSize,
            (captionHeight-captionSize.height)/2,
            captionSize.width-imageSize,
            captionSize.height);

         //if its too long we do hscroll with the caption
        if (captionSize.width > maximumWidth)
        {
            mHorizontalScrollIndex = mHorizontalScrollIndex + 2;
            if (mHorizontalScrollIndex > captionSize.width){
                mHorizontalScrollIndex = -totalSize.width;
            }
            int horizontal_scroll = mHorizontalScrollIndex;
            if (horizontal_scroll < 40){
                horizontal_scroll = 40;
            }else{
                horizontal_scroll = 80 - mHorizontalScrollIndex;
            }
            captionWidget->setPosition (MyGUI::IntPoint(horizontal_scroll, captionWidget->getPosition().top + padding.top));
        } else {
            captionWidget->setPosition (captionWidget->getPosition() + padding);
        }

        textWidget->setPosition (textWidget->getPosition() + MyGUI::IntPoint(0, padding.top)); // only apply vertical padding, the horizontal works automatically due to Align::HCenter

        if (image != "")
        {
            MyGUI::ImageBox* imageWidget = mDynamicToolTipBox->createWidget<MyGUI::ImageBox>("ImageBox",
                MyGUI::IntCoord((totalSize.width - captionSize.width - imageCaptionHPadding)/2, 0, imageSize, imageSize),
                MyGUI::Align::Left | MyGUI::Align::Top);
            imageWidget->setImageTexture(realImage);
            imageWidget->setPosition (imageWidget->getPosition() + padding);
        }

        totalSize += MyGUI::IntSize(padding.left*2, padding.top*2);

        return totalSize;
    }

    std::string ToolTips::toString(const float value)
    {
        std::ostringstream stream;

        if (value != int(value))
            stream << std::setprecision(3);

        stream << value;
        return stream.str();
    }

    std::string ToolTips::toString(const int value)
    {
        return std::to_string(value);
    }

    std::string ToolTips::getWeightString(const float weight, const std::string& prefix)
    {
        if (weight == 0)
            return "";
        else
            return "\n" + prefix + ": " + toString(weight);
    }

    std::string ToolTips::getPercentString(const float value, const std::string& prefix)
    {
        if (value == 0)
            return "";
        else
            return "\n" + prefix + ": " + toString(value*100) +"%";
    }

    std::string ToolTips::getValueString(const int value, const std::string& prefix)
    {
        if (value == 0)
            return "";
        else
            return "\n" + prefix + ": " + toString(value);
    }

    std::string ToolTips::getMiscString(const std::string& text, const std::string& prefix)
    {
        if (text == "")
            return "";
        else
            return "\n" + prefix + ": " + text;
    }

    std::string ToolTips::getCountString(const int value)
    {
        if (value == 1)
            return "";
        else
            return " (" + MyGUI::utility::toString(value) + ")";
    }

    std::string ToolTips::getSoulString(const MWWorld::CellRef& cellref)
    {
        std::string soul = cellref.getSoul();
        if (soul.empty())
            return std::string();
        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        const ESM::Creature *creature = store.get<ESM::Creature>().search(soul);
        if (!creature)
            return std::string();
        if (creature->mName.empty())
            return " (" + creature->mId + ")";
        return " (" + creature->mName + ")";
    }

    std::string ToolTips::getCellRefString(const MWWorld::CellRef& cellref)
    {
        std::string ret;
        ret += getMiscString(cellref.getOwner(), "Owner");
        const std::string factionId = cellref.getFaction();
        if (!factionId.empty())
        {
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const ESM::Faction *fact = store.get<ESM::Faction>().search(factionId);
            if (fact != nullptr)
            {
                ret += getMiscString(fact->mName.empty() ? factionId : fact->mName, "Owner Faction");
                if (cellref.getFactionRank() >= 0)
                {
                    int rank = cellref.getFactionRank();
                    const std::string rankName = fact->mRanks[rank];
                    if (rankName.empty())
                        ret += getValueString(cellref.getFactionRank(), "Rank");
                    else
                        ret += getMiscString(rankName, "Rank");
                }
            }
        }

        std::vector<std::pair<std::string, int> > itemOwners =
                MWBase::Environment::get().getMechanicsManager()->getStolenItemOwners(cellref.getRefId());

        for (std::pair<std::string, int>& owner : itemOwners)
        {
            if (owner.second == std::numeric_limits<int>::max())
                ret += std::string("\nStolen from ") + owner.first; // for legacy (ESS) savegames
            else
                ret += std::string("\nStolen ") + MyGUI::utility::toString(owner.second) + " from " + owner.first;
        }

        ret += getMiscString(cellref.getGlobalVariable(), "Global");
        return ret;
    }

    std::string ToolTips::getDurationString(float duration, const std::string& prefix)
    {
        std::string ret;
        ret = prefix + ": ";

        if (duration < 1.f)
        {
            ret += "0 s";
            return ret;
        }

        constexpr int secondsPerMinute = 60; // 60 seconds
        constexpr int secondsPerHour = secondsPerMinute * 60; // 60 minutes
        constexpr int secondsPerDay = secondsPerHour * 24; // 24 hours
        constexpr int secondsPerMonth = secondsPerDay * 30; // 30 days
        constexpr int secondsPerYear = secondsPerDay * 365;
        int fullDuration = static_cast<int>(duration);
        int units = 0;
        int years = fullDuration / secondsPerYear;
        int months = fullDuration % secondsPerYear / secondsPerMonth;
        int days = fullDuration % secondsPerYear % secondsPerMonth / secondsPerDay; // Because a year is not exactly 12 "months"
        int hours = fullDuration % secondsPerDay / secondsPerHour;
        int minutes = fullDuration % secondsPerHour / secondsPerMinute;
        int seconds = fullDuration % secondsPerMinute;
        if (years)
        {
            units++;
            ret += toString(years) + " y ";
        }
        if (months)
        {
            units++;
            ret += toString(months) + " mo ";
        }
        if (units < 2 && days)
        {
            units++;
            ret += toString(days) + " d ";
        }
        if (units < 2 && hours)
        {
            units++;
            ret += toString(hours) + " h ";
        }
        if (units >= 2)
            return ret;
        if (minutes)
            ret += toString(minutes) + " min ";
        if (seconds)
            ret += toString(seconds) + " s ";

        return ret;
    }

    bool ToolTips::toggleFullHelp()
    {
        mFullHelp = !mFullHelp;
        return mFullHelp;
    }

    bool ToolTips::getFullHelp() const
    {
        return mFullHelp;
    }

    void ToolTips::setFocusObjectScreenCoords(float min_x, float min_y, float max_x, float max_y)
    {
        mFocusToolTipX = (min_x + max_x) / 2;
        mFocusToolTipY = min_y;
    }

    void ToolTips::createSkillToolTip(MyGUI::Widget* widget, int skillId)
    {
        if (skillId == -1)
            return;

        const MWWorld::ESMStore &store =
            MWBase::Environment::get().getWorld()->getStore();

        const std::string &skillNameId = ESM::Skill::sSkillNameIds[skillId];
        const ESM::Skill* skill = store.get<ESM::Skill>().find(skillId);
        assert(skill);

        const ESM::Attribute* attr =
            store.get<ESM::Attribute>().find(skill->mData.mAttribute);
        assert(attr);
        std::string icon = "icons\\k\\" + ESM::Skill::sIconNames[skillId];

        widget->setUserString("ToolTipType", "Layout");
        widget->setUserString("ToolTipLayout", "SkillNoProgressToolTip");
        widget->setUserString("Caption_SkillNoProgressName", "#{"+skillNameId+"}");
        widget->setUserString("Caption_SkillNoProgressDescription", skill->mDescription);
        widget->setUserString("Caption_SkillNoProgressAttribute", "#{sGoverningAttribute}: #{" + attr->mName + "}");
        widget->setUserString("ImageTexture_SkillNoProgressImage", icon);
    }

    void ToolTips::createAttributeToolTip(MyGUI::Widget* widget, int attributeId)
    {
        if (attributeId == -1)
            return;

        std::string icon = ESM::Attribute::sAttributeIcons[attributeId];
        std::string name = ESM::Attribute::sGmstAttributeIds[attributeId];
        std::string desc = ESM::Attribute::sGmstAttributeDescIds[attributeId];

        widget->setUserString("ToolTipType", "Layout");
        widget->setUserString("ToolTipLayout", "AttributeToolTip");
        widget->setUserString("Caption_AttributeName", "#{"+name+"}");
        widget->setUserString("Caption_AttributeDescription", "#{"+desc+"}");
        widget->setUserString("ImageTexture_AttributeImage", icon);
    }

    void ToolTips::createSpecializationToolTip(MyGUI::Widget* widget, const std::string& name, int specId)
    {
        widget->setUserString("Caption_Caption", name);
        std::string specText;
        // get all skills of this specialisation
        const MWWorld::Store<ESM::Skill> &skills =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Skill>();

        bool isFirst = true;
        for (auto& skillPair : skills)
        {
            if (skillPair.second.mData.mSpecialization == specId)
            {
                if (isFirst)
                    isFirst = false;
                else
                    specText += "\n";

                specText += std::string("#{") + ESM::Skill::sSkillNameIds[skillPair.first] + "}";
            }
        }
        widget->setUserString("Caption_ColumnText", specText);
        widget->setUserString("ToolTipLayout", "SpecializationToolTip");
        widget->setUserString("ToolTipType", "Layout");
    }

    void ToolTips::createBirthsignToolTip(MyGUI::Widget* widget, const std::string& birthsignId)
    {
        const MWWorld::ESMStore &store =
            MWBase::Environment::get().getWorld()->getStore();

        const ESM::BirthSign *sign = store.get<ESM::BirthSign>().find(birthsignId);

        widget->setUserString("ToolTipType", "Layout");
        widget->setUserString("ToolTipLayout", "BirthSignToolTip");
        widget->setUserString("ImageTexture_BirthSignImage", MWBase::Environment::get().getWindowManager()->correctTexturePath(sign->mTexture));
        std::string text;

        text += sign->mName;
        text += "\n#{fontcolourhtml=normal}" + sign->mDescription;

        std::vector<std::string> abilities, powers, spells;

        for (const std::string& spellId : sign->mPowers.mList)
        {
            const ESM::Spell *spell = store.get<ESM::Spell>().search(spellId);
            if (!spell)
                continue; // Skip spells which cannot be found
            ESM::Spell::SpellType type = static_cast<ESM::Spell::SpellType>(spell->mData.mType);
            if (type != ESM::Spell::ST_Spell && type != ESM::Spell::ST_Ability && type != ESM::Spell::ST_Power)
                continue; // We only want spell, ability and powers.

            if (type == ESM::Spell::ST_Ability)
                abilities.push_back(spellId);
            else if (type == ESM::Spell::ST_Power)
                powers.push_back(spellId);
            else if (type == ESM::Spell::ST_Spell)
                spells.push_back(spellId);
        }

        struct {
            const std::vector<std::string> &spells;
            std::string label;
        }
        categories[3] = {
            {abilities, "sBirthsignmenu1"},
            {powers,    "sPowers"},
            {spells,    "sBirthsignmenu2"}
        };

        for (int category = 0; category < 3; ++category)
        {
            bool addHeader = true;
            for (const std::string& spellId : categories[category].spells)
            {
                if (addHeader)
                {
                    text += std::string("\n\n#{fontcolourhtml=header}") + std::string("#{") + categories[category].label + "}";
                    addHeader = false;
                }

                const ESM::Spell *spell = store.get<ESM::Spell>().find(spellId);
                text += "\n#{fontcolourhtml=normal}" + spell->mName;
            }
        }

        widget->setUserString("Caption_BirthSignText", text);
    }

    void ToolTips::createRaceToolTip(MyGUI::Widget* widget, const ESM::Race* playerRace)
    {
        widget->setUserString("Caption_CenteredCaption", playerRace->mName);
        widget->setUserString("Caption_CenteredCaptionText", playerRace->mDescription);
        widget->setUserString("ToolTipType", "Layout");
        widget->setUserString("ToolTipLayout", "RaceToolTip");
    }

    void ToolTips::createClassToolTip(MyGUI::Widget* widget, const ESM::Class& playerClass)
    {
        if (playerClass.mName == "")
            return;

        int spec = playerClass.mData.mSpecialization;
        std::string specStr;
        if (spec == 0)
            specStr = "#{sSpecializationCombat}";
        else if (spec == 1)
            specStr = "#{sSpecializationMagic}";
        else if (spec == 2)
            specStr = "#{sSpecializationStealth}";

        widget->setUserString("Caption_ClassName", playerClass.mName);
        widget->setUserString("Caption_ClassDescription", playerClass.mDescription);
        widget->setUserString("Caption_ClassSpecialisation", "#{sSpecialization}: " + specStr);
        widget->setUserString("ToolTipType", "Layout");
        widget->setUserString("ToolTipLayout", "ClassToolTip");
    }

    void ToolTips::createMagicEffectToolTip(MyGUI::Widget* widget, short id)
    {
        const ESM::MagicEffect* effect =
            MWBase::Environment::get().getWorld ()->getStore ().get<ESM::MagicEffect>().find(id);
        const std::string &name = ESM::MagicEffect::effectIdToString (id);

        std::string icon = effect->mIcon;
        int slashPos = icon.rfind('\\');
        icon.insert(slashPos+1, "b_");
        icon = MWBase::Environment::get().getWindowManager()->correctIconPath(icon);

        widget->setUserString("ToolTipType", "Layout");
        widget->setUserString("ToolTipLayout", "MagicEffectToolTip");
        widget->setUserString("Caption_MagicEffectName", "#{" + name + "}");
        widget->setUserString("Caption_MagicEffectDescription", effect->mDescription);
        widget->setUserString("Caption_MagicEffectSchool", "#{sSchool}: " + sSchoolNames[effect->mData.mSchool]);
        widget->setUserString("ImageTexture_MagicEffectImage", icon);
    }

    void ToolTips::setDelay(float delay)
    {
        mDelay = delay;
        mRemainingDelay = mDelay;
    }

}
