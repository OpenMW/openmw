#include "spellicons.hpp"

#include <boost/lexical_cast.hpp>

#include <sstream>
#include <iomanip>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwmechanics/creaturestats.hpp"

#include "tooltips.hpp"


namespace MWGui
{

    void SpellIcons::updateWidgets(MyGUI::Widget *parent, bool adjustSize)
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        const MWMechanics::CreatureStats& stats = MWWorld::Class::get(player).getCreatureStats(player);

        std::map <int, std::vector<MagicEffectInfo> > effects;

        // add permanent item enchantments
        MWWorld::InventoryStore& store = MWWorld::Class::get(player).getInventoryStore(player);
        for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
        {
            MWWorld::ContainerStoreIterator it = store.getSlot(slot);
            if (it == store.end())
                continue;
            std::string enchantment = MWWorld::Class::get(*it).getEnchantment(*it);
            if (enchantment.empty())
                continue;
            const ESM::Enchantment* enchant = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(enchantment);
            if (enchant->mData.mType != ESM::Enchantment::ConstantEffect)
                continue;

            const ESM::EffectList& list = enchant->mEffects;
            for (std::vector<ESM::ENAMstruct>::const_iterator effectIt = list.mList.begin();
                 effectIt != list.mList.end(); ++effectIt)
            {
                const ESM::MagicEffect* magicEffect =
                    MWBase::Environment::get().getWorld ()->getStore ().get<ESM::MagicEffect>().find(effectIt->mEffectID);

                MagicEffectInfo effectInfo;
                effectInfo.mSource = MWWorld::Class::get(*it).getName(*it);
                effectInfo.mKey = MWMechanics::EffectKey (effectIt->mEffectID);
                if (magicEffect->mData.mFlags & ESM::MagicEffect::TargetSkill)
                    effectInfo.mKey.mArg = effectIt->mSkill;
                else if (magicEffect->mData.mFlags & ESM::MagicEffect::TargetAttribute)
                    effectInfo.mKey.mArg = effectIt->mAttribute;
                // just using the min magnitude here, permanent enchantments with a random magnitude just wouldn't make any sense
                effectInfo.mMagnitude = effectIt->mMagnMin;
                effectInfo.mPermanent = true;
                effects[effectIt->mEffectID].push_back (effectInfo);
            }
        }

        // add permanent spells
        const MWMechanics::Spells& spells = stats.getSpells();
        for (MWMechanics::Spells::TIterator it = spells.begin(); it != spells.end(); ++it)
        {
            const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(it->first);

            // these are the spell types that are permanently in effect
            if (!(spell->mData.mType == ESM::Spell::ST_Ability)
                    && !(spell->mData.mType == ESM::Spell::ST_Disease)
                    && !(spell->mData.mType == ESM::Spell::ST_Curse)
                    && !(spell->mData.mType == ESM::Spell::ST_Blight))
                continue;
            const ESM::EffectList& list = spell->mEffects;
            for (std::vector<ESM::ENAMstruct>::const_iterator effectIt = list.mList.begin();
                 effectIt != list.mList.end(); ++effectIt)
            {
                const ESM::MagicEffect* magicEffect =
                    MWBase::Environment::get().getWorld ()->getStore ().get<ESM::MagicEffect>().find(effectIt->mEffectID);
                MagicEffectInfo effectInfo;
                effectInfo.mSource = getSpellDisplayName (it->first);
                effectInfo.mKey = MWMechanics::EffectKey (effectIt->mEffectID);
                if (magicEffect->mData.mFlags & ESM::MagicEffect::TargetSkill)
                    effectInfo.mKey.mArg = effectIt->mSkill;
                else if (magicEffect->mData.mFlags & ESM::MagicEffect::TargetAttribute)
                    effectInfo.mKey.mArg = effectIt->mAttribute;
                // just using the min magnitude here, permanent spells with a random magnitude just wouldn't make any sense
                effectInfo.mMagnitude = effectIt->mMagnMin;
                effectInfo.mPermanent = true;

                effects[effectIt->mEffectID].push_back (effectInfo);
            }
        }

        // add lasting effect spells/potions etc
        const MWMechanics::ActiveSpells::TContainer& activeSpells = stats.getActiveSpells().getActiveSpells();
        for (MWMechanics::ActiveSpells::TContainer::const_iterator it = activeSpells.begin();
             it != activeSpells.end(); ++it)
        {
            const ESM::EffectList& list = getSpellEffectList(it->first);

            float timeScale = MWBase::Environment::get().getWorld()->getTimeScaleFactor();

            for (std::vector<ESM::ENAMstruct>::const_iterator effectIt = list.mList.begin();
                 effectIt != list.mList.end(); ++effectIt)
            {
                const ESM::MagicEffect* magicEffect =
                    MWBase::Environment::get().getWorld ()->getStore ().get<ESM::MagicEffect>().find(effectIt->mEffectID);

                MagicEffectInfo effectInfo;
                effectInfo.mSource = getSpellDisplayName (it->first);
                effectInfo.mKey = MWMechanics::EffectKey (effectIt->mEffectID);
                if (magicEffect->mData.mFlags & ESM::MagicEffect::TargetSkill)
                    effectInfo.mKey.mArg = effectIt->mSkill;
                else if (magicEffect->mData.mFlags & ESM::MagicEffect::TargetAttribute)
                    effectInfo.mKey.mArg = effectIt->mAttribute;
                effectInfo.mMagnitude = effectIt->mMagnMin + (effectIt->mMagnMax-effectIt->mMagnMin) * it->second.second;
                effectInfo.mRemainingTime = effectIt->mDuration +
                        (it->second.first - MWBase::Environment::get().getWorld()->getTimeStamp())*3600/timeScale;

                // ingredients need special casing for their magnitude / duration
                /// \todo duplicated from ActiveSpells, helper function?
                if (MWBase::Environment::get().getWorld()->getStore().get<ESM::Ingredient>().search (it->first))
                {
                    effectInfo.mRemainingTime = effectIt->mDuration * it->second.second +
                            (it->second.first - MWBase::Environment::get().getWorld()->getTimeStamp())*3600/timeScale;

                    effectInfo.mMagnitude = static_cast<int> (0.05*it->second.second / (0.1 * magicEffect->mData.mBaseCost));
                }

                effects[effectIt->mEffectID].push_back (effectInfo);
            }
        }

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


    std::string SpellIcons::getSpellDisplayName (const std::string& id)
    {
        if (const ESM::Spell *spell =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search (id))
            return spell->mName;

        if (const ESM::Potion *potion =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Potion>().search (id))
            return potion->mName;

        if (const ESM::Ingredient *ingredient =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Ingredient>().search (id))
            return ingredient->mName;

        throw std::runtime_error ("ID " + id + " has no display name");
    }

    ESM::EffectList SpellIcons::getSpellEffectList (const std::string& id)
    {
        if (const ESM::Spell *spell =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search (id))
            return spell->mEffects;

        if (const ESM::Potion *potion =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Potion>().search (id))
            return potion->mEffects;

        if (const ESM::Ingredient *ingredient =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Ingredient>().search (id))
        {
            const ESM::MagicEffect *magicEffect =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find (
                ingredient->mData.mEffectID[0]);

            ESM::ENAMstruct effect;
            effect.mEffectID = ingredient->mData.mEffectID[0];
            effect.mSkill = ingredient->mData.mSkills[0];
            effect.mAttribute = ingredient->mData.mAttributes[0];
            effect.mRange = 0;
            effect.mArea = 0;
            effect.mDuration = magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration ? 0 : 1;
            effect.mMagnMin = 1;
            effect.mMagnMax = 1;
            ESM::EffectList result;
            result.mList.push_back (effect);
            return result;
        }
        throw std::runtime_error("ID " + id + " does not have effects");
    }

}
