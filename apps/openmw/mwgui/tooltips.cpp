#include "tooltips.hpp"

#include <format>
#include <iomanip>

#include <MyGUI_Gui.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_TextIterator.h>
#include <MyGUI_UString.h>

#include <components/esm/records.hpp>
#include <components/l10n/manager.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/settings/values.hpp>
#include <components/widgets/box.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/spellutil.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "inventorywindow.hpp"
#include "mapwindow.hpp"

#include "itemmodel.hpp"

namespace MWGui
{
    ToolTips::ToolTips()
        : Layout("openmw_tooltips.layout")
        , mFocusToolTipX(0.0)
        , mFocusToolTipY(0.0)
        , mHorizontalScrollIndex(0)
        , mRemainingDelay(Settings::gui().mTooltipDelay)
        , mLastMouseX(0)
        , mLastMouseY(0)
        , mEnabled(true)
        , mFullHelp(false)
        , mFrameDuration(0.f)
    {
        getWidget(mDynamicToolTipBox, "DynamicToolTipBox");

        mDynamicToolTipBox->setVisible(false);

        // turn off mouse focus so that getMouseFocusWidget returns the correct widget,
        // even if the mouse is over the tooltip
        mDynamicToolTipBox->setNeedMouseFocus(false);
        mMainWidget->setNeedMouseFocus(false);

        for (size_t i = 0; i < mMainWidget->getChildCount(); ++i)
        {
            mMainWidget->getChildAt(i)->setVisible(false);
        }
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
        for (size_t i = 0; i < mMainWidget->getChildCount(); ++i)
        {
            mMainWidget->getChildAt(i)->setVisible(false);
        }

        const MyGUI::IntSize& viewSize = MyGUI::RenderManager::getInstance().getViewSize();

        if (!mEnabled)
        {
            return;
        }

        MWBase::WindowManager* winMgr = MWBase::Environment::get().getWindowManager();
        bool guiMode = winMgr->isGuiMode();

        if (guiMode)
        {
            if (!winMgr->getCursorVisible() && !winMgr->getControllerTooltipVisible())
                return;
            const MyGUI::IntPoint& mousePos = MyGUI::InputManager::getInstance().getMousePosition();

            if (winMgr->getWorldMouseOver()
                && (winMgr->isConsoleMode() || (winMgr->getMode() == GM_Container)
                    || (winMgr->getMode() == GM_Inventory)))
            {
                if (mFocusObject.isEmpty())
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
                        info.caption = mFocusObject.getCellRef().getRefId().toDebugString();
                    info.icon.clear();
                    tooltipSize = createToolTip(info, checkOwned());
                }
                else
                    tooltipSize = getToolTipViaPtr(mFocusObject.getCellRef().getCount(), true);

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
                    mRemainingDelay = Settings::gui().mTooltipDelay;
                }
                mLastMouseX = mousePos.left;
                mLastMouseY = mousePos.top;

                if (mRemainingDelay > 0)
                    return;

                MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getMouseFocusWidget();
                if (focus == nullptr)
                    return;

                MyGUI::IntSize tooltipSize;

                // try to go 1 level up until there is a widget that has tooltip
                // this is necessary because some skin elements are actually separate widgets
                while (!focus->isUserString("ToolTipType"))
                {
                    focus = focus->getParent();
                    if (!focus)
                        return;
                }

                std::string_view type = focus->getUserString("ToolTipType");

                if (type.empty())
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
                    if (mFocusObject.isEmpty())
                        return;

                    tooltipSize = getToolTipViaPtr(mFocusObject.getCellRef().getCount(), false, checkOwned());
                }
                else if (type == "ItemModelIndex")
                {
                    std::pair<ItemModel::ModelIndex, ItemModel*> pair
                        = *focus->getUserData<std::pair<ItemModel::ModelIndex, ItemModel*>>();
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
                    MyGUI::IntPoint relMousePos = MyGUI::InputManager::getInstance().getMousePosition()
                        - MyGUI::IntPoint(avatarPos.left, avatarPos.top);
                    MWWorld::Ptr item
                        = winMgr->getInventoryWindow()->getAvatarSelectedItem(relMousePos.left, relMousePos.top);

                    mFocusObject = item;
                    if (!mFocusObject.isEmpty())
                        tooltipSize = getToolTipViaPtr(mFocusObject.getCellRef().getCount(), false);
                }
                else if (type == "Spell")
                {
                    ToolTipInfo info;

                    const auto& store = MWBase::Environment::get().getESMStore();
                    const ESM::Spell* spell
                        = store->get<ESM::Spell>().find(ESM::RefId::deserialize(focus->getUserString("Spell")));
                    info.caption = spell->mName;
                    Widgets::SpellEffectList effects;
                    for (const ESM::IndexedENAMstruct& spellEffect : spell->mEffects.mList)
                    {
                        Widgets::SpellEffectParams params;
                        params.mEffectID = spellEffect.mData.mEffectID;
                        params.mSkill = ESM::Skill::indexToRefId(spellEffect.mData.mSkill);
                        params.mAttribute = ESM::Attribute::indexToRefId(spellEffect.mData.mAttribute);
                        params.mDuration = spellEffect.mData.mDuration;
                        params.mMagnMin = spellEffect.mData.mMagnMin;
                        params.mMagnMax = spellEffect.mData.mMagnMax;
                        params.mRange = spellEffect.mData.mRange;
                        params.mArea = spellEffect.mData.mArea;
                        params.mIsConstant = (spell->mData.mType == ESM::Spell::ST_Ability);
                        params.mNoTarget = false;
                        effects.push_back(params);
                    }
                    // display school of spells that contribute to skill progress
                    if (MWMechanics::spellIncreasesSkill(spell))
                    {
                        ESM::RefId id = MWMechanics::getSpellSchool(spell, MWMechanics::getPlayer());
                        if (!id.empty())
                        {
                            const auto& school = store->get<ESM::Skill>().find(id)->mSchool;
                            info.text = "#{sSchool}: " + MyGUI::TextIterator::toTagsString(school->mName).asUTF8();
                        }
                    }
                    if (focus->getUserString("SpellCost") == "true")
                        info.text
                            += MWGui::ToolTips::getValueString(MWMechanics::calcSpellCost(*spell), "#{sCastCost}");
                    info.effects = std::move(effects);
                    tooltipSize = createToolTip(info);
                }
                else if (type == "Layout")
                {
                    // tooltip defined in the layout
                    MyGUI::Widget* tooltip;
                    getWidget(tooltip, focus->getUserString("ToolTipLayout"));

                    tooltip->setVisible(true);

                    const auto& userStrings = focus->getUserStrings();
                    for (auto& userStringPair : userStrings)
                    {
                        size_t underscorePos = userStringPair.first.find('_');
                        if (underscorePos == std::string::npos)
                            continue;
                        std::string key = userStringPair.first.substr(0, underscorePos);
                        std::string_view first = userStringPair.first;
                        std::string_view widgetName = first.substr(underscorePos + 1);

                        type = "Property";
                        size_t caretPos = key.find('^');
                        if (caretPos != std::string::npos)
                        {
                            type = first.substr(0, caretPos);
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
                    throw std::runtime_error("unknown tooltip type");

                MyGUI::IntPoint tooltipPosition = MyGUI::InputManager::getInstance().getMousePosition();

                position(tooltipPosition, tooltipSize, viewSize);

                setCoord(tooltipPosition.left, tooltipPosition.top, tooltipSize.width, tooltipSize.height);
            }
        }
        else
        {
            if (!mFocusObject.isEmpty())
            {
                MyGUI::IntSize tooltipSize = getToolTipViaPtr(mFocusObject.getCellRef().getCount(), true, checkOwned());

                const int left = viewSize.width / 2 - tooltipSize.width / 2;
                const int top = std::max(0, int(mFocusToolTipY * viewSize.height - tooltipSize.height - 20));
                setCoord(left, top, tooltipSize.width, tooltipSize.height);

                mDynamicToolTipBox->setVisible(true);
            }
        }
    }

    void ToolTips::position(MyGUI::IntPoint& position, MyGUI::IntSize size, MyGUI::IntSize viewportSize)
    {
        position += MyGUI::IntPoint(0, 32)
            - MyGUI::IntPoint(static_cast<int>(MyGUI::InputManager::getInstance().getMousePosition().left
                                  / float(viewportSize.width) * size.width),
                0);

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

        for (size_t i = 0; i < mMainWidget->getChildCount(); ++i)
        {
            mMainWidget->getChildAt(i)->setVisible(false);
        }
    }

    void ToolTips::setFocusObject(const MWWorld::Ptr& focus)
    {
        mFocusObject = focus;

        update(mFrameDuration);
    }

    MyGUI::IntSize ToolTips::getToolTipViaPtr(int count, bool image, bool isOwned)
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
                info.icon.clear();
            tooltipSize = createToolTip(info, isOwned);
        }

        return tooltipSize;
    }

    bool ToolTips::checkOwned()
    {
        if (mFocusObject.isEmpty())
            return false;

        MWWorld::Ptr ptr = MWMechanics::getPlayer();
        MWWorld::Ptr victim;

        MWBase::MechanicsManager* mm = MWBase::Environment::get().getMechanicsManager();
        return !mm->isAllowedToUse(ptr, mFocusObject, victim);
    }

    MyGUI::IntSize ToolTips::createToolTip(const MWGui::ToolTipInfo& info, bool isOwned)
    {
        mDynamicToolTipBox->setVisible(true);

        const int showOwned = Settings::game().mShowOwned;
        if ((showOwned == 1 || showOwned == 3) && isOwned)
            mDynamicToolTipBox->changeWidgetSkin(MWBase::Environment::get().getWindowManager()->isGuiMode()
                    ? "HUD_Box_NoTransp_Owned"
                    : "HUD_Box_Owned");
        else
            mDynamicToolTipBox->changeWidgetSkin(
                MWBase::Environment::get().getWindowManager()->isGuiMode() ? "HUD_Box_NoTransp" : "HUD_Box");

        const std::string& caption = info.caption;
        const std::string& image = info.icon;
        int imageSize = (!image.empty()) ? info.imageSize : 0;
        std::string text = info.text;
        std::string_view extra = info.extra;

        // remove the first newline (easier this way)
        if (!text.empty() && text[0] == '\n')
            text.erase(0, 1);
        if (!extra.empty() && extra[0] == '\n')
            extra = extra.substr(1);

        const ESM::Enchantment* enchant = nullptr;
        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
        if (!info.enchant.empty())
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

        const int imageCaptionHPadding = !caption.empty() ? 8 : 0;
        const int imageCaptionVPadding = !caption.empty() ? 4 : 0;

        const int maximumWidth = MyGUI::RenderManager::getInstance().getViewSize().width - imageCaptionHPadding * 2;

        const std::string realImage
            = Misc::ResourceHelpers::correctIconPath(image, MWBase::Environment::get().getResourceSystem()->getVFS());

        Gui::EditBox* captionWidget = mDynamicToolTipBox->createWidget<Gui::EditBox>(
            "NormalText", MyGUI::IntCoord(0, 0, 300, 300), MyGUI::Align::Left | MyGUI::Align::Top, "ToolTipCaption");
        captionWidget->setEditStatic(true);
        captionWidget->setNeedKeyFocus(false);
        captionWidget->setCaptionWithReplacing(caption);
        MyGUI::IntSize captionSize = captionWidget->getTextSize();

        int captionHeight = std::max(!caption.empty() ? captionSize.height : 0, imageSize);

        Gui::EditBox* textWidget = mDynamicToolTipBox->createWidget<Gui::EditBox>("SandText",
            MyGUI::IntCoord(0, captionHeight + imageCaptionVPadding, 300, 300 - captionHeight - imageCaptionVPadding),
            MyGUI::Align::Stretch, "ToolTipText");
        textWidget->setEditStatic(true);
        textWidget->setEditMultiLine(true);
        textWidget->setEditWordWrap(info.wordWrap);
        textWidget->setCaptionWithReplacing(text);
        textWidget->setTextAlign(MyGUI::Align::HCenter | MyGUI::Align::Top);
        textWidget->setNeedKeyFocus(false);
        MyGUI::IntSize textSize = textWidget->getTextSize();

        captionSize += MyGUI::IntSize(imageSize, 0); // adjust for image
        MyGUI::IntSize totalSize = MyGUI::IntSize(
            std::min(std::max(textSize.width, captionSize.width + ((!image.empty()) ? imageCaptionHPadding : 0)),
                maximumWidth),
            (!text.empty() ? textSize.height + imageCaptionVPadding : 0) + captionHeight);

        for (const std::string& note : info.notes)
        {
            MyGUI::ImageBox* icon = mDynamicToolTipBox->createWidget<MyGUI::ImageBox>("MarkerButton",
                MyGUI::IntCoord(padding.left, totalSize.height + padding.top, 8, 8), MyGUI::Align::Default);
            icon->setColour(MyGUI::Colour(1.0f, 0.3f, 0.3f));
            Gui::EditBox* edit = mDynamicToolTipBox->createWidget<Gui::EditBox>("SandText",
                MyGUI::IntCoord(padding.left + 8 + 4, totalSize.height + padding.top, 300 - padding.left - 8 - 4,
                    300 - totalSize.height),
                MyGUI::Align::Default);
            constexpr size_t maxLength = 60;
            std::string shortenedNote = note.substr(0, std::min(maxLength, note.find('\n')));
            if (shortenedNote.size() < note.size())
                shortenedNote += " ...";
            edit->setCaption(shortenedNote);
            MyGUI::IntSize noteTextSize = edit->getTextSize();
            edit->setSize(std::max(edit->getWidth(), noteTextSize.width), noteTextSize.height);
            icon->setPosition(icon->getLeft(), (edit->getTop() + edit->getBottom()) / 2 - icon->getHeight() / 2);
            totalSize.height += std::max(edit->getHeight(), icon->getHeight());
            totalSize.width = std::max(totalSize.width, edit->getWidth() + 8 + 4);
        }

        if (!info.effects.empty())
        {
            MyGUI::Widget* effectArea = mDynamicToolTipBox->createWidget<MyGUI::Widget>({},
                MyGUI::IntCoord(padding.left, totalSize.height, 300 - padding.left, 300 - totalSize.height),
                MyGUI::Align::Stretch);

            MyGUI::IntCoord coord(0, 6, totalSize.width, 24);

            Widgets::MWEffectListPtr effectsWidget
                = effectArea->createWidget<Widgets::MWEffectList>("MW_StatName", coord, MyGUI::Align::Default);
            effectsWidget->setEffectList(info.effects);

            std::vector<MyGUI::Widget*> effectItems;
            int flag = info.isPotion ? Widgets::MWEffectList::EF_NoTarget : 0;
            flag |= info.isIngredient ? Widgets::MWEffectList::EF_NoMagnitude : 0;
            flag |= info.isIngredient ? Widgets::MWEffectList::EF_Constant : 0;
            effectsWidget->createEffectWidgets(
                effectItems, effectArea, coord, info.isPotion || info.isIngredient, flag);
            totalSize.height += coord.top - 6;
            totalSize.width = std::max(totalSize.width, coord.width);
        }

        if (enchant)
        {
            MyGUI::Widget* enchantArea = mDynamicToolTipBox->createWidget<MyGUI::Widget>({},
                MyGUI::IntCoord(padding.left, totalSize.height, 300 - padding.left, 300 - totalSize.height),
                MyGUI::Align::Stretch);

            MyGUI::IntCoord coord(0, 6, totalSize.width, 24);

            Widgets::MWEffectListPtr enchantWidget
                = enchantArea->createWidget<Widgets::MWEffectList>("MW_StatName", coord, MyGUI::Align::Default);
            enchantWidget->setEffectList(Widgets::MWEffectList::effectListFromESM(&enchant->mEffects));

            std::vector<MyGUI::Widget*> enchantEffectItems;
            int flag
                = (enchant->mData.mType == ESM::Enchantment::ConstantEffect) ? Widgets::MWEffectList::EF_Constant : 0;
            enchantWidget->createEffectWidgets(enchantEffectItems, enchantArea, coord, false, flag);
            totalSize.height += coord.top - 6;
            totalSize.width = std::max(totalSize.width, coord.width);

            if (enchant->mData.mType == ESM::Enchantment::WhenStrikes
                || enchant->mData.mType == ESM::Enchantment::WhenUsed)
            {
                const int maxCharge = MWMechanics::getEnchantmentCharge(*enchant);
                int charge = (info.remainingEnchantCharge == -1) ? maxCharge : info.remainingEnchantCharge;

                const int chargeWidth = 204;

                MyGUI::TextBox* chargeText = enchantArea->createWidget<MyGUI::TextBox>(
                    "SandText", MyGUI::IntCoord(0, 0, 10, 18), MyGUI::Align::Default, "ToolTipEnchantChargeText");
                chargeText->setCaptionWithReplacing("#{sCharges}");

                const int chargeTextWidth = chargeText->getTextSize().width + 5;

                const int chargeAndTextWidth = chargeWidth + chargeTextWidth;

                totalSize.width = std::max(totalSize.width, chargeAndTextWidth);

                chargeText->setCoord((totalSize.width - chargeAndTextWidth) / 2, coord.top + 6, chargeTextWidth, 18);

                MyGUI::IntCoord chargeCoord;
                if (totalSize.width < chargeWidth)
                {
                    totalSize.width = chargeWidth;
                    chargeCoord = MyGUI::IntCoord(0, coord.top + 6, chargeWidth, 18);
                }
                else
                {
                    chargeCoord = MyGUI::IntCoord(
                        (totalSize.width - chargeAndTextWidth) / 2 + chargeTextWidth, coord.top + 6, chargeWidth, 18);
                }
                Widgets::MWDynamicStatPtr chargeWidget = enchantArea->createWidget<Widgets::MWDynamicStat>(
                    "MW_ChargeBar", chargeCoord, MyGUI::Align::Default);
                chargeWidget->setValue(charge, maxCharge);
                totalSize.height += 24;
            }
        }

        if (!extra.empty())
        {
            Gui::EditBox* extraWidget = mDynamicToolTipBox->createWidget<Gui::EditBox>("SandText",
                MyGUI::IntCoord(padding.left, totalSize.height + 12, 300 - padding.left, 300 - totalSize.height),
                MyGUI::Align::Stretch, "ToolTipExtraText");

            extraWidget->setEditStatic(true);
            extraWidget->setEditMultiLine(true);
            extraWidget->setEditWordWrap(info.wordWrap);
            extraWidget->setCaptionWithReplacing(extra);
            extraWidget->setTextAlign(MyGUI::Align::HCenter | MyGUI::Align::Top);
            extraWidget->setNeedKeyFocus(false);

            MyGUI::IntSize extraTextSize = extraWidget->getTextSize();
            totalSize.height += extraTextSize.height + 4;
            totalSize.width = std::max(totalSize.width, extraTextSize.width);
        }

        captionWidget->setCoord((totalSize.width - captionSize.width) / 2 + imageSize,
            (captionHeight - captionSize.height) / 2, captionSize.width - imageSize, captionSize.height);

        // if its too long we do hscroll with the caption
        if (captionSize.width > maximumWidth)
        {
            mHorizontalScrollIndex = mHorizontalScrollIndex + 2;
            if (mHorizontalScrollIndex > captionSize.width)
            {
                mHorizontalScrollIndex = -totalSize.width;
            }
            int horizontalScroll = mHorizontalScrollIndex;
            if (horizontalScroll < 40)
            {
                horizontalScroll = 40;
            }
            else
            {
                horizontalScroll = 80 - mHorizontalScrollIndex;
            }
            captionWidget->setPosition(
                MyGUI::IntPoint(horizontalScroll, captionWidget->getPosition().top + padding.top));
        }
        else
        {
            captionWidget->setPosition(captionWidget->getPosition() + padding);
        }

        textWidget->setPosition(textWidget->getPosition()
            + MyGUI::IntPoint(0,
                padding.top)); // only apply vertical padding, the horizontal works automatically due to Align::HCenter

        if (!image.empty())
        {
            MyGUI::ImageBox* imageWidget = mDynamicToolTipBox->createWidget<MyGUI::ImageBox>("ImageBox",
                MyGUI::IntCoord(
                    (totalSize.width - captionSize.width - imageCaptionHPadding) / 2, 0, imageSize, imageSize),
                MyGUI::Align::Left | MyGUI::Align::Top);
            imageWidget->setImageTexture(realImage);
            imageWidget->setPosition(imageWidget->getPosition() + padding);
        }

        totalSize += MyGUI::IntSize(padding.left * 2, padding.top * 2);

        return totalSize;
    }

    std::string ToolTips::toString(const float value)
    {
        std::string s = std::format("{:.2f}", value);
        // Trim result so 1.00 turns into 1
        while (!s.empty() && s.back() == '0')
            s.pop_back();
        if (!s.empty() && s.back() == '.')
            s.pop_back();
        return s;
    }

    std::string ToolTips::toString(const int value)
    {
        return std::to_string(value);
    }

    std::string ToolTips::getWeightString(const float weight, std::string_view prefix)
    {
        if (weight == 0)
            return {};
        return std::format("\n{}: {}", prefix, toString(weight));
    }

    std::string ToolTips::getPercentString(const float value, std::string_view prefix)
    {
        if (value == 0)
            return {};
        return std::format("\n{}: {}%", prefix, toString(value * 100));
    }

    std::string ToolTips::getValueString(const int value, std::string_view prefix)
    {
        if (value == 0)
            return {};
        return std::format("\n{}: {}", prefix, value);
    }

    std::string ToolTips::getMiscString(std::string_view text, std::string_view prefix)
    {
        if (text.empty())
            return {};
        return std::format("\n{}: {}", prefix, text);
    }

    std::string ToolTips::getCountString(const int value)
    {
        if (value == 1)
            return {};
        return std::format(" ({})", value);
    }

    std::string ToolTips::getSoulString(const MWWorld::CellRef& cellref)
    {
        const ESM::RefId& soul = cellref.getSoul();
        if (soul.empty())
            return {};
        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
        const ESM::Creature* creature = store.get<ESM::Creature>().search(soul);
        if (!creature)
            return {};
        if (creature->mName.empty())
            return std::format(" ({})", creature->mId.toDebugString());
        return std::format(" ({})", creature->mName);
    }

    std::string ToolTips::getCellRefString(const MWWorld::CellRef& cellref)
    {
        std::string ret;
        ret += getMiscString(cellref.getOwner().getRefIdString(), "Owner");
        const ESM::RefId& factionId = cellref.getFaction();
        if (!factionId.empty())
        {
            const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
            const ESM::Faction* fact = store.get<ESM::Faction>().search(factionId);
            if (fact != nullptr)
            {
                ret += getMiscString(fact->mName.empty() ? factionId.getRefIdString() : fact->mName, "Owner Faction");
                if (cellref.getFactionRank() >= 0)
                {
                    int rank = cellref.getFactionRank();
                    const std::string& rankName = fact->mRanks[rank];
                    if (rankName.empty())
                        ret += getValueString(cellref.getFactionRank(), "Rank");
                    else
                        ret += getMiscString(rankName, "Rank");
                }
            }
        }

        std::vector<std::pair<ESM::RefId, int>> itemOwners
            = MWBase::Environment::get().getMechanicsManager()->getStolenItemOwners(cellref.getRefId());

        for (std::pair<ESM::RefId, int>& owner : itemOwners)
        {
            if (owner.second == std::numeric_limits<int>::max())
                ret += std::format("\nStolen from {}", owner.first.toDebugString()); // for legacy (ESS) savegames
            else
                ret += std::format("\nStolen {} from {}", owner.second, owner.first.toDebugString());
        }

        ret += getMiscString(cellref.getGlobalVariable(), "Global");
        return ret;
    }

    std::string ToolTips::getDurationString(float duration, std::string_view prefix)
    {
        auto l10n = MWBase::Environment::get().getL10nManager()->getContext("Interface");

        std::string ret(prefix);
        ret += ": ";

        if (duration < 1.f)
        {
            ret += l10n->formatMessage("DurationSecond", { "seconds" }, { 0 });
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
        int days = fullDuration % secondsPerYear % secondsPerMonth
            / secondsPerDay; // Because a year is not exactly 12 "months"
        int hours = fullDuration % secondsPerDay / secondsPerHour;
        int minutes = fullDuration % secondsPerHour / secondsPerMinute;
        int seconds = fullDuration % secondsPerMinute;
        if (years)
        {
            units++;
            ret += l10n->formatMessage("DurationYear", { "years" }, { years });
        }
        if (months)
        {
            units++;
            ret += l10n->formatMessage("DurationMonth", { "months" }, { months });
        }
        if (units < 2 && days)
        {
            units++;
            ret += l10n->formatMessage("DurationDay", { "days" }, { days });
        }
        if (units < 2 && hours)
        {
            units++;
            ret += l10n->formatMessage("DurationHour", { "hours" }, { hours });
        }
        if (units >= 2)
            return ret;
        if (minutes)
            ret += l10n->formatMessage("DurationMinute", { "minutes" }, { minutes });
        if (seconds)
            ret += l10n->formatMessage("DurationSecond", { "seconds" }, { seconds });

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

    void ToolTips::setFocusObjectScreenCoords(float x, float y)
    {
        mFocusToolTipX = x;
        mFocusToolTipY = y;
    }

    void ToolTips::createSkillToolTip(MyGUI::Widget* widget, ESM::RefId skillId)
    {
        if (skillId.empty())
            return;

        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
        const ESM::Skill* skill = store.get<ESM::Skill>().find(skillId);
        const ESM::Attribute* attr
            = store.get<ESM::Attribute>().find(ESM::Attribute::indexToRefId(skill->mData.mAttribute));

        widget->setUserString("ToolTipType", "Layout");
        widget->setUserString("ToolTipLayout", "SkillNoProgressToolTip");
        widget->setUserString("Caption_SkillNoProgressName", MyGUI::TextIterator::toTagsString(skill->mName));
        widget->setUserString("Caption_SkillNoProgressDescription", skill->mDescription);
        widget->setUserString("Caption_SkillNoProgressAttribute",
            "#{sGoverningAttribute}: " + MyGUI::TextIterator::toTagsString(attr->mName));
        widget->setUserString("ImageTexture_SkillNoProgressImage", skill->mIcon);
    }

    void ToolTips::createAttributeToolTip(MyGUI::Widget* widget, ESM::RefId attributeId)
    {
        const ESM::Attribute* attribute
            = MWBase::Environment::get().getESMStore()->get<ESM::Attribute>().search(attributeId);
        if (!attribute)
            return;

        widget->setUserString("ToolTipType", "Layout");
        widget->setUserString("ToolTipLayout", "AttributeToolTip");
        widget->setUserString("Caption_AttributeName", MyGUI::TextIterator::toTagsString(attribute->mName));
        widget->setUserString(
            "Caption_AttributeDescription", MyGUI::TextIterator::toTagsString(attribute->mDescription));
        widget->setUserString("ImageTexture_AttributeImage", attribute->mIcon);
    }

    void ToolTips::createSpecializationToolTip(MyGUI::Widget* widget, std::string_view name, int specId)
    {
        widget->setUserString("Caption_Caption", name);
        std::string specText;
        // get all skills of this specialisation
        const MWWorld::Store<ESM::Skill>& skills = MWBase::Environment::get().getESMStore()->get<ESM::Skill>();

        bool isFirst = true;
        for (const auto& skill : skills)
        {
            if (skill.mData.mSpecialization == specId)
            {
                if (isFirst)
                    isFirst = false;
                else
                    specText += "\n";

                specText += MyGUI::TextIterator::toTagsString(skill.mName);
            }
        }
        widget->setUserString("Caption_ColumnText", specText);
        widget->setUserString("ToolTipLayout", "SpecializationToolTip");
        widget->setUserString("ToolTipType", "Layout");
    }

    void ToolTips::createBirthsignToolTip(MyGUI::Widget* widget, const ESM::RefId& birthsignId)
    {
        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();

        const ESM::BirthSign* sign = store.get<ESM::BirthSign>().find(birthsignId);
        const VFS::Manager* const vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        widget->setUserString("ToolTipType", "Layout");
        widget->setUserString("ToolTipLayout", "BirthSignToolTip");
        widget->setUserString(
            "ImageTexture_BirthSignImage", Misc::ResourceHelpers::correctTexturePath(sign->mTexture, vfs));
        widget->setUserString("Caption_BirthSignName", sign->mName);
        widget->setUserString("Caption_BirthSignDescription", sign->mDescription);

        std::vector<const ESM::Spell*> abilities, powers, spells;

        for (const ESM::RefId& spellId : sign->mPowers.mList)
        {
            const ESM::Spell* spell = store.get<ESM::Spell>().search(spellId);
            if (!spell)
                continue; // Skip spells which cannot be found
            ESM::Spell::SpellType type = static_cast<ESM::Spell::SpellType>(spell->mData.mType);
            if (type != ESM::Spell::ST_Spell && type != ESM::Spell::ST_Ability && type != ESM::Spell::ST_Power)
                continue; // We only want spell, ability and powers.

            if (type == ESM::Spell::ST_Ability)
                abilities.push_back(spell);
            else if (type == ESM::Spell::ST_Power)
                powers.push_back(spell);
            else if (type == ESM::Spell::ST_Spell)
                spells.push_back(spell);
        }

        using Category = std::tuple<const std::vector<const ESM::Spell*>&, std::string_view, std::string_view>;
        std::initializer_list<Category> categories{ { abilities, "#{sBirthsignmenu1}", "Abilities" },
            { powers, "#{sPowers}", "Powers" }, { spells, "#{sBirthsignmenu2}", "Spells" } };

        for (const auto& [category, label, widgetName] : categories)
        {
            std::string text;
            if (!category.empty())
            {
                text = std::string(label) + "\n#{fontcolourhtml=normal}";
                for (const ESM::Spell* spell : category)
                    text += spell->mName + ' ';
                text.pop_back();
            }
            widget->setUserString("Caption_BirthSign" + std::string(widgetName), text);
        }
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
        if (playerClass.mName.empty())
            return;

        int spec = playerClass.mData.mSpecialization;
        std::string specStr = "#{";
        specStr += ESM::Class::sGmstSpecializationIds[spec];
        specStr += '}';

        widget->setUserString("Caption_ClassName", playerClass.mName);
        widget->setUserString("Caption_ClassDescription", playerClass.mDescription);
        widget->setUserString("Caption_ClassSpecialisation", "#{sSpecialization}: " + specStr);
        widget->setUserString("ToolTipType", "Layout");
        widget->setUserString("ToolTipLayout", "ClassToolTip");
    }

    void ToolTips::createMagicEffectToolTip(MyGUI::Widget* widget, short id)
    {
        const auto& store = MWBase::Environment::get().getESMStore();
        const ESM::MagicEffect* effect = store->get<ESM::MagicEffect>().find(id);
        const std::string& name = ESM::MagicEffect::indexToGmstString(id);

        std::string icon = effect->mIcon;
        int slashPos = icon.rfind('\\');
        icon.insert(slashPos + 1, "b_");
        icon = Misc::ResourceHelpers::correctIconPath(icon, MWBase::Environment::get().getResourceSystem()->getVFS());

        widget->setUserString("ToolTipType", "Layout");
        widget->setUserString("ToolTipLayout", "MagicEffectToolTip");
        widget->setUserString("Caption_MagicEffectName", "#{" + name + "}");
        widget->setUserString("Caption_MagicEffectDescription", effect->mDescription);
        widget->setUserString("Caption_MagicEffectSchool",
            "#{sSchool}: "
                + MyGUI::TextIterator::toTagsString(
                    store->get<ESM::Skill>().find(effect->mData.mSchool)->mSchool->mName));
        widget->setUserString("ImageTexture_MagicEffectImage", icon);
    }
}
