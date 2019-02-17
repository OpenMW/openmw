#include "widgets.hpp"

#include <sstream>
#include <iomanip>

#include <MyGUI_ProgressBar.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_ControllerManager.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/esmstore.hpp"

#include "controllers.hpp"

namespace MWGui
{
    namespace Widgets
    {
        /* MWSkill */

        MWSkill::MWSkill()
            : mSkillId(ESM::Skill::Length)
            , mSkillNameWidget(nullptr)
            , mSkillValueWidget(nullptr)
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
                throw std::runtime_error("Skill number out of range");
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
                if (mSkillId == ESM::Skill::Length)
                {
                    mSkillNameWidget->setCaption("");
                }
                else
                {
                    const std::string &name = MWBase::Environment::get().getWindowManager()->getGameSettingString(ESM::Skill::sSkillNameIds[mSkillId], "");
                    mSkillNameWidget->setCaption(name);
                }
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
                mSkillValueWidget = button;
                button->eventMouseButtonClick += MyGUI::newDelegate(this, &MWSkill::onClicked);
            }
        }

        /* MWAttribute */

        MWAttribute::MWAttribute()
            : mId(-1)
            , mAttributeNameWidget(nullptr)
            , mAttributeValueWidget(nullptr)
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
            if (mAttributeNameWidget)
            {
                if (mId < 0 || mId >= 8)
                {
                    mAttributeNameWidget->setCaption("");
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
                    const std::string &name = MWBase::Environment::get().getWindowManager()->getGameSettingString(attributes[mId], "");
                    mAttributeNameWidget->setCaption(name);
                }
            }
            if (mAttributeValueWidget)
            {
                int modified = mValue.getModified(), base = mValue.getBase();
                mAttributeValueWidget->setCaption(MyGUI::utility::toString(modified));
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
            : mSpellNameWidget(nullptr)
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

            std::vector<ESM::ENAMstruct>::const_iterator end = spell->mEffects.mList.end();
            for (std::vector<ESM::ENAMstruct>::const_iterator it = spell->mEffects.mList.begin(); it != end; ++it)
            {
                MWSpellEffectPtr effect = creator->createWidget<MWSpellEffect>("MW_EffectImage", coord, MyGUI::Align::Default);
                SpellEffectParams params;
                params.mEffectID = it->mEffectID;
                params.mSkill = it->mSkill;
                params.mAttribute = it->mAttribute;
                params.mDuration = it->mDuration;
                params.mMagnMin = it->mMagnMin;
                params.mMagnMax = it->mMagnMax;
                params.mRange = it->mRange;
                params.mIsConstant = (flags & MWEffectList::EF_Constant) != 0;
                params.mNoTarget = (flags & MWEffectList::EF_NoTarget);
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
                const MWWorld::ESMStore &store =
                    MWBase::Environment::get().getWorld()->getStore();

                const ESM::Spell *spell = store.get<ESM::Spell>().search(mId);
                if (spell)
                    mSpellNameWidget->setCaption(spell->mName);
                else
                    mSpellNameWidget->setCaption("");
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
            : mEffectList(0)
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
            MWSpellEffectPtr effect = nullptr;
            int maxwidth = coord.width;

            for (SpellEffectList::iterator it=mEffectList.begin();
                it != mEffectList.end(); ++it)
            {
                effect = creator->createWidget<MWSpellEffect>("MW_EffectImage", coord, MyGUI::Align::Default);
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
                effect = (*it)->castType<MWSpellEffect>();
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

            std::string pt =  MWBase::Environment::get().getWindowManager()->getGameSettingString("spoint", "");
            std::string pts =  MWBase::Environment::get().getWindowManager()->getGameSettingString("spoints", "");
            std::string pct =  MWBase::Environment::get().getWindowManager()->getGameSettingString("spercent", "");
            std::string ft =  MWBase::Environment::get().getWindowManager()->getGameSettingString("sfeet", "");
            std::string lvl =  MWBase::Environment::get().getWindowManager()->getGameSettingString("sLevel", "");
            std::string lvls =  MWBase::Environment::get().getWindowManager()->getGameSettingString("sLevels", "");
            std::string to =  " " + MWBase::Environment::get().getWindowManager()->getGameSettingString("sTo", "") + " ";
            std::string sec =  " " + MWBase::Environment::get().getWindowManager()->getGameSettingString("ssecond", "");
            std::string secs =  " " + MWBase::Environment::get().getWindowManager()->getGameSettingString("sseconds", "");

            std::string effectIDStr = ESM::MagicEffect::effectIdToString(mEffectParams.mEffectID);
            std::string spellLine = MWBase::Environment::get().getWindowManager()->getGameSettingString(effectIDStr, "");

            if (magicEffect->mData.mFlags & ESM::MagicEffect::TargetSkill && mEffectParams.mSkill != -1)
            {
                spellLine += " " + MWBase::Environment::get().getWindowManager()->getGameSettingString(ESM::Skill::sSkillNameIds[mEffectParams.mSkill], "");
            }
            if (magicEffect->mData.mFlags & ESM::MagicEffect::TargetAttribute && mEffectParams.mAttribute != -1)
            {
                spellLine += " " + MWBase::Environment::get().getWindowManager()->getGameSettingString(ESM::Attribute::sGmstAttributeIds[mEffectParams.mAttribute], "");
            }

            if (mEffectParams.mMagnMin >= 0 || mEffectParams.mMagnMax >= 0) {
                ESM::MagicEffect::MagnitudeDisplayType displayType = magicEffect->getMagnitudeDisplayType();
                if ( displayType == ESM::MagicEffect::MDT_TimesInt ) {
                    std::string timesInt =  MWBase::Environment::get().getWindowManager()->getGameSettingString("sXTimesINT", "");
                    std::stringstream formatter;

                    formatter << std::fixed << std::setprecision(1) << " " << (mEffectParams.mMagnMin / 10.0f);
                    if (mEffectParams.mMagnMin != mEffectParams.mMagnMax)
                        formatter << to << (mEffectParams.mMagnMax / 10.0f);
                    formatter << timesInt;

                    spellLine += formatter.str();
                }
                else if ( displayType != ESM::MagicEffect::MDT_None ) {
                    spellLine += " " + MyGUI::utility::toString(mEffectParams.mMagnMin);
                    if (mEffectParams.mMagnMin != mEffectParams.mMagnMax)
                        spellLine += to + MyGUI::utility::toString(mEffectParams.mMagnMax);

                    if ( displayType == ESM::MagicEffect::MDT_Percentage )
                        spellLine += pct;
                    else if ( displayType == ESM::MagicEffect::MDT_Feet )
                        spellLine += " " + ft;
                    else if ( displayType == ESM::MagicEffect::MDT_Level )
                        spellLine += " " + ((mEffectParams.mMagnMin == 1 && mEffectParams.mMagnMax == 1) ? lvl : lvls );
                    else  // ESM::MagicEffect::MDT_Points
                        spellLine += " " + ((mEffectParams.mMagnMin == 1 && mEffectParams.mMagnMax == 1) ? pt : pts );
                }
            }

            // constant effects have no duration and no target
            if (!mEffectParams.mIsConstant)
            {
                if (mEffectParams.mDuration > 0 && !(magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration))
                {
                    spellLine += " " + MWBase::Environment::get().getWindowManager()->getGameSettingString("sfor", "") + " " + MyGUI::utility::toString(mEffectParams.mDuration) + ((mEffectParams.mDuration == 1) ? sec : secs);
                }

                if (mEffectParams.mArea > 0)
                {
                    spellLine += " #{sin} " + MyGUI::utility::toString(mEffectParams.mArea) + " #{sfootarea}";
                }

                // potions have no target
                if (!mEffectParams.mNoTarget)
                {
                    std::string on = MWBase::Environment::get().getWindowManager()->getGameSettingString("sonword", "");
                    if (mEffectParams.mRange == ESM::RT_Self)
                        spellLine += " " + on + " " + MWBase::Environment::get().getWindowManager()->getGameSettingString("sRangeSelf", "");
                    else if (mEffectParams.mRange == ESM::RT_Touch)
                        spellLine += " " + on + " " + MWBase::Environment::get().getWindowManager()->getGameSettingString("sRangeTouch", "");
                    else if (mEffectParams.mRange == ESM::RT_Target)
                        spellLine += " " + on + " " + MWBase::Environment::get().getWindowManager()->getGameSettingString("sRangeTarget", "");
                }
            }

            mTextWidget->setCaptionWithReplacing(spellLine);
            mRequestedWidth = mTextWidget->getTextSize().width + 24;

            mImageWidget->setImageTexture(MWBase::Environment::get().getWindowManager()->correctIconPath(magicEffect->mIcon));
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
                mBarTextWidget->setCaption(out.str().c_str());
            }
        }
        void MWDynamicStat::setTitle(const std::string& text)
        {
            if (mTextWidget)
                mTextWidget->setCaption(text);
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

        MWScrollBar::MWScrollBar()
          : mEnableRepeat(true)
          , mRepeatTriggerTime(0.5f)
          , mRepeatStepTime(0.1f)
          , mIsIncreasing(true)
        {
#if MYGUI_VERSION >= MYGUI_DEFINE_VERSION(3,2,2)
            ScrollBar::setRepeatEnabled(false);
#endif
        }

        MWScrollBar::~MWScrollBar()
        {
        }

        void MWScrollBar::initialiseOverride()
        {
            ScrollBar::initialiseOverride();

            if(mWidgetStart)
            {
                mWidgetStart->eventMouseButtonPressed += MyGUI::newDelegate(this, &MWScrollBar::onDecreaseButtonPressed);
                mWidgetStart->eventMouseButtonReleased += MyGUI::newDelegate(this, &MWScrollBar::onDecreaseButtonReleased);
            }
            if(mWidgetEnd)
            {
                mWidgetEnd->eventMouseButtonPressed += MyGUI::newDelegate(this, &MWScrollBar::onIncreaseButtonPressed);
                mWidgetEnd->eventMouseButtonReleased += MyGUI::newDelegate(this, &MWScrollBar::onIncreaseButtonReleased);
            }
        }

        void MWScrollBar::setRepeat(float trigger, float step)
        {
            mRepeatTriggerTime = trigger;
            mRepeatStepTime = step;
        }

        void MWScrollBar::repeatClick(MyGUI::Widget* _widget, MyGUI::ControllerItem* _controller)
        {
            int stepSize = mScrollPage;

            if(mIsIncreasing && mScrollPosition < mScrollRange-1)
            {
                if(mScrollPosition + stepSize > mScrollRange-1)
                    mScrollPosition = mScrollRange-1;
                else
                    mScrollPosition += stepSize;

                eventScrollChangePosition(this, mScrollPosition);
                updateTrack();
            }
            else if(!mIsIncreasing && mScrollPosition > 0)
            {
                int newPos = mScrollPosition - stepSize;
                if(newPos < 0)
                    mScrollPosition = 0;
                else
                    mScrollPosition -= stepSize;

                eventScrollChangePosition(this, mScrollPosition);
                updateTrack();
            }
        }

        void MWScrollBar::onDecreaseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id)
        {
            mIsIncreasing = false;
            MyGUI::ControllerItem* item = MyGUI::ControllerManager::getInstance().createItem(MWGui::Controllers::ControllerRepeatEvent::getClassTypeName());
            MWGui::Controllers::ControllerRepeatEvent* controller = item->castType<MWGui::Controllers::ControllerRepeatEvent>();
            controller->eventRepeatClick += newDelegate(this, &MWScrollBar::repeatClick);
            controller->setEnabled(mEnableRepeat);
            controller->setRepeat(mRepeatTriggerTime, mRepeatStepTime);
            MyGUI::ControllerManager::getInstance().addItem(this, controller);
        }

        void MWScrollBar::onDecreaseButtonReleased(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id)
        {
            MyGUI::ControllerManager::getInstance().removeItem(this);
        }

        void MWScrollBar::onIncreaseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id)
        {
            mIsIncreasing = true;
            MyGUI::ControllerItem* item = MyGUI::ControllerManager::getInstance().createItem(MWGui::Controllers::ControllerRepeatEvent::getClassTypeName());
            MWGui::Controllers::ControllerRepeatEvent* controller = item->castType<MWGui::Controllers::ControllerRepeatEvent>();
            controller->eventRepeatClick += newDelegate(this, &MWScrollBar::repeatClick);
            controller->setEnabled(mEnableRepeat);
            controller->setRepeat(mRepeatTriggerTime, mRepeatStepTime);
            MyGUI::ControllerManager::getInstance().addItem(this, controller);
        }

        void MWScrollBar::onIncreaseButtonReleased(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id)
        {
            MyGUI::ControllerManager::getInstance().removeItem(this);
        }
    }
}
