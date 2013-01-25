#include "tooltips.hpp"

#include <boost/lexical_cast.hpp>

#include <OgreResourceGroupManager.h>

#include <components/settings/settings.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/class.hpp"

#include "map_window.hpp"
#include "widgets.hpp"
#include "inventorywindow.hpp"

using namespace MWGui;
using namespace MyGUI;

ToolTips::ToolTips(MWBase::WindowManager* windowManager) :
    Layout("openmw_tooltips.layout")
    , mGameMode(true)
    , mWindowManager(windowManager)
    , mFullHelp(false)
    , mEnabled(true)
    , mFocusToolTipX(0.0)
    , mFocusToolTipY(0.0)
    , mDelay(0.0)
    , mRemainingDelay(0.0)
    , mLastMouseX(0)
    , mLastMouseY(0)
    , mHorizontalScrollIndex(0)
{
    getWidget(mDynamicToolTipBox, "DynamicToolTipBox");

    mDynamicToolTipBox->setVisible(false);

    // turn off mouse focus so that getMouseFocusWidget returns the correct widget,
    // even if the mouse is over the tooltip
    mDynamicToolTipBox->setNeedMouseFocus(false);
    mMainWidget->setNeedMouseFocus(false);

    mDelay = Settings::Manager::getFloat("tooltip delay", "GUI");
    mRemainingDelay = mDelay;
}

void ToolTips::setEnabled(bool enabled)
{
    mEnabled = enabled;
}

void ToolTips::onFrame(float frameDuration)
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

    const IntSize &viewSize = RenderManager::getInstance().getViewSize();

    if (!mEnabled)
    {
        return;
    }

    if (!mGameMode)
    {
        const MyGUI::IntPoint& mousePos = InputManager::getInstance().getMousePosition();

        if (mWindowManager->getWorldMouseOver() && ((mWindowManager->getMode() == GM_Console)
            || (mWindowManager->getMode() == GM_Container)
            || (mWindowManager->getMode() == GM_Inventory)))
        {
            mFocusObject = MWBase::Environment::get().getWorld()->getFacedObject();

            if (mFocusObject.isEmpty ())
                return;

            MyGUI::IntSize tooltipSize = getToolTipViaPtr(true);

            IntPoint tooltipPosition = InputManager::getInstance().getMousePosition() + IntPoint(0, 24);

            // make the tooltip stay completely in the viewport
            if ((tooltipPosition.left + tooltipSize.width) > viewSize.width)
            {
                tooltipPosition.left = viewSize.width - tooltipSize.width;
            }
            if ((tooltipPosition.top + tooltipSize.height) > viewSize.height)
            {
                tooltipPosition.top = viewSize.height - tooltipSize.height;
            }

            setCoord(tooltipPosition.left, tooltipPosition.top, tooltipSize.width, tooltipSize.height);
        }

        else
        {
	    const MyGUI::IntPoint& lastPressed = InputManager::getInstance().getLastPressedPosition(MyGUI::MouseButton::Left);

            if (mousePos == lastPressed) // mouseclick makes tooltip disappear
                return;

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

            Widget* focus = InputManager::getInstance().getMouseFocusWidget();
            if (focus == 0)
            {
                return;
            }

            IntSize tooltipSize;

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
            std::string text = focus->getUserString("ToolTipText");

            if (type == "")
            {
                return;
            }
	    
	
            // special handling for markers on the local map: the tooltip should only be visible
            // if the marker is not hidden due to the fog of war.
            if (focus->getUserString ("IsMarker") == "true")
            {
                LocalMapBase::MarkerPosition pos = *focus->getUserData<LocalMapBase::MarkerPosition>();

                if (!MWBase::Environment::get().getWorld ()->isPositionExplored (pos.nX, pos.nY, pos.cellX, pos.cellY, pos.interior))
                    return;
            }

            if (type == "ItemPtr")
            {
                mFocusObject = *focus->getUserData<MWWorld::Ptr>();
                tooltipSize = getToolTipViaPtr(false);
            }
            else if (type == "AvatarItemSelection")
            {
                MyGUI::IntCoord avatarPos = mWindowManager->getInventoryWindow ()->getAvatarScreenCoord ();
                MyGUI::IntPoint relMousePos = MyGUI::InputManager::getInstance ().getMousePosition () - MyGUI::IntPoint(avatarPos.left, avatarPos.top);
                int realX = int(float(relMousePos.left) / float(avatarPos.width) * 512.f );
                int realY = int(float(relMousePos.top) / float(avatarPos.height) * 1024.f );
                MWWorld::Ptr item = mWindowManager->getInventoryWindow ()->getAvatarSelectedItem (realX, realY);

                mFocusObject = item;
                if (!mFocusObject.isEmpty ())
                    tooltipSize = getToolTipViaPtr(false);
            }
            else if (type == "Spell")
            {
                ToolTipInfo info;

                const ESM::Spell *spell =
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(focus->getUserString("Spell"));
                info.caption = spell->mName;
                Widgets::SpellEffectList effects;
                std::vector<ESM::ENAMstruct>::const_iterator end = spell->mEffects.mList.end();
                for (std::vector<ESM::ENAMstruct>::const_iterator it = spell->mEffects.mList.begin(); it != end; ++it)
                {
                    Widgets::SpellEffectParams params;
                    params.mEffectID = it->mEffectID;
                    params.mSkill = it->mSkill;
                    params.mAttribute = it->mAttribute;
                    params.mDuration = it->mDuration;
                    params.mMagnMin = it->mMagnMin;
                    params.mMagnMax = it->mMagnMax;
                    params.mRange = it->mRange;
                    params.mIsConstant = (spell->mData.mType == ESM::Spell::ST_Ability);
                    params.mNoTarget = false;
                    effects.push_back(params);
                }
                info.effects = effects;
                tooltipSize = createToolTip(info);
            }
            else if (type == "Layout")
            {
                // tooltip defined in the layout
                MyGUI::Widget* tooltip;
                getWidget(tooltip, focus->getUserString("ToolTipLayout"));

                tooltip->setVisible(true);
                if (!tooltip->isUserString("DontResize"))
                {
                    tooltip->setCoord(0, 0, 450, 300); // this is the maximum width of the tooltip before it starts word-wrapping

                    tooltipSize = MyGUI::IntSize(0, tooltip->getSize().height);
                }
                else
                    tooltipSize = tooltip->getSize();

                std::map<std::string, std::string> userStrings = focus->getUserStrings();
                for (std::map<std::string, std::string>::iterator it = userStrings.begin();
                    it != userStrings.end(); ++it)
                {
                    if (it->first == "ToolTipType"
                        || it->first == "ToolTipLayout"
                        || it->first == "IsMarker")
                        continue;


                    size_t underscorePos = it->first.find("_");
                    std::string propertyKey = it->first.substr(0, underscorePos);
                    std::string widgetName = it->first.substr(underscorePos+1, it->first.size()-(underscorePos+1));

                    MyGUI::Widget* w;
                    getWidget(w, widgetName);
                    w->setProperty(propertyKey, it->second);
                }

                for (unsigned int i=0; i<tooltip->getChildCount(); ++i)
                {
                    MyGUI::Widget* w = tooltip->getChildAt(i);

                    if (w->isUserString("AutoResizeHorizontal"))
                    {
                        MyGUI::TextBox* text = w->castType<MyGUI::TextBox>();
                        tooltipSize.width = std::max(tooltipSize.width, w->getLeft() + text->getTextSize().width + 8);
                    }
                    else if (!tooltip->isUserString("DontResize"))
                        tooltipSize.width = std::max(tooltipSize.width, w->getLeft() + w->getWidth() + 8);

                    if (w->isUserString("AutoResizeVertical"))
                    {
                        MyGUI::TextBox* text = w->castType<MyGUI::TextBox>();
                        int height = text->getTextSize().height;
                        if (height > w->getHeight())
                        {
                            tooltipSize += MyGUI::IntSize(0, height - w->getHeight());
                        }
                        if (height < w->getHeight())
                        {
                            tooltipSize -= MyGUI::IntSize(0, w->getHeight() - height);
                        }
                    }
                }
                tooltip->setCoord(0, 0, tooltipSize.width, tooltipSize.height);
            }
            else
                throw std::runtime_error ("unknown tooltip type");

            IntPoint tooltipPosition = InputManager::getInstance().getMousePosition() + IntPoint(0, 24);

            // make the tooltip stay completely in the viewport
            if ((tooltipPosition.left + tooltipSize.width) > viewSize.width)
            {
                tooltipPosition.left = viewSize.width - tooltipSize.width;
            }
            if ((tooltipPosition.top + tooltipSize.height) > viewSize.height)
            {
                tooltipPosition.top = viewSize.height - tooltipSize.height;
            }

            setCoord(tooltipPosition.left, tooltipPosition.top, tooltipSize.width, tooltipSize.height);
        }
    }
    else
    {
        if (!mFocusObject.isEmpty())
        {
            IntSize tooltipSize = getToolTipViaPtr();

            setCoord(viewSize.width/2 - tooltipSize.width/2,
                    std::max(0, int(mFocusToolTipY*viewSize.height - tooltipSize.height)),
                    tooltipSize.width,
                    tooltipSize.height);

            mDynamicToolTipBox->setVisible(true);
        }
    }
}

void ToolTips::enterGameMode()
{
    mGameMode = true;
}

void ToolTips::enterGuiMode()
{
    mGameMode = false;
}

void ToolTips::setFocusObject(const MWWorld::Ptr& focus)
{
    mFocusObject = focus;
}

IntSize ToolTips::getToolTipViaPtr (bool image)
{
    // this the maximum width of the tooltip before it starts word-wrapping
    setCoord(0, 0, 300, 300);

    IntSize tooltipSize;

    const MWWorld::Class& object = MWWorld::Class::get (mFocusObject);
    if (!object.hasToolTip(mFocusObject))
    {
        mDynamicToolTipBox->setVisible(false);
    }
    else
    {
        mDynamicToolTipBox->setVisible(true);

        ToolTipInfo info = object.getToolTipInfo(mFocusObject);
        if (!image)
            info.icon = "";
        tooltipSize = createToolTip(info);
    }

    return tooltipSize;
}

void ToolTips::findImageExtension(std::string& image)
{
    int len = image.size();
    if (len < 4) return;

    if (!Ogre::ResourceGroupManager::getSingleton().resourceExists(Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, image))
    {
        // Change texture extension to .dds
        image[len-3] = 'd';
        image[len-2] = 'd';
        image[len-1] = 's';
    }
}

IntSize ToolTips::createToolTip(const MWGui::ToolTipInfo& info)
{    
    mDynamicToolTipBox->setVisible(true);

    std::string caption = info.caption;
    std::string image = info.icon;
    int imageSize = (image != "") ? 32 : 0;
    std::string text = info.text;

    // remove the first newline (easier this way)
    if (text.size() > 0 && text[0] == '\n')
        text.erase(0, 1);

    if(caption.size() > 0 && isalnum(caption[0]))
        caption[0] = toupper(caption[0]);

    const ESM::Enchantment* enchant = 0;
    const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
    if (info.enchant != "")
    {
        enchant = store.get<ESM::Enchantment>().find(info.enchant);
        if (enchant->mData.mType == ESM::Enchantment::CastOnce)
            text += "\n#{sItemCastOnce}";
        else if (enchant->mData.mType == ESM::Enchantment::WhenStrikes)
            text += "\n#{sItemCastWhenStrikes}";
        else if (enchant->mData.mType == ESM::Enchantment::WhenUsed)
            text += "\n#{sItemCastWhenUsed}";
        else if (enchant->mData.mType == ESM::Enchantment::ConstantEffect)
            text += "\n#{sItemCastConstant}";
    }

    // this the maximum width of the tooltip before it starts word-wrapping
    setCoord(0, 0, 300, 300);

    const IntPoint padding(8, 8);
    
    const int maximumWidth = 500;

    const int imageCaptionHPadding = (caption != "" ? 8 : 0);
    const int imageCaptionVPadding = (caption != "" ? 4 : 0);

    std::string realImage = "icons\\" + image;
    findImageExtension(realImage);

    EditBox* captionWidget = mDynamicToolTipBox->createWidget<EditBox>("NormalText", IntCoord(0, 0, 300, 300), Align::Left | Align::Top, "ToolTipCaption");
    captionWidget->setProperty("Static", "true");
    captionWidget->setCaption(caption);
    IntSize captionSize = captionWidget->getTextSize();

    int captionHeight = std::max(caption != "" ? captionSize.height : 0, imageSize);

    EditBox* textWidget = mDynamicToolTipBox->createWidget<EditBox>("SandText", IntCoord(0, captionHeight+imageCaptionVPadding, 300, 300-captionHeight-imageCaptionVPadding), Align::Stretch, "ToolTipText");
    textWidget->setProperty("Static", "true");
    textWidget->setProperty("MultiLine", "true");
    textWidget->setProperty("WordWrap", "true");
    textWidget->setCaptionWithReplacing(text);
    textWidget->setTextAlign(Align::HCenter | Align::Top);
    IntSize textSize = textWidget->getTextSize();

    captionSize += IntSize(imageSize, 0); // adjust for image
    IntSize totalSize = IntSize( std::min(std::max(textSize.width,captionSize.width + ((image != "") ? imageCaptionHPadding : 0)),maximumWidth),
        ((text != "") ? textSize.height + imageCaptionVPadding : 0) + captionHeight );

    if (!info.effects.empty())
    {
        Widget* effectArea = mDynamicToolTipBox->createWidget<Widget>("",
            IntCoord(0, totalSize.height, 300, 300-totalSize.height),
            Align::Stretch, "ToolTipEffectArea");

        IntCoord coord(0, 6, totalSize.width, 24);

        /**
         * \todo
         * the various potion effects should appear in the tooltip depending if the player
         * has enough skill in alchemy to know about the effects of this potion.
         */

        Widgets::MWEffectListPtr effectsWidget = effectArea->createWidget<Widgets::MWEffectList>
            ("MW_StatName", coord, Align::Default, "ToolTipEffectsWidget");
        effectsWidget->setWindowManager(mWindowManager);
        effectsWidget->setEffectList(info.effects);

        std::vector<MyGUI::WidgetPtr> effectItems;
        effectsWidget->createEffectWidgets(effectItems, effectArea, coord, true, info.isPotion ? Widgets::MWEffectList::EF_NoTarget : 0);
        totalSize.height += coord.top-6;
        totalSize.width = std::max(totalSize.width, coord.width);
    }

    if (info.enchant != "")
    {
        assert(enchant);
        Widget* enchantArea = mDynamicToolTipBox->createWidget<Widget>("",
            IntCoord(0, totalSize.height, 300, 300-totalSize.height),
            Align::Stretch, "ToolTipEnchantArea");

        IntCoord coord(0, 6, totalSize.width, 24);

        Widgets::MWEffectListPtr enchantWidget = enchantArea->createWidget<Widgets::MWEffectList>
            ("MW_StatName", coord, Align::Default, "ToolTipEnchantWidget");
        enchantWidget->setWindowManager(mWindowManager);
        enchantWidget->setEffectList(Widgets::MWEffectList::effectListFromESM(&enchant->mEffects));

        std::vector<MyGUI::WidgetPtr> enchantEffectItems;
        int flag = (enchant->mData.mType == ESM::Enchantment::ConstantEffect) ? Widgets::MWEffectList::EF_Constant : 0;
        enchantWidget->createEffectWidgets(enchantEffectItems, enchantArea, coord, true, flag);
        totalSize.height += coord.top-6;
        totalSize.width = std::max(totalSize.width, coord.width);

        if (enchant->mData.mType == ESM::Enchantment::WhenStrikes
            || enchant->mData.mType == ESM::Enchantment::WhenUsed)
        {
            /// \todo store the current enchantment charge somewhere
            int charge = enchant->mData.mCharge;

            const int chargeWidth = 204;

            TextBox* chargeText = enchantArea->createWidget<TextBox>("SandText", IntCoord(0, 0, 10, 18), Align::Default, "ToolTipEnchantChargeText");
            chargeText->setCaptionWithReplacing("#{sCharges}");

            const int chargeTextWidth = chargeText->getTextSize().width + 5;

            const int chargeAndTextWidth = chargeWidth + chargeTextWidth;

            totalSize.width = std::max(totalSize.width, chargeAndTextWidth);

            chargeText->setCoord((totalSize.width - chargeAndTextWidth)/2, coord.top+6, chargeTextWidth, 18);

            IntCoord chargeCoord;
            if (totalSize.width < chargeWidth)
            {
                totalSize.width = chargeWidth;
                chargeCoord = IntCoord(0, coord.top+6, chargeWidth, 18);
            }
            else
            {
                chargeCoord = IntCoord((totalSize.width - chargeAndTextWidth)/2 + chargeTextWidth, coord.top+6, chargeWidth, 18);
            }
            Widgets::MWDynamicStatPtr chargeWidget = enchantArea->createWidget<Widgets::MWDynamicStat>
                ("MW_ChargeBar", chargeCoord, Align::Default, "ToolTipEnchantCharge");
            chargeWidget->setValue(charge, charge);
            totalSize.height += 24;
        }
    }

    captionWidget->setCoord( (totalSize.width - captionSize.width)/2 + imageSize,
        (captionHeight-captionSize.height)/2,
        captionSize.width-imageSize,
        captionSize.height);
    
     //if its too long we do hscroll with the caption
    if (captionSize.width > maximumWidth){
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
      captionWidget->setPosition (IntPoint(horizontal_scroll, captionWidget->getPosition().top + padding.top));
    } else {
      captionWidget->setPosition (captionWidget->getPosition() + padding);
    }

    textWidget->setPosition (textWidget->getPosition() + IntPoint(0, padding.top)); // only apply vertical padding, the horizontal works automatically due to Align::HCenter

    if (image != "")
    {
        ImageBox* imageWidget = mDynamicToolTipBox->createWidget<ImageBox>("ImageBox",
            IntCoord((totalSize.width - captionSize.width - imageCaptionHPadding)/2, 0, imageSize, imageSize),
            Align::Left | Align::Top, "ToolTipImage");
        imageWidget->setImageTexture(realImage);
        imageWidget->setPosition (imageWidget->getPosition() + padding);
    }

    totalSize += IntSize(padding.left*2, padding.top*2);

    return totalSize;
}

std::string ToolTips::toString(const float value)
{
    std::ostringstream stream;
    stream << std::setprecision(3) << value;
    return stream.str();
}

std::string ToolTips::toString(const int value)
{
    std::ostringstream stream;
    stream << value;
    return stream.str();
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
        return " (" + boost::lexical_cast<std::string>(value) + ")";
}

void ToolTips::toggleFullHelp()
{
    mFullHelp = !mFullHelp;
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
    widget->setUserString("Caption_CenteredCaption", name);
    std::string specText;
    // get all skills of this specialisation
    const MWWorld::Store<ESM::Skill> &skills =
        MWBase::Environment::get().getWorld()->getStore().get<ESM::Skill>();

    MWWorld::Store<ESM::Skill>::iterator it = skills.begin();
    for (; it != skills.end(); ++it)
    {
        if (it->mData.mSpecialization == specId)
            specText += std::string("\n#{") + ESM::Skill::sSkillNameIds[it->mIndex] + "}";
    }
    widget->setUserString("Caption_CenteredCaptionText", specText);
    widget->setUserString("ToolTipLayout", "TextWithCenteredCaptionToolTip");
    widget->setUserString("ToolTipType", "Layout");
}

void ToolTips::createBirthsignToolTip(MyGUI::Widget* widget, const std::string& birthsignId)
{
    const MWWorld::ESMStore &store =
        MWBase::Environment::get().getWorld()->getStore();

    const ESM::BirthSign *sign = store.get<ESM::BirthSign>().find(birthsignId);

    widget->setUserString("ToolTipType", "Layout");
    widget->setUserString("ToolTipLayout", "BirthSignToolTip");
    std::string image = sign->mTexture;
    image.replace(image.size()-3, 3, "dds");
    widget->setUserString("ImageTexture_BirthSignImage", "textures\\" + image);
    std::string text;

    text += sign->mName;
    text += "\n#BF9959" + sign->mDescription;

    std::vector<std::string> abilities, powers, spells;

    std::vector<std::string>::const_iterator it = sign->mPowers.mList.begin();
    std::vector<std::string>::const_iterator end = sign->mPowers.mList.end();
    for (; it != end; ++it)
    {
        const std::string &spellId = *it;
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
        for (std::vector<std::string>::const_iterator it = categories[category].spells.begin(); it != categories[category].spells.end(); ++it)
        {
            if (it == categories[category].spells.begin())
            {
                text += std::string("\n#DDC79E") + std::string("#{") + categories[category].label + "}";
            }

            const std::string &spellId = *it;

            const ESM::Spell *spell = store.get<ESM::Spell>().find(spellId);
            text += "\n#BF9959" + spell->mName;
        }
    }

    widget->setUserString("Caption_BirthSignText", text);
}

void ToolTips::createRaceToolTip(MyGUI::Widget* widget, const ESM::Race* playerRace)
{
    widget->setUserString("Caption_CenteredCaption", playerRace->mName);
    widget->setUserString("Caption_CenteredCaptionText", playerRace->mDescription);
    widget->setUserString("ToolTipType", "Layout");
    widget->setUserString("ToolTipLayout", "TextWithCenteredCaptionToolTip");
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

    int slashPos = icon.find("\\");
    icon.insert(slashPos+1, "b_");

    icon[icon.size()-3] = 'd';
    icon[icon.size()-2] = 'd';
    icon[icon.size()-1] = 's';

    icon = "icons\\" + icon;

    std::vector<std::string> schools;
    schools.push_back ("#{sSchoolAlteration}");
    schools.push_back ("#{sSchoolConjuration}");
    schools.push_back ("#{sSchoolDestruction}");
    schools.push_back ("#{sSchoolIllusion}");
    schools.push_back ("#{sSchoolMysticism}");
    schools.push_back ("#{sSchoolRestoration}");

    widget->setUserString("ToolTipType", "Layout");
    widget->setUserString("ToolTipLayout", "MagicEffectToolTip");
    widget->setUserString("Caption_MagicEffectName", "#{" + name + "}");
    widget->setUserString("Caption_MagicEffectDescription", effect->mDescription);
    widget->setUserString("Caption_MagicEffectSchool", "#{sSchool}: " + schools[effect->mData.mSchool]);
    widget->setUserString("ImageTexture_MagicEffectImage", icon);
}

void ToolTips::setDelay(float delay)
{
    mDelay = delay;
    mRemainingDelay = mDelay;
}
