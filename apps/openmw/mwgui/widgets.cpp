#include "widgets.hpp"

#include <boost/lexical_cast.hpp>

#include <MyGUI_ProgressBar.h>
#include <MyGUI_ImageBox.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/esmstore.hpp"

#undef min
#undef max

using namespace MWGui;
using namespace MWGui::Widgets;

/* Helper functions */

/*
 * Fixes the filename of a texture path to use the correct .dds extension.
 * This is needed on some ESM entries which point to a .tga file instead.
 */
void MWGui::Widgets::fixTexturePath(std::string &path)
{
    int offset = path.rfind(".");
    if (offset < 0)
        return;
    path.replace(offset, path.length() - offset, ".dds");
}

/* MWSkill */

MWSkill::MWSkill()
    : mManager(NULL)
    , mSkillId(ESM::Skill::Length)
    , mSkillNameWidget(NULL)
    , mSkillValueWidget(NULL)
{
}

void MWSkill::setSkillId(ESM::Skill::SkillEnum skill)
{
    mSkillId = skill;
    updateWidgets();
}

void MWSkill::setSkillNumber(int skill)
{
    if (skill < 0)
        setSkillId(ESM::Skill::Length);
    else if (skill < ESM::Skill::Length)
        setSkillId(static_cast<ESM::Skill::SkillEnum>(skill));
    else
        throw new std::runtime_error("Skill number out of range");
}

void MWSkill::setSkillValue(const SkillValue& value)
{
    mValue = value;
    updateWidgets();
}

void MWSkill::updateWidgets()
{
    if (mSkillNameWidget && mManager)
    {
        if (mSkillId == ESM::Skill::Length)
        {
            static_cast<MyGUI::TextBox*>(mSkillNameWidget)->setCaption("");
        }
        else
        {
            const std::string &name = mManager->getGameSettingString(ESM::Skill::sSkillNameIds[mSkillId], "");
            static_cast<MyGUI::TextBox*>(mSkillNameWidget)->setCaption(name);
        }
    }
    if (mSkillValueWidget)
    {
        SkillValue::Type modified = mValue.getModified(), base = mValue.getBase();
        static_cast<MyGUI::TextBox*>(mSkillValueWidget)->setCaption(boost::lexical_cast<std::string>(modified));
        if (modified > base)
            mSkillValueWidget->_setWidgetState("increased");
        else if (modified < base)
            mSkillValueWidget->_setWidgetState("decreased");
        else
            mSkillValueWidget->_setWidgetState("normal");
    }
}

void MWSkill::onClicked(MyGUI::Widget* _sender)
{
    eventClicked(this);
}

MWSkill::~MWSkill()
{
}

void MWSkill::initialiseOverride()
{
    Base::initialiseOverride();

    assignWidget(mSkillNameWidget, "StatName");
    assignWidget(mSkillValueWidget, "StatValue");

    MyGUI::Button* button;
    assignWidget(button, "StatNameButton");
    if (button)
    {
        mSkillNameWidget = button;
        button->eventMouseButtonClick += MyGUI::newDelegate(this, &MWSkill::onClicked);
    }

    button = 0;
    assignWidget(button, "StatValueButton");
    if (button)
    {
        mSkillNameWidget = button;
        button->eventMouseButtonClick += MyGUI::newDelegate(this, &MWSkill::onClicked);
    }
}

/* MWAttribute */

MWAttribute::MWAttribute()
    : mManager(NULL)
    , mId(-1)
    , mAttributeNameWidget(NULL)
    , mAttributeValueWidget(NULL)
{
}

void MWAttribute::setAttributeId(int attributeId)
{
    mId = attributeId;
    updateWidgets();
}

void MWAttribute::setAttributeValue(const AttributeValue& value)
{
    mValue = value;
    updateWidgets();
}

void MWAttribute::onClicked(MyGUI::Widget* _sender)
{
    eventClicked(this);
}

void MWAttribute::updateWidgets()
{
    if (mAttributeNameWidget && mManager)
    {
        if (mId < 0 || mId >= 8)
        {
            static_cast<MyGUI::TextBox*>(mAttributeNameWidget)->setCaption("");
        }
        else
        {
            static const char *attributes[8] = {
                "sAttributeStrength",
                "sAttributeIntelligence",
                "sAttributeWillpower",
                "sAttributeAgility",
                "sAttributeSpeed",
                "sAttributeEndurance",
                "sAttributePersonality",
                "sAttributeLuck"
            };
            const std::string &name = mManager->getGameSettingString(attributes[mId], "");
            static_cast<MyGUI::TextBox*>(mAttributeNameWidget)->setCaption(name);
        }
    }
    if (mAttributeValueWidget)
    {
        AttributeValue::Type modified = mValue.getModified(), base = mValue.getBase();
        static_cast<MyGUI::TextBox*>(mAttributeValueWidget)->setCaption(boost::lexical_cast<std::string>(modified));
        if (modified > base)
            mAttributeValueWidget->_setWidgetState("increased");
        else if (modified < base)
            mAttributeValueWidget->_setWidgetState("decreased");
        else
            mAttributeValueWidget->_setWidgetState("normal");
    }
}

MWAttribute::~MWAttribute()
{
}

void MWAttribute::initialiseOverride()
{
    Base::initialiseOverride();

    assignWidget(mAttributeNameWidget, "StatName");
    assignWidget(mAttributeValueWidget, "StatValue");

    MyGUI::Button* button;
    assignWidget(button, "StatNameButton");
    if (button)
    {
        mAttributeNameWidget = button;
        button->eventMouseButtonClick += MyGUI::newDelegate(this, &MWAttribute::onClicked);
    }

    button = 0;
    assignWidget(button, "StatValueButton");
    if (button)
    {
        mAttributeValueWidget = button;
        button->eventMouseButtonClick += MyGUI::newDelegate(this, &MWAttribute::onClicked);
    }
}

/* MWSpell */

MWSpell::MWSpell()
    : mWindowManager(NULL)
    , mSpellNameWidget(NULL)
{
}

void MWSpell::setSpellId(const std::string &spellId)
{
    mId = spellId;
    updateWidgets();
}

void MWSpell::createEffectWidgets(std::vector<MyGUI::Widget*> &effects, MyGUI::Widget* creator, MyGUI::IntCoord &coord, int flags)
{
    const MWWorld::ESMStore &store =
        MWBase::Environment::get().getWorld()->getStore();

    const ESM::Spell *spell = store.get<ESM::Spell>().search(mId);
    MYGUI_ASSERT(spell, "spell with id '" << mId << "' not found");

    MWSpellEffectPtr effect = NULL;
    std::vector<ESM::ENAMstruct>::const_iterator end = spell->mEffects.mList.end();
    for (std::vector<ESM::ENAMstruct>::const_iterator it = spell->mEffects.mList.begin(); it != end; ++it)
    {
        effect = creator->createWidget<MWSpellEffect>("MW_EffectImage", coord, MyGUI::Align::Default);
        effect->setWindowManager(mWindowManager);
        SpellEffectParams params;
        params.mEffectID = it->mEffectID;
        params.mSkill = it->mSkill;
        params.mAttribute = it->mAttribute;
        params.mDuration = it->mDuration;
        params.mMagnMin = it->mMagnMin;
        params.mMagnMax = it->mMagnMax;
        params.mRange = it->mRange;
        params.mIsConstant = (flags & MWEffectList::EF_Constant);
        params.mNoTarget = (flags & MWEffectList::EF_NoTarget);
        effect->setSpellEffect(params);
        effects.push_back(effect);
        coord.top += effect->getHeight();
        coord.width = std::max(coord.width, effect->getRequestedWidth());
    }
}

void MWSpell::updateWidgets()
{
    if (mSpellNameWidget && mWindowManager)
    {
        const MWWorld::ESMStore &store =
            MWBase::Environment::get().getWorld()->getStore();

        const ESM::Spell *spell = store.get<ESM::Spell>().search(mId);
        if (spell)
            static_cast<MyGUI::TextBox*>(mSpellNameWidget)->setCaption(spell->mName);
        else
            static_cast<MyGUI::TextBox*>(mSpellNameWidget)->setCaption("");
    }
}

void MWSpell::initialiseOverride()
{
    Base::initialiseOverride();

    assignWidget(mSpellNameWidget, "StatName");
}

MWSpell::~MWSpell()
{
}

/* MWEffectList */

MWEffectList::MWEffectList()
    : mWindowManager(NULL)
    , mEffectList(0)
{
}

void MWEffectList::setEffectList(const SpellEffectList& list)
{
    mEffectList = list;
    updateWidgets();
}

void MWEffectList::createEffectWidgets(std::vector<MyGUI::Widget*> &effects, MyGUI::Widget* creator, MyGUI::IntCoord &coord, bool center, int flags)
{
    // We don't know the width of all the elements beforehand, so we do it in
    // 2 steps: first, create all widgets and check their width....
    MWSpellEffectPtr effect = NULL;
    int maxwidth = coord.width;

    for (SpellEffectList::iterator it=mEffectList.begin();
        it != mEffectList.end(); ++it)
    {
        effect = creator->createWidget<MWSpellEffect>("MW_EffectImage", coord, MyGUI::Align::Default);
        effect->setWindowManager(mWindowManager);
        it->mIsConstant = (flags & EF_Constant) || it->mIsConstant;
        it->mNoTarget = (flags & EF_NoTarget) || it->mNoTarget;
        effect->setSpellEffect(*it);
        effects.push_back(effect);
        if (effect->getRequestedWidth() > maxwidth)
            maxwidth = effect->getRequestedWidth();

        coord.top += effect->getHeight();
    }

    // ... then adjust the size for all widgets
    for (std::vector<MyGUI::Widget*>::iterator it = effects.begin(); it != effects.end(); ++it)
    {
        effect = static_cast<MWSpellEffectPtr>(*it);
        bool needcenter = center && (maxwidth > effect->getRequestedWidth());
        int diff = maxwidth - effect->getRequestedWidth();
        if (needcenter)
        {
            effect->setCoord(diff/2, effect->getCoord().top, effect->getRequestedWidth(), effect->getCoord().height);
        }
        else
        {
            effect->setCoord(0, effect->getCoord().top, effect->getRequestedWidth(), effect->getCoord().height);
        }
    }

    // inform the parent about width
    coord.width = maxwidth;
}

void MWEffectList::updateWidgets()
{
}

void MWEffectList::initialiseOverride()
{
    Base::initialiseOverride();
}

MWEffectList::~MWEffectList()
{
}

SpellEffectList MWEffectList::effectListFromESM(const ESM::EffectList* effects)
{
    SpellEffectList result;
    std::vector<ESM::ENAMstruct>::const_iterator end = effects->mList.end();
    for (std::vector<ESM::ENAMstruct>::const_iterator it = effects->mList.begin(); it != end; ++it)
    {
        SpellEffectParams params;
        params.mEffectID = it->mEffectID;
        params.mSkill = it->mSkill;
        params.mAttribute = it->mAttribute;
        params.mDuration = it->mDuration;
        params.mMagnMin = it->mMagnMin;
        params.mMagnMax = it->mMagnMax;
        params.mRange = it->mRange;
        params.mArea = it->mArea;
        result.push_back(params);
    }
    return result;
}

/* MWSpellEffect */

MWSpellEffect::MWSpellEffect()
    : mWindowManager(NULL)
    , mImageWidget(NULL)
    , mTextWidget(NULL)
    , mRequestedWidth(0)
{
}

void MWSpellEffect::setSpellEffect(const SpellEffectParams& params)
{
    mEffectParams = params;
    updateWidgets();
}

void MWSpellEffect::updateWidgets()
{
    if (!mEffectParams.mKnown)
    {
        mTextWidget->setCaption ("?");
        mRequestedWidth = mTextWidget->getTextSize().width + 24;
        mImageWidget->setImageTexture ("");
        return;
    }

    const MWWorld::ESMStore &store =
        MWBase::Environment::get().getWorld()->getStore();

    const ESM::MagicEffect *magicEffect =
        store.get<ESM::MagicEffect>().search(mEffectParams.mEffectID);

    assert(magicEffect);

    std::string pt =  mWindowManager->getGameSettingString("spoint", "");
    std::string pts =  mWindowManager->getGameSettingString("spoints", "");
    std::string to =  " " + mWindowManager->getGameSettingString("sTo", "") + " ";
    std::string sec =  " " + mWindowManager->getGameSettingString("ssecond", "");
    std::string secs =  " " + mWindowManager->getGameSettingString("sseconds", "");

    std::string effectIDStr = ESM::MagicEffect::effectIdToString(mEffectParams.mEffectID);
    std::string spellLine = mWindowManager->getGameSettingString(effectIDStr, "");

    if (magicEffect->mData.mFlags & ESM::MagicEffect::TargetSkill)
    {
        spellLine += " " + mWindowManager->getGameSettingString(ESM::Skill::sSkillNameIds[mEffectParams.mSkill], "");
    }
    if (magicEffect->mData.mFlags & ESM::MagicEffect::TargetAttribute)
    {
        spellLine += " " + mWindowManager->getGameSettingString(ESM::Attribute::sGmstAttributeIds[mEffectParams.mAttribute], "");
    }

    if ((mEffectParams.mMagnMin >= 0 || mEffectParams.mMagnMax >= 0) && !(magicEffect->mData.mFlags & ESM::MagicEffect::NoMagnitude))
    {
        if (mEffectParams.mMagnMin == mEffectParams.mMagnMax)
            spellLine += " " + boost::lexical_cast<std::string>(mEffectParams.mMagnMin) + " " + ((mEffectParams.mMagnMin == 1) ? pt : pts);
        else
        {
            spellLine += " " + boost::lexical_cast<std::string>(mEffectParams.mMagnMin) + to + boost::lexical_cast<std::string>(mEffectParams.mMagnMax) + " " + pts;
        }
    }

    // constant effects have no duration and no target
    if (!mEffectParams.mIsConstant)
    {
        if (mEffectParams.mDuration >= 0 && !(magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration))
        {
            spellLine += " " + mWindowManager->getGameSettingString("sfor", "") + " " + boost::lexical_cast<std::string>(mEffectParams.mDuration) + ((mEffectParams.mDuration == 1) ? sec : secs);
        }

        if (mEffectParams.mArea > 0)
        {
            spellLine += " #{sin} " + boost::lexical_cast<std::string>(mEffectParams.mArea) + " #{sfootarea}";
        }

        // potions have no target
        if (!mEffectParams.mNoTarget)
        {
            std::string on = mWindowManager->getGameSettingString("sonword", "");
            if (mEffectParams.mRange == ESM::RT_Self)
                spellLine += " " + on + " " + mWindowManager->getGameSettingString("sRangeSelf", "");
            else if (mEffectParams.mRange == ESM::RT_Touch)
                spellLine += " " + on + " " + mWindowManager->getGameSettingString("sRangeTouch", "");
            else if (mEffectParams.mRange == ESM::RT_Target)
                spellLine += " " + on + " " + mWindowManager->getGameSettingString("sRangeTarget", "");
        }
    }

    static_cast<MyGUI::TextBox*>(mTextWidget)->setCaptionWithReplacing(spellLine);
    mRequestedWidth = mTextWidget->getTextSize().width + 24;

    std::string path = std::string("icons\\") + magicEffect->mIcon;
    fixTexturePath(path);
    mImageWidget->setImageTexture(path);
}

MWSpellEffect::~MWSpellEffect()
{
}

void MWSpellEffect::initialiseOverride()
{
    Base::initialiseOverride();

    assignWidget(mTextWidget, "Text");
    assignWidget(mImageWidget, "Image");
}

/* MWDynamicStat */

MWDynamicStat::MWDynamicStat()
: mValue(0)
, mMax(1)
, mTextWidget(NULL)
, mBarWidget(NULL)
, mBarTextWidget(NULL)
{
}

void MWDynamicStat::setValue(int cur, int max)
{
    mValue = cur;
    mMax = max;

    if (mBarWidget)
    {
        mBarWidget->setProgressRange(mMax);
        mBarWidget->setProgressPosition(mValue);
    }


    if (mBarTextWidget)
    {
        if (mValue >= 0 && mMax > 0)
        {
            std::stringstream out;
            out << mValue << "/" << mMax;
            static_cast<MyGUI::TextBox*>(mBarTextWidget)->setCaption(out.str().c_str());
        }
        else
            static_cast<MyGUI::TextBox*>(mBarTextWidget)->setCaption("");
    }
}
void MWDynamicStat::setTitle(const std::string& text)
{
    if (mTextWidget)
        static_cast<MyGUI::TextBox*>(mTextWidget)->setCaption(text);
}

MWDynamicStat::~MWDynamicStat()
{
}

void MWDynamicStat::initialiseOverride()
{
    Base::initialiseOverride();

    assignWidget(mTextWidget, "Text");
    assignWidget(mBarWidget, "Bar");
    assignWidget(mBarTextWidget, "BarText");
}




// ---------------------------------------------------------------------------------------------------------------------

void AutoSizedWidget::notifySizeChange (MyGUI::Widget* w)
{
    if (w->getParent () != 0)
    {
        Box* b = dynamic_cast<Box*>(w->getParent());
        if (b)
            b->notifyChildrenSizeChanged ();
        else
        {
            if (mExpandDirection == MyGUI::Align::Left)
            {
                int hdiff = getRequestedSize ().width - w->getSize().width;
                w->setPosition(w->getPosition() - MyGUI::IntPoint(hdiff, 0));
            }
            w->setSize(getRequestedSize ());
        }
    }
}


MyGUI::IntSize AutoSizedTextBox::getRequestedSize()
{
    return getTextSize();
}

void AutoSizedTextBox::setCaption(const MyGUI::UString& _value)
{
    TextBox::setCaption(_value);

    notifySizeChange (this);
}

void AutoSizedTextBox::setPropertyOverride(const std::string& _key, const std::string& _value)
{
    if (_key == "ExpandDirection")
    {
        mExpandDirection = MyGUI::Align::parse (_value);
    }
    else
    {
        TextBox::setPropertyOverride (_key, _value);
    }
}

MyGUI::IntSize AutoSizedEditBox::getRequestedSize()
{
    if (getAlign().isHStretch())
        throw std::runtime_error("AutoSizedEditBox can't have HStretch align (" + getName() + ")");
    return MyGUI::IntSize(getSize().width, getTextSize().height);
}

void AutoSizedEditBox::setCaption(const MyGUI::UString& _value)
{
    EditBox::setCaption(_value);

    notifySizeChange (this);
}

void AutoSizedEditBox::setPropertyOverride(const std::string& _key, const std::string& _value)
{
    if (_key == "ExpandDirection")
    {
        mExpandDirection = MyGUI::Align::parse (_value);
    }
    else
    {
        EditBox::setPropertyOverride (_key, _value);
    }
}


MyGUI::IntSize AutoSizedButton::getRequestedSize()
{
    MyGUI::IntSize size = getTextSize() + MyGUI::IntSize(24,0);
    size.height = std::max(24, size.height);
    return size;
}

void AutoSizedButton::setCaption(const MyGUI::UString& _value)
{
    Button::setCaption(_value);

    notifySizeChange (this);
}

void AutoSizedButton::setPropertyOverride(const std::string& _key, const std::string& _value)
{
    if (_key == "ExpandDirection")
    {
        mExpandDirection = MyGUI::Align::parse (_value);
    }
    else
    {
        Button::setPropertyOverride (_key, _value);
    }
}

Box::Box()
    : mSpacing(4)
    , mPadding(0)
    , mAutoResize(false)
{

}

void Box::notifyChildrenSizeChanged ()
{
    align();
}

void Box::_setPropertyImpl(const std::string& _key, const std::string& _value)
{
    if (_key == "Spacing")
        mSpacing = MyGUI::utility::parseValue<int>(_value);
    else if (_key == "Padding")
        mPadding = MyGUI::utility::parseValue<int>(_value);
    else if (_key == "AutoResize")
        mAutoResize = MyGUI::utility::parseValue<bool>(_value);
}

void HBox::align ()
{
    unsigned int count = getChildCount ();
    size_t h_stretched_count = 0;
    int total_width = 0;
    int total_height = 0;
    std::vector< std::pair<MyGUI::IntSize, bool> > sizes;

    for (unsigned int i = 0; i < count; ++i)
    {
        MyGUI::Widget* w = getChildAt(i);
        bool hstretch = w->getUserString ("HStretch") == "true";
        h_stretched_count += hstretch;
        AutoSizedWidget* aw = dynamic_cast<AutoSizedWidget*>(w);
        if (aw)
        {
            sizes.push_back(std::make_pair(aw->getRequestedSize (), hstretch));
            total_width += aw->getRequestedSize ().width;
            total_height = std::max(total_height, aw->getRequestedSize ().height);
        }
        else
        {
            sizes.push_back (std::make_pair(w->getSize(), hstretch));
            total_width += w->getSize().width;
            if (!(w->getUserString("VStretch") == "true"))
                total_height = std::max(total_height, w->getSize().height);
        }

        if (i != count-1)
            total_width += mSpacing;
    }

    if (mAutoResize && (total_width+mPadding*2 != getSize().width || total_height+mPadding*2 != getSize().height))
    {
        setSize(MyGUI::IntSize(total_width+mPadding*2, total_height+mPadding*2));
        return;
    }


    int curX = 0;
    for (unsigned int i = 0; i < count; ++i)
    {
        if (i == 0)
            curX += mPadding;

        MyGUI::Widget* w = getChildAt(i);

        bool vstretch = w->getUserString ("VStretch") == "true";
        int height = vstretch ? total_height : sizes[i].first.height;

        MyGUI::IntCoord widgetCoord;
        widgetCoord.left = curX;
        widgetCoord.top = mPadding + (getSize().height-mPadding*2 - height) / 2;
        int width = sizes[i].second ? sizes[i].first.width + (getSize().width-mPadding*2 - total_width)/h_stretched_count
                                    : sizes[i].first.width;
        widgetCoord.width = width;
        widgetCoord.height = height;
        w->setCoord(widgetCoord);
        curX += width;

        if (i != count-1)
            curX += mSpacing;
    }
}

void HBox::setPropertyOverride(const std::string& _key, const std::string& _value)
{
    Box::_setPropertyImpl (_key, _value);
}

void HBox::setSize (const MyGUI::IntSize& _value)
{
    MyGUI::Widget::setSize (_value);
    align();
}

void HBox::setCoord (const MyGUI::IntCoord& _value)
{
    MyGUI::Widget::setCoord (_value);
    align();
}

void HBox::onWidgetCreated(MyGUI::Widget* _widget)
{
    align();
}

MyGUI::IntSize HBox::getRequestedSize ()
{
    MyGUI::IntSize size(0,0);
    for (unsigned int i = 0; i < getChildCount (); ++i)
    {
        AutoSizedWidget* w = dynamic_cast<AutoSizedWidget*>(getChildAt(i));
        if (w)
        {
            MyGUI::IntSize requested = w->getRequestedSize ();
            size.height = std::max(size.height, requested.height);
            size.width = size.width + requested.width;
            if (i != getChildCount()-1)
                size.width += mSpacing;
        }
        else
        {
            MyGUI::IntSize requested = getChildAt(i)->getSize ();
            size.height = std::max(size.height, requested.height);

            if (getChildAt(i)->getUserString("HStretch") != "true")
                size.width = size.width + requested.width;

            if (i != getChildCount()-1)
                size.width += mSpacing;
        }
        size.height += mPadding*2;
        size.width += mPadding*2;
    }
    return size;
}




void VBox::align ()
{
    unsigned int count = getChildCount ();
    size_t v_stretched_count = 0;
    int total_height = 0;
    int total_width = 0;
    std::vector< std::pair<MyGUI::IntSize, bool> > sizes;
    for (unsigned int i = 0; i < count; ++i)
    {
        MyGUI::Widget* w = getChildAt(i);
        bool vstretch = w->getUserString ("VStretch") == "true";
        v_stretched_count += vstretch;
        AutoSizedWidget* aw = dynamic_cast<AutoSizedWidget*>(w);
        if (aw)
        {
            sizes.push_back(std::make_pair(aw->getRequestedSize (), vstretch));
            total_height += aw->getRequestedSize ().height;
            total_width = std::max(total_width, aw->getRequestedSize ().width);
        }
        else
        {
            sizes.push_back (std::make_pair(w->getSize(), vstretch));
            total_height += w->getSize().height;

            if (!(w->getUserString("HStretch") == "true"))
                total_width = std::max(total_width, w->getSize().width);
        }

        if (i != count-1)
            total_height += mSpacing;
    }

    if (mAutoResize && (total_width+mPadding*2 != getSize().width || total_height+mPadding*2 != getSize().height))
    {
        setSize(MyGUI::IntSize(total_width+mPadding*2, total_height+mPadding*2));
        return;
    }


    int curY = 0;
    for (unsigned int i = 0; i < count; ++i)
    {
        if (i==0)
            curY += mPadding;

        MyGUI::Widget* w = getChildAt(i);

        bool hstretch = w->getUserString ("HStretch") == "true";
        int width = hstretch ? total_width : sizes[i].first.width;

        MyGUI::IntCoord widgetCoord;
        widgetCoord.top = curY;
        widgetCoord.left = mPadding + (getSize().width-mPadding*2 - width) / 2;
        int height = sizes[i].second ? sizes[i].first.height + (getSize().height-mPadding*2 - total_height)/v_stretched_count
                                    : sizes[i].first.height;
        widgetCoord.height = height;
        widgetCoord.width = width;
        w->setCoord(widgetCoord);
        curY += height;

        if (i != count-1)
            curY += mSpacing;
    }
}

void VBox::setPropertyOverride(const std::string& _key, const std::string& _value)
{
    Box::_setPropertyImpl (_key, _value);
}

void VBox::setSize (const MyGUI::IntSize& _value)
{
    MyGUI::Widget::setSize (_value);
    align();
}

void VBox::setCoord (const MyGUI::IntCoord& _value)
{
    MyGUI::Widget::setCoord (_value);
    align();
}

MyGUI::IntSize VBox::getRequestedSize ()
{
    MyGUI::IntSize size(0,0);
    for (unsigned int i = 0; i < getChildCount (); ++i)
    {
        AutoSizedWidget* w = dynamic_cast<AutoSizedWidget*>(getChildAt(i));
        if (w)
        {
            MyGUI::IntSize requested = w->getRequestedSize ();
            size.width = std::max(size.width, requested.width);
            size.height = size.height + requested.height;
            if (i != getChildCount()-1)
                size.height += mSpacing;
        }
        else
        {
            MyGUI::IntSize requested = getChildAt(i)->getSize ();
            size.width = std::max(size.width, requested.width);

            if (getChildAt(i)->getUserString("VStretch") != "true")
                size.height = size.height + requested.height;

            if (i != getChildCount()-1)
                size.height += mSpacing;
        }
        size.height += mPadding*2;
        size.width += mPadding*2;
    }
    return size;
}

void VBox::onWidgetCreated(MyGUI::Widget* _widget)
{
    align();
}
