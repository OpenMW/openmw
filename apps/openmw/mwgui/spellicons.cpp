#include "spellicons.hpp"

#include <format>
#include <iomanip>
#include <sstream>

#include <MyGUI_ImageBox.h>

#include <components/esm3/loadmgef.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/creaturestats.hpp"

#include "tooltips.hpp"

namespace MWGui
{
    void SpellIcons::updateWidgets(MyGUI::Widget* parent, bool adjustSize)
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        const MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);

        std::map<ESM::RefId, std::vector<MagicEffectInfo>> effects;
        for (const auto& params : stats.getActiveSpells())
        {
            for (const auto& effect : params.getEffects())
            {
                if (!(effect.mFlags & ESM::ActiveEffect::Flag_Applied))
                    continue;
                MagicEffectInfo newEffectSource;
                newEffectSource.mKey = MWMechanics::EffectKey(effect.mEffectId, effect.getSkillOrAttribute());
                newEffectSource.mMagnitude = static_cast<int>(effect.mMagnitude);
                newEffectSource.mPermanent = effect.mDuration == -1.f;
                newEffectSource.mRemainingTime = effect.mTimeLeft;
                newEffectSource.mSource = params.getDisplayName();
                newEffectSource.mTotalTime = effect.mDuration;
                effects[effect.mEffectId].push_back(std::move(newEffectSource));
            }
        }

        int w = 2;
        const auto& store = MWBase::Environment::get().getESMStore();
        for (const auto& [effectId, effectInfos] : effects)
        {
            const ESM::MagicEffect* effect = store->get<ESM::MagicEffect>().find(effectId);

            float remainingDuration = 0;
            float totalDuration = 0;

            std::string sourcesDescription;

            static const float fadeTime
                = store->get<ESM::GameSetting>().find("fMagicStartIconBlink")->mValue.getFloat();

            bool addNewLine = false;
            for (const MagicEffectInfo& effectInfo : effectInfos)
            {
                if (addNewLine)
                    sourcesDescription += '\n';

                // if at least one of the effect sources is permanent, the effect will never wear off
                if (effectInfo.mPermanent)
                {
                    remainingDuration = fadeTime;
                    totalDuration = fadeTime;
                }
                else
                {
                    remainingDuration = std::max(remainingDuration, effectInfo.mRemainingTime);
                    totalDuration = std::max(totalDuration, effectInfo.mTotalTime);
                }

                sourcesDescription += effectInfo.mSource;

                if (effect->mData.mFlags & ESM::MagicEffect::TargetSkill)
                {
                    const ESM::Skill* skill = store->get<ESM::Skill>().find(effectInfo.mKey.mArg);
                    sourcesDescription += " (" + skill->mName + ')';
                }
                if (effect->mData.mFlags & ESM::MagicEffect::TargetAttribute)
                {
                    const ESM::Attribute* attribute = store->get<ESM::Attribute>().find(effectInfo.mKey.mArg);
                    sourcesDescription += " (" + attribute->mName + ')';
                }
                ESM::MagicEffect::MagnitudeDisplayType displayType = effect->getMagnitudeDisplayType();
                if (displayType == ESM::MagicEffect::MDT_TimesInt)
                {
                    std::string_view timesInt
                        = MWBase::Environment::get().getWindowManager()->getGameSettingString("sXTimesINT", {});
                    std::stringstream formatter;
                    formatter << std::fixed << std::setprecision(1) << " " << (effectInfo.mMagnitude / 10.0f)
                              << timesInt;
                    sourcesDescription += formatter.str();
                }
                else if (displayType != ESM::MagicEffect::MDT_None)
                {
                    sourcesDescription += ": " + MyGUI::utility::toString(effectInfo.mMagnitude);

                    if (displayType == ESM::MagicEffect::MDT_Percentage)
                        sourcesDescription
                            += MWBase::Environment::get().getWindowManager()->getGameSettingString("spercent", {});
                    else if (displayType == ESM::MagicEffect::MDT_Feet)
                    {
                        sourcesDescription += ' ';
                        sourcesDescription
                            += MWBase::Environment::get().getWindowManager()->getGameSettingString("sfeet", {});
                    }
                    else if (displayType == ESM::MagicEffect::MDT_Level)
                    {
                        sourcesDescription += ' ';
                        if (effectInfo.mMagnitude > 1)
                            sourcesDescription
                                += MWBase::Environment::get().getWindowManager()->getGameSettingString("sLevels", {});
                        else
                            sourcesDescription
                                += MWBase::Environment::get().getWindowManager()->getGameSettingString("sLevel", {});
                    }
                    else // ESM::MagicEffect::MDT_Points
                    {
                        sourcesDescription += ' ';
                        if (effectInfo.mMagnitude > 1)
                            sourcesDescription
                                += MWBase::Environment::get().getWindowManager()->getGameSettingString("spoints", {});
                        else
                            sourcesDescription
                                += MWBase::Environment::get().getWindowManager()->getGameSettingString("spoint", {});
                    }
                }
                if (effectInfo.mRemainingTime > -1 && Settings::game().mShowEffectDuration)
                    sourcesDescription
                        += MWGui::ToolTips::getDurationString(effectInfo.mRemainingTime, " #{sDuration}");

                addNewLine = true;
            }

            if (remainingDuration > 0.f)
            {
                MyGUI::ImageBox* image;
                if (mWidgetMap.find(effectId) == mWidgetMap.end())
                {
                    image = parent->createWidget<MyGUI::ImageBox>(
                        "ImageBox", MyGUI::IntCoord(w, 2, 16, 16), MyGUI::Align::Default);
                    mWidgetMap[effectId] = image;

                    image->setImageTexture(
                        Misc::ResourceHelpers::correctIconPath(VFS::Path::toNormalized(effect->mIcon),
                            *MWBase::Environment::get().getResourceSystem()->getVFS()));

                    ToolTipInfo tooltipInfo;
                    tooltipInfo.caption = std::format("#{{{}}}", ESM::MagicEffect::refIdToGmstString(effectId));
                    tooltipInfo.icon = effect->mIcon;
                    tooltipInfo.imageSize = 16;
                    tooltipInfo.wordWrap = false;

                    image->setUserData(tooltipInfo);
                    image->setUserString("ToolTipType", "ToolTipInfo");
                }
                else
                    image = mWidgetMap[effectId];

                image->setPosition(w, 2);
                image->setVisible(true);
                w += 16;

                ToolTipInfo* tooltipInfo = image->getUserData<ToolTipInfo>();
                tooltipInfo->text = std::move(sourcesDescription);

                // Fade out
                if (totalDuration >= fadeTime && fadeTime > 0.f)
                    image->setAlpha(std::min(remainingDuration / fadeTime, 1.f));
            }
            else if (mWidgetMap.find(effectId) != mWidgetMap.end())
            {
                MyGUI::ImageBox* image = mWidgetMap[effectId];
                image->setVisible(false);
                image->setAlpha(1.f);
            }
        }

        if (adjustSize)
        {
            int s = w + 2;
            if (effects.empty())
                s = 0;
            int diff = parent->getWidth() - s;
            parent->setSize(s, parent->getHeight());
            parent->setPosition(parent->getLeft() + diff, parent->getTop());
        }

        // hide inactive effects
        for (auto& widgetPair : mWidgetMap)
        {
            if (effects.find(widgetPair.first) == effects.end())
                widgetPair.second->setVisible(false);
        }
    }

}
