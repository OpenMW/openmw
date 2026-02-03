#include "widgets.hpp"

#include <iomanip>

#include <MyGUI_Button.h>
#include <MyGUI_ControllerManager.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_ProgressBar.h>
#include <MyGUI_UString.h>

#include <components/esm/attr.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadspel.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "textcolours.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/magiceffects.hpp"

#include "../mwworld/esmstore.hpp"

namespace MWGui::Widgets
{
    /* MWSkill */

    MWSkill::MWSkill()
        : mSkillNameWidget(nullptr)
        , mSkillValueWidget(nullptr)
    {
    }

    void MWSkill::setSkillId(ESM::RefId skill)
    {
        mSkillId = skill;
        updateWidgets();
    }

    void MWSkill::setSkillValue(const SkillValue& value)
    {
        mValue = value;
        updateWidgets();
    }

    void MWSkill::updateWidgets()
    {
        if (mSkillNameWidget)
        {
            const ESM::Skill* skill = MWBase::Environment::get().getESMStore()->get<ESM::Skill>().search(mSkillId);
            if (skill == nullptr)
                mSkillNameWidget->setCaption({});
            else
                mSkillNameWidget->setCaption(skill->mName);
        }
        if (mSkillValueWidget)
        {
            SkillValue::Type modified = mValue.getModified(), base = mValue.getBase();
            mSkillValueWidget->setCaption(MyGUI::utility::toString(modified));
            if (modified > base)
                mSkillValueWidget->_setWidgetState("increased");
            else if (modified < base)
                mSkillValueWidget->_setWidgetState("decreased");
            else
                mSkillValueWidget->_setWidgetState("normal");
        }
    }

    void MWSkill::setStateSelected(bool selected)
    {
        const TextColours& textColours{ MWBase::Environment::get().getWindowManager()->getTextColours() };
        mSkillNameWidget->setTextColour(selected ? textColours.link : textColours.normal);
    }

    void MWSkill::onClicked(MyGUI::Widget* /*sender*/)
    {
        eventClicked(this);
    }

    MWSkill::~MWSkill() {}

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

        button = nullptr;
        assignWidget(button, "StatValueButton");
        if (button)
        {
            mSkillValueWidget = button;
            button->eventMouseButtonClick += MyGUI::newDelegate(this, &MWSkill::onClicked);
        }
    }

    /* MWAttribute */

    MWAttribute::MWAttribute()
        : mAttributeNameWidget(nullptr)
        , mAttributeValueWidget(nullptr)
    {
    }

    void MWAttribute::setAttributeId(ESM::RefId attributeId)
    {
        mId = attributeId;
        updateWidgets();
    }

    void MWAttribute::setAttributeValue(const AttributeValue& value)
    {
        mValue = value;
        updateWidgets();
    }

    void MWAttribute::onClicked(MyGUI::Widget* /*sender*/)
    {
        eventClicked(this);
    }

    void MWAttribute::updateWidgets()
    {
        if (mAttributeNameWidget)
        {
            const ESM::Attribute* attribute
                = MWBase::Environment::get().getESMStore()->get<ESM::Attribute>().search(mId);
            if (!attribute)
            {
                mAttributeNameWidget->setCaption({});
            }
            else
            {
                mAttributeNameWidget->setCaption(MyGUI::UString(attribute->mName));
            }
        }
        if (mAttributeValueWidget)
        {
            float modified = mValue.getModified();
            float base = mValue.getBase();
            mAttributeValueWidget->setCaption(MyGUI::utility::toString(static_cast<int>(modified)));
            if (modified > base)
                mAttributeValueWidget->_setWidgetState("increased");
            else if (modified < base)
                mAttributeValueWidget->_setWidgetState("decreased");
            else
                mAttributeValueWidget->_setWidgetState("normal");
        }
    }

    void MWAttribute::setStateSelected(bool selected)
    {
        const TextColours& textColours{ MWBase::Environment::get().getWindowManager()->getTextColours() };
        mAttributeNameWidget->setTextColour(selected ? textColours.link : textColours.normal);
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

        button = nullptr;
        assignWidget(button, "StatValueButton");
        if (button)
        {
            mAttributeValueWidget = button;
            button->eventMouseButtonClick += MyGUI::newDelegate(this, &MWAttribute::onClicked);
        }
    }

    /* MWSpell */

    MWSpell::MWSpell()
        : mSpellNameWidget(nullptr)
    {
    }

    void MWSpell::setSpellId(const ESM::RefId& spellId)
    {
        mId = spellId;
        updateWidgets();
    }

    void MWSpell::createEffectWidgets(
        std::vector<MyGUI::Widget*>& effects, MyGUI::Widget* creator, MyGUI::IntCoord& coord, int flags)
    {
        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();

        const ESM::Spell* spell = store.get<ESM::Spell>().search(mId);
        MYGUI_ASSERT(spell, "spell with id '" << mId << "' not found");

        for (const ESM::IndexedENAMstruct& effectInfo : spell->mEffects.mList)
        {
            MWSpellEffectPtr effect
                = creator->createWidget<MWSpellEffect>("MW_EffectImage", coord, MyGUI::Align::Default);
            SpellEffectParams params;
            params.mEffectID = effectInfo.mData.mEffectID;
            params.mSkill = effectInfo.mData.mSkill;
            params.mAttribute = effectInfo.mData.mAttribute;
            params.mDuration = effectInfo.mData.mDuration;
            params.mMagnMin = effectInfo.mData.mMagnMin;
            params.mMagnMax = effectInfo.mData.mMagnMax;
            params.mRange = effectInfo.mData.mRange;
            params.mIsConstant = (flags & MWEffectList::EF_Constant) != 0;
            params.mNoTarget = (flags & MWEffectList::EF_NoTarget);
            params.mNoMagnitude = (flags & MWEffectList::EF_NoMagnitude);
            effect->setSpellEffect(params);
            effects.push_back(effect);
            coord.top += effect->getHeight();
            coord.width = std::max(coord.width, effect->getRequestedWidth());
        }
    }

    void MWSpell::updateWidgets()
    {
        if (mSpellNameWidget && MWBase::Environment::get().getWindowManager())
        {
            const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();

            const ESM::Spell* spell = store.get<ESM::Spell>().search(mId);
            if (spell)
                mSpellNameWidget->setCaption(spell->mName);
            else
                mSpellNameWidget->setCaption({});
        }
    }

    void MWSpell::setStateSelected(bool selected)
    {
        const TextColours& textColours{ MWBase::Environment::get().getWindowManager()->getTextColours() };
        mSpellNameWidget->setTextColour(selected ? textColours.link : textColours.normal);
    }

    void MWSpell::initialiseOverride()
    {
        Base::initialiseOverride();

        assignWidget(mSpellNameWidget, "StatName");
    }

    MWSpell::~MWSpell() {}

    /* MWEffectList */

    MWEffectList::MWEffectList()
        : mEffectList(0)
    {
    }

    void MWEffectList::setEffectList(const SpellEffectList& list)
    {
        mEffectList = list;
        updateWidgets();
    }

    void MWEffectList::createEffectWidgets(
        std::vector<MyGUI::Widget*>& effects, MyGUI::Widget* creator, MyGUI::IntCoord& coord, bool center, int flags)
    {
        // We don't know the width of all the elements beforehand, so we do it in
        // 2 steps: first, create all widgets and check their width....
        MWSpellEffectPtr effect = nullptr;
        int maxwidth = coord.width;

        for (auto& effectInfo : mEffectList)
        {
            effect = creator->createWidget<MWSpellEffect>("MW_EffectImage", coord, MyGUI::Align::Default);
            effectInfo.mIsConstant = (flags & EF_Constant) || effectInfo.mIsConstant;
            effectInfo.mNoTarget = (flags & EF_NoTarget) || effectInfo.mNoTarget;
            effectInfo.mNoMagnitude = (flags & EF_NoMagnitude) || effectInfo.mNoMagnitude;
            effect->setSpellEffect(effectInfo);
            effects.push_back(effect);
            if (effect->getRequestedWidth() > maxwidth)
                maxwidth = effect->getRequestedWidth();

            coord.top += effect->getHeight();
        }

        // ... then adjust the size for all widgets
        for (MyGUI::Widget* effectWidget : effects)
        {
            effect = effectWidget->castType<MWSpellEffect>();
            bool needcenter = center && (maxwidth > effect->getRequestedWidth());
            int diff = maxwidth - effect->getRequestedWidth();
            if (needcenter)
            {
                effect->setCoord(
                    diff / 2, effect->getCoord().top, effect->getRequestedWidth(), effect->getCoord().height);
            }
            else
            {
                effect->setCoord(0, effect->getCoord().top, effect->getRequestedWidth(), effect->getCoord().height);
            }
        }

        // inform the parent about width
        coord.width = maxwidth;
    }

    void MWEffectList::updateWidgets() {}

    void MWEffectList::initialiseOverride()
    {
        Base::initialiseOverride();
    }

    MWEffectList::~MWEffectList() {}

    SpellEffectList MWEffectList::effectListFromESM(const ESM::EffectList* effects)
    {
        SpellEffectList result;
        for (const ESM::IndexedENAMstruct& effectInfo : effects->mList)
        {
            SpellEffectParams params;
            params.mEffectID = effectInfo.mData.mEffectID;
            params.mSkill = effectInfo.mData.mSkill;
            params.mAttribute = effectInfo.mData.mAttribute;
            params.mDuration = effectInfo.mData.mDuration;
            params.mMagnMin = effectInfo.mData.mMagnMin;
            params.mMagnMax = effectInfo.mData.mMagnMax;
            params.mRange = effectInfo.mData.mRange;
            params.mArea = effectInfo.mData.mArea;
            result.push_back(params);
        }
        return result;
    }

    /* MWSpellEffect */

    MWSpellEffect::MWSpellEffect()
        : mImageWidget(nullptr)
        , mTextWidget(nullptr)
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
            mTextWidget->setCaption("?");
            mTextWidget->setCoord(sIconOffset / 2, mTextWidget->getCoord().top, mTextWidget->getCoord().width,
                mTextWidget->getCoord().height); // Compensates for the missing image when effect is not known
            mRequestedWidth = mTextWidget->getTextSize().width + sIconOffset;
            mImageWidget->setImageTexture({});
            return;
        }

        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();

        const ESM::MagicEffect* magicEffect = store.get<ESM::MagicEffect>().find(mEffectParams.mEffectID);
        const ESM::Attribute* attribute = store.get<ESM::Attribute>().search(mEffectParams.mAttribute);
        const ESM::Skill* skill = store.get<ESM::Skill>().search(mEffectParams.mSkill);

        auto windowManager = MWBase::Environment::get().getWindowManager();

        std::string_view pt = windowManager->getGameSettingString("spoint", {});
        std::string_view pts = windowManager->getGameSettingString("spoints", {});
        std::string_view pct = windowManager->getGameSettingString("spercent", {});
        std::string_view ft = windowManager->getGameSettingString("sfeet", {});
        std::string_view lvl = windowManager->getGameSettingString("sLevel", {});
        std::string_view lvls = windowManager->getGameSettingString("sLevels", {});
        std::string to = " " + std::string{ windowManager->getGameSettingString("sTo", {}) } + " ";
        std::string sec = " " + std::string{ windowManager->getGameSettingString("ssecond", {}) };
        std::string secs = " " + std::string{ windowManager->getGameSettingString("sseconds", {}) };

        std::string spellLine = MWMechanics::getMagicEffectString(*magicEffect, attribute, skill);

        if ((mEffectParams.mMagnMin || mEffectParams.mMagnMax) && !mEffectParams.mNoMagnitude)
        {
            ESM::MagicEffect::MagnitudeDisplayType displayType = magicEffect->getMagnitudeDisplayType();
            if (displayType == ESM::MagicEffect::MDT_TimesInt)
            {
                std::string_view timesInt = windowManager->getGameSettingString("sXTimesINT", {});
                std::stringstream formatter;

                formatter << std::fixed << std::setprecision(1) << " " << (mEffectParams.mMagnMin / 10.0f);
                if (mEffectParams.mMagnMin != mEffectParams.mMagnMax)
                    formatter << to << (mEffectParams.mMagnMax / 10.0f);
                formatter << timesInt;

                spellLine += formatter.str();
            }
            else if (displayType != ESM::MagicEffect::MDT_None)
            {
                spellLine += " " + MyGUI::utility::toString(mEffectParams.mMagnMin);
                if (mEffectParams.mMagnMin != mEffectParams.mMagnMax)
                    spellLine += to + MyGUI::utility::toString(mEffectParams.mMagnMax);

                if (displayType == ESM::MagicEffect::MDT_Percentage)
                    spellLine += pct;
                else if (displayType == ESM::MagicEffect::MDT_Feet)
                {
                    spellLine += ' ';
                    spellLine += ft;
                }
                else if (displayType == ESM::MagicEffect::MDT_Level)
                {
                    spellLine += ' ';
                    if (mEffectParams.mMagnMin == mEffectParams.mMagnMax && std::abs(mEffectParams.mMagnMin) == 1)
                        spellLine += lvl;
                    else
                        spellLine += lvls;
                }
                else // ESM::MagicEffect::MDT_Points
                {
                    spellLine += ' ';
                    if (mEffectParams.mMagnMin == mEffectParams.mMagnMax && std::abs(mEffectParams.mMagnMin) == 1)
                        spellLine += pt;
                    else
                        spellLine += pts;
                }
            }
        }

        // constant effects have no duration and no target
        if (!mEffectParams.mIsConstant)
        {
            if (!(magicEffect->mData.mFlags & ESM::MagicEffect::AppliedOnce))
                mEffectParams.mDuration = std::max(1, mEffectParams.mDuration);

            if (mEffectParams.mDuration > 0 && !(magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration))
            {
                spellLine += ' ';
                spellLine += windowManager->getGameSettingString("sfor", {});
                spellLine += ' ' + MyGUI::utility::toString(mEffectParams.mDuration)
                    + ((mEffectParams.mDuration == 1) ? sec : secs);
            }

            if (mEffectParams.mArea > 0)
            {
                spellLine += " #{sin} " + MyGUI::utility::toString(mEffectParams.mArea) + " #{sfootarea}";
            }

            // potions have no target
            if (!mEffectParams.mNoTarget)
            {
                spellLine += ' ';
                spellLine += windowManager->getGameSettingString("sonword", {});
                spellLine += ' ';
                if (mEffectParams.mRange == ESM::RT_Self)
                    spellLine += windowManager->getGameSettingString("sRangeSelf", {});
                else if (mEffectParams.mRange == ESM::RT_Touch)
                    spellLine += windowManager->getGameSettingString("sRangeTouch", {});
                else if (mEffectParams.mRange == ESM::RT_Target)
                    spellLine += windowManager->getGameSettingString("sRangeTarget", {});
            }
        }

        mTextWidget->setCaptionWithReplacing(spellLine);
        mRequestedWidth = mTextWidget->getTextSize().width + sIconOffset;

        mImageWidget->setImageTexture(Misc::ResourceHelpers::correctIconPath(
            VFS::Path::toNormalized(magicEffect->mIcon), *MWBase::Environment::get().getResourceSystem()->getVFS()));
    }

    MWSpellEffect::~MWSpellEffect() {}

    void MWSpellEffect::setStateSelected(bool selected)
    {
        const TextColours& textColours{ MWBase::Environment::get().getWindowManager()->getTextColours() };
        mTextWidget->setTextColour(selected ? textColours.link : textColours.normal);
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
        , mTextWidget(nullptr)
        , mBarWidget(nullptr)
        , mBarTextWidget(nullptr)
    {
    }

    void MWDynamicStat::setValue(int cur, int max)
    {
        mValue = cur;
        mMax = max;

        if (mBarWidget)
        {
            mBarWidget->setProgressRange(std::max(0, mMax));
            mBarWidget->setProgressPosition(std::max(0, mValue));
        }

        if (mBarTextWidget)
        {
            std::stringstream out;
            out << mValue << "/" << mMax;
            mBarTextWidget->setCaption(out.str());
        }
    }
    void MWDynamicStat::setTitle(std::string_view text)
    {
        if (mTextWidget)
            mTextWidget->setCaption(MyGUI::UString(text));
    }

    MWDynamicStat::~MWDynamicStat() {}

    void MWDynamicStat::initialiseOverride()
    {
        Base::initialiseOverride();

        assignWidget(mTextWidget, "Text");
        assignWidget(mBarWidget, "Bar");
        assignWidget(mBarTextWidget, "BarText");
    }
}
