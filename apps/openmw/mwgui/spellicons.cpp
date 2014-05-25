#include "spellicons.hpp"

#include <boost/lexical_cast.hpp>

#include <sstream>
#include <iomanip>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwmechanics/creaturestats.hpp"

#include "tooltips.hpp"


namespace MWGui
{

    void EffectSourceVisitor::visit (MWMechanics::EffectKey key,
                                           const std::string& sourceName, int casterActorId,
                                     float magnitude, float remainingTime)
    {
        MagicEffectInfo newEffectSource;
        newEffectSource.mKey = key;
        newEffectSource.mMagnitude = magnitude;
        newEffectSource.mPermanent = mIsPermanent;
        newEffectSource.mRemainingTime = remainingTime;
        newEffectSource.mSource = sourceName;

        mEffectSources[key.mId].push_back(newEffectSource);
    }


    void SpellIcons::updateWidgets(MyGUI::Widget *parent, bool adjustSize)
    {
        // TODO: Tracking add/remove/expire would be better than force updating every frame

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        const MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);


        EffectSourceVisitor visitor;

        // permanent item enchantments & permanent spells
        visitor.mIsPermanent = true;
        MWWorld::InventoryStore& store = player.getClass().getInventoryStore(player);
        store.visitEffectSources(visitor);
        stats.getSpells().visitEffectSources(visitor);

        // now add lasting effects
        visitor.mIsPermanent = false;
        stats.getActiveSpells().visitEffectSources(visitor);

        std::map <int, std::vector<MagicEffectInfo> >& effects = visitor.mEffectSources;

        int w=2;

        for (std::map <int, std::vector<MagicEffectInfo> >::const_iterator it = effects.begin(); it != effects.end(); ++it)
        {
            const ESM::MagicEffect* effect =
                MWBase::Environment::get().getWorld ()->getStore ().get<ESM::MagicEffect>().find(it->first);

            float remainingDuration = 0;

            std::string sourcesDescription;

            const float fadeTime = 5.f;

            for (std::vector<MagicEffectInfo>::const_iterator effectIt = it->second.begin();
                 effectIt != it->second.end(); ++effectIt)
            {
                if (effectIt != it->second.begin())
                    sourcesDescription += "\n";

                // if at least one of the effect sources is permanent, the effect will never wear off
                if (effectIt->mPermanent)
                    remainingDuration = fadeTime;
                else
                    remainingDuration = std::max(remainingDuration, effectIt->mRemainingTime);

                sourcesDescription +=  effectIt->mSource;

                if (effect->mData.mFlags & ESM::MagicEffect::TargetSkill)
                    sourcesDescription += " (" +
                            MWBase::Environment::get().getWindowManager()->getGameSettingString(
                                ESM::Skill::sSkillNameIds[effectIt->mKey.mArg], "") + ")";
                if (effect->mData.mFlags & ESM::MagicEffect::TargetAttribute)
                    sourcesDescription += " (" +
                            MWBase::Environment::get().getWindowManager()->getGameSettingString(
                                ESM::Attribute::sGmstAttributeIds[effectIt->mKey.mArg], "") + ")";

                ESM::MagicEffect::MagnitudeDisplayType displayType = effect->getMagnitudeDisplayType();
                if (displayType == ESM::MagicEffect::MDT_TimesInt)
                {
                    std::string timesInt =  MWBase::Environment::get().getWindowManager()->getGameSettingString("sXTimesINT", "");
                    std::stringstream formatter;
                    formatter << std::fixed << std::setprecision(1) << " " << (effectIt->mMagnitude / 10.0f) << timesInt;
                    sourcesDescription += formatter.str();
                }
                else if ( displayType != ESM::MagicEffect::MDT_None )
                {
                    sourcesDescription += ": " + boost::lexical_cast<std::string>(effectIt->mMagnitude);

                    if ( displayType == ESM::MagicEffect::MDT_Percentage )
                        sourcesDescription += MWBase::Environment::get().getWindowManager()->getGameSettingString("spercent", "");
                    else if ( displayType == ESM::MagicEffect::MDT_Feet )
                        sourcesDescription += " " + MWBase::Environment::get().getWindowManager()->getGameSettingString("sfeet", "");
                    else if ( displayType == ESM::MagicEffect::MDT_Level )
                    {
                        sourcesDescription += " " + ((effectIt->mMagnitude > 1) ?
                            MWBase::Environment::get().getWindowManager()->getGameSettingString("sLevels", "") :
                            MWBase::Environment::get().getWindowManager()->getGameSettingString("sLevel", "") );
                    }
                    else // ESM::MagicEffect::MDT_Points
                    {
                        sourcesDescription += " " + ((effectIt->mMagnitude > 1) ?
                            MWBase::Environment::get().getWindowManager()->getGameSettingString("spoints", "") :
                            MWBase::Environment::get().getWindowManager()->getGameSettingString("spoint", "") );
                    }
                }
            }

            if (remainingDuration > 0.f)
            {
                MyGUI::ImageBox* image;
                if (mWidgetMap.find(it->first) == mWidgetMap.end())
                {
                    image = parent->createWidget<MyGUI::ImageBox>
                        ("ImageBox", MyGUI::IntCoord(w,2,16,16), MyGUI::Align::Default);
                    mWidgetMap[it->first] = image;

                    std::string icon = effect->mIcon;
                    icon[icon.size()-3] = 'd';
                    icon[icon.size()-2] = 'd';
                    icon[icon.size()-1] = 's';
                    icon = "icons\\" + icon;

                    image->setImageTexture(icon);

                    std::string name = ESM::MagicEffect::effectIdToString (it->first);

                    ToolTipInfo tooltipInfo;
                    tooltipInfo.caption = "#{" + name + "}";
                    tooltipInfo.icon = effect->mIcon;
                    tooltipInfo.imageSize = 16;
                    tooltipInfo.wordWrap = false;

                    image->setUserData(tooltipInfo);
                    image->setUserString("ToolTipType", "ToolTipInfo");
                }
                else
                    image = mWidgetMap[it->first];

                image->setPosition(w,2);
                image->setVisible(true);
                w += 16;

                ToolTipInfo* tooltipInfo = image->getUserData<ToolTipInfo>();
                tooltipInfo->text = sourcesDescription;

                // Fade out during the last 5 seconds
                image->setAlpha(std::min(remainingDuration/fadeTime, 1.f));
            }
            else if (mWidgetMap.find(it->first) != mWidgetMap.end())
            {
                mWidgetMap[it->first]->setVisible(false);
            }
        }

        if (adjustSize)
        {
            int s = w + 2;
            if (effects.empty())
                s = 0;
            int diff = parent->getWidth() - s;
            parent->setSize(s, parent->getHeight());
            parent->setPosition(parent->getLeft()+diff, parent->getTop());
        }

        // hide inactive effects
        for (std::map<int, MyGUI::ImageBox*>::iterator it = mWidgetMap.begin(); it != mWidgetMap.end(); ++it)
        {
            if (effects.find(it->first) == effects.end())
                it->second->setVisible(false);
        }
    }

}
