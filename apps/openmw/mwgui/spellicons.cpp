#include "spellicons.hpp"

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
    namespace
    {
        MyGUI::ImageBox* createIcon(MyGUI::Widget& parent, std::string_view name, std::string_view icon, int size)
        {
            const VFS::Manager& vfs = *MWBase::Environment::get().getResourceSystem()->getVFS();

            const MyGUI::IntCoord coord(0, 0, size, size);
            MyGUI::ImageBox* widget = parent.createWidget<MyGUI::ImageBox>("ImageBox", coord, MyGUI::Align::Default);
            widget->setImageTexture(Misc::ResourceHelpers::correctIconPath(VFS::Path::toNormalized(icon), vfs));

            ToolTipInfo tooltipInfo;
            tooltipInfo.caption = name;
            tooltipInfo.icon = icon;
            tooltipInfo.imageSize = size;
            tooltipInfo.wordWrap = false;

            widget->setUserData(tooltipInfo);
            widget->setUserString("ToolTipType", "ToolTipInfo");
            widget->setVisible(false);

            return widget;
        }

        std::string printEffectMagnitude(float srcMagnitude, ESM::MagicEffect::MagnitudeDisplayType displayType)
        {
            std::string result;
            if (displayType == ESM::MagicEffect::MDT_None)
                return result;

            const int magnitude = static_cast<int>(srcMagnitude);
            if (displayType == ESM::MagicEffect::MDT_TimesInt)
            {
                std::stringstream formatter;
                formatter << std::fixed << std::setprecision(1) << " " << (magnitude / 10.0f);
                result += formatter.str();
            }
            else
            {
                result += ": " + MyGUI::utility::toString(magnitude);
                if (displayType != ESM::MagicEffect::MDT_Percentage)
                    result += ' ';
            }

            std::string_view unit;
            if (displayType == ESM::MagicEffect::MDT_TimesInt)
                unit = "sXTimesINT";
            else if (displayType == ESM::MagicEffect::MDT_Percentage)
                unit = "sPercent";
            else if (displayType == ESM::MagicEffect::MDT_Feet)
                unit = "sFeet";
            else if (displayType == ESM::MagicEffect::MDT_Level)
                unit = magnitude > 1 ? "sLevels" : "sLevel";
            else if (displayType == ESM::MagicEffect::MDT_Points)
                unit = magnitude > 1 ? "sPoints" : "sPoint";

            result += MWBase::Environment::get().getWindowManager()->getGameSettingString(unit, {});

            return result;
        }
    }

    void SpellIcons::updateWidgets(MyGUI::Widget* parent, bool adjustSize)
    {
        for (auto& [effectId, widget] : mWidgetMap)
        {
            widget->setVisible(false);
            widget->setAlpha(1.f);
            widget->getUserData<ToolTipInfo>()->text.clear();
        }

        int horizontalOffset = 2;
        constexpr int verticalOffset = 2;
        constexpr int size = 16;

        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
        static const float fadeTime = store.get<ESM::GameSetting>().find("fMagicStartIconBlink")->mValue.getFloat();

        const MWWorld::Ptr player = MWMechanics::getPlayer();
        const MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);
        for (const auto& params : stats.getActiveSpells())
        {
            for (const auto& source : params.getEffects())
            {
                if (!(source.mFlags & ESM::ActiveEffect::Flag_Applied))
                    continue;
                if (source.mDuration != -1.f && source.mTimeLeft <= 0.f)
                    continue;

                const ESM::RefId effectId = source.mEffectId;
                const ESM::MagicEffect& effect = *store.get<ESM::MagicEffect>().find(effectId);
                const ESM::RefId arg = source.getSkillOrAttribute();

                if (mWidgetMap.find(effectId) == mWidgetMap.end())
                    mWidgetMap[effectId] = createIcon(*parent, effect.mName, effect.mIcon, size);

                MyGUI::ImageBox& widget = *mWidgetMap[effectId];
                if (!widget.getVisible())
                {
                    widget.setPosition(horizontalOffset, verticalOffset);
                    widget.setVisible(true);
                    if (source.mDuration >= fadeTime && fadeTime > 0.f)
                        widget.setAlpha(std::min(source.mTimeLeft / fadeTime, 1.f));
                    horizontalOffset += size;
                }

                std::string& desc = widget.getUserData<ToolTipInfo>()->text;
                if (!desc.empty())
                    desc += '\n';

                desc += params.getDisplayName();

                if (effect.mData.mFlags & ESM::MagicEffect::TargetSkill)
                {
                    const ESM::Skill& skill = *store.get<ESM::Skill>().find(arg);
                    desc += " (" + skill.mName + ')';
                }
                if (effect.mData.mFlags & ESM::MagicEffect::TargetAttribute)
                {
                    const ESM::Attribute& attribute = *store.get<ESM::Attribute>().find(arg);
                    desc += " (" + attribute.mName + ')';
                }
                desc += printEffectMagnitude(source.mMagnitude, effect.getMagnitudeDisplayType());
                if (source.mTimeLeft > -1 && Settings::game().mShowEffectDuration)
                    desc += MWGui::ToolTips::getDurationString(source.mTimeLeft, " #{sDuration}");
            }
        }

        if (adjustSize)
        {
            const int newWidth = horizontalOffset > 2 ? horizontalOffset + 2 : 0;
            const int diff = parent->getWidth() - newWidth;
            parent->setSize(newWidth, parent->getHeight());
            parent->setPosition(parent->getLeft() + diff, parent->getTop());
        }
    }
}
