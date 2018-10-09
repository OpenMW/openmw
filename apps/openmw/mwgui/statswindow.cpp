#include "statswindow.hpp"

#include <MyGUI_Window.h>
#include <MyGUI_ScrollView.h>
#include <MyGUI_ProgressBar.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_Gui.h>

#include <components/settings/settings.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "tooltips.hpp"

namespace MWGui
{
    StatsWindow::StatsWindow (DragAndDrop* drag)
      : WindowPinnableBase("openmw_stats_window.layout")
      , NoDrop(drag, mMainWidget)
      , mSkillView(nullptr)
      , mMajorSkills()
      , mMinorSkills()
      , mMiscSkills()
      , mSkillValues()
      , mSkillWidgetMap()
      , mFactionWidgetMap()
      , mFactions()
      , mBirthSignId()
      , mReputation(0)
      , mBounty(0)
      , mSkillWidgets()
      , mChanged(true)
    {
        setCoord(0,0,498, 342);

        const char *names[][2] =
        {
            { "Attrib1", "sAttributeStrength" },
            { "Attrib2", "sAttributeIntelligence" },
            { "Attrib3", "sAttributeWillpower" },
            { "Attrib4", "sAttributeAgility" },
            { "Attrib5", "sAttributeSpeed" },
            { "Attrib6", "sAttributeEndurance" },
            { "Attrib7", "sAttributePersonality" },
            { "Attrib8", "sAttributeLuck" },
            { 0, 0 }
        };

        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        for (int i=0; names[i][0]; ++i)
        {
            setText (names[i][0], store.get<ESM::GameSetting>().find (names[i][1])->mValue.getString());
        }

        getWidget(mSkillView, "SkillView");
        getWidget(mLeftPane, "LeftPane");
        getWidget(mRightPane, "RightPane");

        for (int i = 0; i < ESM::Skill::Length; ++i)
        {
            mSkillValues.insert(std::make_pair(i, MWMechanics::SkillValue()));
            mSkillWidgetMap.insert(std::make_pair(i, std::make_pair((MyGUI::TextBox*)nullptr, (MyGUI::TextBox*)nullptr)));
        }

        MyGUI::Window* t = mMainWidget->castType<MyGUI::Window>();
        t->eventWindowChangeCoord += MyGUI::newDelegate(this, &StatsWindow::onWindowResize);

        onWindowResize(t);
    }

    void StatsWindow::onMouseWheel(MyGUI::Widget* _sender, int _rel)
    {
        if (mSkillView->getViewOffset().top + _rel*0.3 > 0)
            mSkillView->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mSkillView->setViewOffset(MyGUI::IntPoint(0, static_cast<int>(mSkillView->getViewOffset().top + _rel*0.3)));
    }

    void StatsWindow::onWindowResize(MyGUI::Window* window)
    {
        mLeftPane->setCoord( MyGUI::IntCoord(0, 0, static_cast<int>(0.44*window->getSize().width), window->getSize().height) );
        mRightPane->setCoord( MyGUI::IntCoord(static_cast<int>(0.44*window->getSize().width), 0, static_cast<int>(0.56*window->getSize().width), window->getSize().height) );
        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the scrollbar is hidden
        mSkillView->setVisibleVScroll(false);
        mSkillView->setCanvasSize (mSkillView->getWidth(), mSkillView->getCanvasSize().height);
        mSkillView->setVisibleVScroll(true);
    }

    void StatsWindow::setBar(const std::string& name, const std::string& tname, int val, int max)
    {
        MyGUI::ProgressBar* pt;
        getWidget(pt, name);

        std::stringstream out;
        out << val << "/" << max;
        setText(tname, out.str());

        pt->setProgressRange(std::max(0, max));
        pt->setProgressPosition(std::max(0, val));
    }

    void StatsWindow::setPlayerName(const std::string& playerName)
    {
        mMainWidget->castType<MyGUI::Window>()->setCaption(playerName);
    }

    void StatsWindow::setValue (const std::string& id, const MWMechanics::AttributeValue& value)
    {
        static const char *ids[] =
        {
            "AttribVal1", "AttribVal2", "AttribVal3", "AttribVal4", "AttribVal5",
            "AttribVal6", "AttribVal7", "AttribVal8",
            0
        };

        for (int i=0; ids[i]; ++i)
            if (ids[i]==id)
            {
                std::ostringstream valueString;
                valueString << value.getModified();
                setText (id, valueString.str());

                MyGUI::TextBox* box;
                getWidget(box, id);

                if (value.getModified()>value.getBase())
                    box->_setWidgetState("increased");
                else if (value.getModified()<value.getBase())
                    box->_setWidgetState("decreased");
                else
                    box->_setWidgetState("normal");

                break;
            }
    }

    void StatsWindow::setValue (const std::string& id, const MWMechanics::DynamicStat<float>& value)
    {
        int current = static_cast<int>(value.getCurrent());
        int modified = static_cast<int>(value.getModified());

        // Fatigue can be negative
        if (id != "FBar")
            current = std::max(0, current);

        setBar (id, id + "T", current, modified);

        // health, magicka, fatigue tooltip
        MyGUI::Widget* w;
        std::string valStr =  MyGUI::utility::toString(current) + " / " + MyGUI::utility::toString(modified);
        if (id == "HBar")
        {
            getWidget(w, "Health");
            w->setUserString("Caption_HealthDescription", "#{sHealthDesc}\n" + valStr);
        }
        else if (id == "MBar")
        {
            getWidget(w, "Magicka");
            w->setUserString("Caption_HealthDescription", "#{sMagDesc}\n" + valStr);
        }
        else if (id == "FBar")
        {
            getWidget(w, "Fatigue");
            w->setUserString("Caption_HealthDescription", "#{sFatDesc}\n" + valStr);
        }
    }

    void StatsWindow::setValue (const std::string& id, const std::string& value)
    {
        if (id=="name")
            setPlayerName (value);
        else if (id=="race")
            setText ("RaceText", value);
        else if (id=="class")
            setText ("ClassText", value);
    }

    void StatsWindow::setValue (const std::string& id, int value)
    {
        if (id=="level")
        {
            std::ostringstream text;
            text << value;
            setText("LevelText", text.str());
        }
    }

    void setSkillProgress(MyGUI::Widget* w, float progress, int skillId)
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        const MWWorld::ESMStore &esmStore =
            MWBase::Environment::get().getWorld()->getStore();

        float progressRequirement = player.getClass().getNpcStats(player).getSkillProgressRequirement(skillId,
            *esmStore.get<ESM::Class>().find(player.get<ESM::NPC>()->mBase->mClass));

        // This is how vanilla MW displays the progress bar (I think). Note it's slightly inaccurate,
        // due to the int casting in the skill levelup logic. Also the progress label could in rare cases
        // reach 100% without the skill levelling up.
        // Leaving the original display logic for now, for consistency with ess-imported savegames.
        int progressPercent = int(float(progress) / float(progressRequirement) * 100.f + 0.5f);

        w->setUserString("Caption_SkillProgressText", MyGUI::utility::toString(progressPercent)+"/100");
        w->setUserString("RangePosition_SkillProgress", MyGUI::utility::toString(progressPercent));
    }

    void StatsWindow::setValue(const ESM::Skill::SkillEnum parSkill, const MWMechanics::SkillValue& value)
    {
        mSkillValues[parSkill] = value;
        std::pair<MyGUI::TextBox*, MyGUI::TextBox*> widgets = mSkillWidgetMap[(int)parSkill];
        MyGUI::TextBox* valueWidget = widgets.second;
        MyGUI::TextBox* nameWidget = widgets.first;
        if (valueWidget && nameWidget)
        {
            int modified = value.getModified(), base = value.getBase();
            std::string text = MyGUI::utility::toString(modified);
            std::string state = "normal";
            if (modified > base)
                state = "increased";
            else if (modified < base)
                state = "decreased";

            int widthBefore = valueWidget->getTextSize().width;

            valueWidget->setCaption(text);
            valueWidget->_setWidgetState(state);

            int widthAfter = valueWidget->getTextSize().width;
            if (widthBefore != widthAfter)
            {
                valueWidget->setCoord(valueWidget->getLeft() - (widthAfter-widthBefore), valueWidget->getTop(), valueWidget->getWidth() + (widthAfter-widthBefore), valueWidget->getHeight());
                nameWidget->setSize(nameWidget->getWidth() - (widthAfter-widthBefore), nameWidget->getHeight());
            }

            if (value.getBase() < 100)
            {
                nameWidget->setUserString("Visible_SkillMaxed", "false");
                nameWidget->setUserString("UserData^Hidden_SkillMaxed", "true");
                nameWidget->setUserString("Visible_SkillProgressVBox", "true");
                nameWidget->setUserString("UserData^Hidden_SkillProgressVBox", "false");

                valueWidget->setUserString("Visible_SkillMaxed", "false");
                valueWidget->setUserString("UserData^Hidden_SkillMaxed", "true");
                valueWidget->setUserString("Visible_SkillProgressVBox", "true");
                valueWidget->setUserString("UserData^Hidden_SkillProgressVBox", "false");

                setSkillProgress(nameWidget, value.getProgress(), parSkill);
                setSkillProgress(valueWidget, value.getProgress(), parSkill);
            }
            else
            {
                nameWidget->setUserString("Visible_SkillMaxed", "true");
                nameWidget->setUserString("UserData^Hidden_SkillMaxed", "false");
                nameWidget->setUserString("Visible_SkillProgressVBox", "false");
                nameWidget->setUserString("UserData^Hidden_SkillProgressVBox", "true");

                valueWidget->setUserString("Visible_SkillMaxed", "true");
                valueWidget->setUserString("UserData^Hidden_SkillMaxed", "false");
                valueWidget->setUserString("Visible_SkillProgressVBox", "false");
                valueWidget->setUserString("UserData^Hidden_SkillProgressVBox", "true");
            }
        }
    }

    void StatsWindow::configureSkills (const std::vector<int>& major, const std::vector<int>& minor)
    {
        mMajorSkills = major;
        mMinorSkills = minor;

        // Update misc skills with the remaining skills not in major or minor
        std::set<int> skillSet;
        std::copy(major.begin(), major.end(), std::inserter(skillSet, skillSet.begin()));
        std::copy(minor.begin(), minor.end(), std::inserter(skillSet, skillSet.begin()));
        boost::array<ESM::Skill::SkillEnum, ESM::Skill::Length>::const_iterator end = ESM::Skill::sSkillIds.end();
        mMiscSkills.clear();
        for (boost::array<ESM::Skill::SkillEnum, ESM::Skill::Length>::const_iterator it = ESM::Skill::sSkillIds.begin(); it != end; ++it)
        {
            int skill = *it;
            if (skillSet.find(skill) == skillSet.end())
                mMiscSkills.push_back(skill);
        }

        updateSkillArea();
    }

    void StatsWindow::onFrame (float dt)
    {
        NoDrop::onFrame(dt);

        MWWorld::Ptr player = MWMechanics::getPlayer();
        const MWMechanics::NpcStats &PCstats = player.getClass().getNpcStats(player);

        // level progress
        MyGUI::Widget* levelWidget;
        for (int i=0; i<2; ++i)
        {
            int max = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("iLevelUpTotal")->mValue.getInteger();
            getWidget(levelWidget, i==0 ? "Level_str" : "LevelText");
            levelWidget->setUserString("RangePosition_LevelProgress", MyGUI::utility::toString(PCstats.getLevelProgress()));
            levelWidget->setUserString("Range_LevelProgress", MyGUI::utility::toString(max));
            levelWidget->setUserString("Caption_LevelProgressText", MyGUI::utility::toString(PCstats.getLevelProgress()) + "/"
                                       + MyGUI::utility::toString(max));
        }

        setFactions(PCstats.getFactionRanks());
        setExpelled(PCstats.getExpelled ());

        const std::string &signId =
            MWBase::Environment::get().getWorld()->getPlayer().getBirthSign();

        setBirthSign(signId);
        setReputation (PCstats.getReputation ());
        setBounty (PCstats.getBounty ());

        if (mChanged)
            updateSkillArea();
    }

    void StatsWindow::setFactions (const FactionList& factions)
    {
        if (mFactions != factions)
        {
            mFactions = factions;
            mChanged = true;
        }
    }

    void StatsWindow::setExpelled (const std::set<std::string>& expelled)
    {
        if (mExpelled != expelled)
        {
            mExpelled = expelled;
            mChanged = true;
        }
    }

    void StatsWindow::setBirthSign (const std::string& signId)
    {
        if (signId != mBirthSignId)
        {
            mBirthSignId = signId;
            mChanged = true;
        }
    }

    void StatsWindow::addSeparator(MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
    {
        MyGUI::ImageBox* separator = mSkillView->createWidget<MyGUI::ImageBox>("MW_HLine",
            MyGUI::IntCoord(10, coord1.top, coord1.width + coord2.width - 4, 18),
            MyGUI::Align::Left | MyGUI::Align::Top | MyGUI::Align::HStretch);
        separator->eventMouseWheel += MyGUI::newDelegate(this, &StatsWindow::onMouseWheel);
        mSkillWidgets.push_back(separator);

        coord1.top += separator->getHeight();
        coord2.top += separator->getHeight();
    }

    void StatsWindow::addGroup(const std::string &label, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
    {
        MyGUI::TextBox* groupWidget = mSkillView->createWidget<MyGUI::TextBox>("SandBrightText",
            MyGUI::IntCoord(0, coord1.top, coord1.width + coord2.width, coord1.height),
            MyGUI::Align::Left | MyGUI::Align::Top | MyGUI::Align::HStretch);
        groupWidget->setCaption(label);
        groupWidget->eventMouseWheel += MyGUI::newDelegate(this, &StatsWindow::onMouseWheel);
        mSkillWidgets.push_back(groupWidget);

        int lineHeight = MWBase::Environment::get().getWindowManager()->getFontHeight() + 2;
        coord1.top += lineHeight;
        coord2.top += lineHeight;
    }

    std::pair<MyGUI::TextBox*, MyGUI::TextBox*> StatsWindow::addValueItem(const std::string& text, const std::string &value, const std::string& state, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
    {
        MyGUI::TextBox *skillNameWidget, *skillValueWidget;

        skillNameWidget = mSkillView->createWidget<MyGUI::TextBox>("SandText", coord1, MyGUI::Align::Left | MyGUI::Align::Top | MyGUI::Align::HStretch);
        skillNameWidget->setCaption(text);
        skillNameWidget->eventMouseWheel += MyGUI::newDelegate(this, &StatsWindow::onMouseWheel);

        skillValueWidget = mSkillView->createWidget<MyGUI::TextBox>("SandTextRight", coord2, MyGUI::Align::Right | MyGUI::Align::Top);
        skillValueWidget->setCaption(value);
        skillValueWidget->_setWidgetState(state);
        skillValueWidget->eventMouseWheel += MyGUI::newDelegate(this, &StatsWindow::onMouseWheel);

        // resize dynamically according to text size
        int textWidthPlusMargin = skillValueWidget->getTextSize().width + 12;
        skillValueWidget->setCoord(coord2.left + coord2.width - textWidthPlusMargin, coord2.top, textWidthPlusMargin, coord2.height);
        skillNameWidget->setSize(skillNameWidget->getSize() + MyGUI::IntSize(coord2.width - textWidthPlusMargin, 0));

        mSkillWidgets.push_back(skillNameWidget);
        mSkillWidgets.push_back(skillValueWidget);

        int lineHeight = MWBase::Environment::get().getWindowManager()->getFontHeight() + 2;
        coord1.top += lineHeight;
        coord2.top += lineHeight;

        return std::make_pair(skillNameWidget, skillValueWidget);
    }

    MyGUI::Widget* StatsWindow::addItem(const std::string& text, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
    {
        MyGUI::TextBox* skillNameWidget;

        skillNameWidget = mSkillView->createWidget<MyGUI::TextBox>("SandText", coord1, MyGUI::Align::Default);

        skillNameWidget->setCaption(text);
        skillNameWidget->eventMouseWheel += MyGUI::newDelegate(this, &StatsWindow::onMouseWheel);

        int textWidth = skillNameWidget->getTextSize().width;
        skillNameWidget->setSize(textWidth, skillNameWidget->getHeight());

        mSkillWidgets.push_back(skillNameWidget);

        int lineHeight = MWBase::Environment::get().getWindowManager()->getFontHeight() + 2;
        coord1.top += lineHeight;
        coord2.top += lineHeight;

        return skillNameWidget;
    }

    void StatsWindow::addSkills(const SkillList &skills, const std::string &titleId, const std::string &titleDefault, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2)
    {
        // Add a line separator if there are items above
        if (!mSkillWidgets.empty())
        {
            addSeparator(coord1, coord2);
        }

        addGroup(MWBase::Environment::get().getWindowManager()->getGameSettingString(titleId, titleDefault), coord1, coord2);

        SkillList::const_iterator end = skills.end();
        for (SkillList::const_iterator it = skills.begin(); it != end; ++it)
        {
            int skillId = *it;
            if (skillId < 0 || skillId >= ESM::Skill::Length) // Skip unknown skill indexes
                continue;
            const std::string &skillNameId = ESM::Skill::sSkillNameIds[skillId];
            const MWMechanics::SkillValue &stat = mSkillValues.find(skillId)->second;
            int base = stat.getBase();
            int modified = stat.getModified();

            const MWWorld::ESMStore &esmStore =
                MWBase::Environment::get().getWorld()->getStore();

            const ESM::Skill* skill = esmStore.get<ESM::Skill>().find(skillId);

            std::string icon = "icons\\k\\" + ESM::Skill::sIconNames[skillId];

            const ESM::Attribute* attr =
                esmStore.get<ESM::Attribute>().find(skill->mData.mAttribute);

            std::string state = "normal";
            if (modified > base)
                state = "increased";
            else if (modified < base)
                state = "decreased";
            std::pair<MyGUI::TextBox*, MyGUI::TextBox*> widgets = addValueItem(MWBase::Environment::get().getWindowManager()->getGameSettingString(skillNameId, skillNameId),
                MyGUI::utility::toString(static_cast<int>(modified)), state, coord1, coord2);

            for (int i=0; i<2; ++i)
            {
                mSkillWidgets[mSkillWidgets.size()-1-i]->setUserString("ToolTipType", "Layout");
                mSkillWidgets[mSkillWidgets.size()-1-i]->setUserString("ToolTipLayout", "SkillToolTip");
                mSkillWidgets[mSkillWidgets.size()-1-i]->setUserString("Caption_SkillName", "#{"+skillNameId+"}");
                mSkillWidgets[mSkillWidgets.size()-1-i]->setUserString("Caption_SkillDescription", skill->mDescription);
                mSkillWidgets[mSkillWidgets.size()-1-i]->setUserString("Caption_SkillAttribute", "#{sGoverningAttribute}: #{" + attr->mName + "}");
                mSkillWidgets[mSkillWidgets.size()-1-i]->setUserString("ImageTexture_SkillImage", icon);
                mSkillWidgets[mSkillWidgets.size()-1-i]->setUserString("Range_SkillProgress", "100");
                if (base < 100)
                {
                    mSkillWidgets[mSkillWidgets.size()-1-i]->setUserString("Visible_SkillMaxed", "false");
                    mSkillWidgets[mSkillWidgets.size()-1-i]->setUserString("UserData^Hidden_SkillMaxed", "true");

                    mSkillWidgets[mSkillWidgets.size()-1-i]->setUserString("Visible_SkillProgressVBox", "true");
                    mSkillWidgets[mSkillWidgets.size()-1-i]->setUserString("UserData^Hidden_SkillProgressVBox", "false");

                    setSkillProgress(mSkillWidgets[mSkillWidgets.size()-1-i], stat.getProgress(), skillId);
                }
                else
                {
                    mSkillWidgets[mSkillWidgets.size()-1-i]->setUserString("Visible_SkillMaxed", "true");
                    mSkillWidgets[mSkillWidgets.size()-1-i]->setUserString("UserData^Hidden_SkillMaxed", "false");

                    mSkillWidgets[mSkillWidgets.size()-1-i]->setUserString("Visible_SkillProgressVBox", "false");
                    mSkillWidgets[mSkillWidgets.size()-1-i]->setUserString("UserData^Hidden_SkillProgressVBox", "true");
                }
            }

            mSkillWidgetMap[skillId] = widgets;
        }
    }

    void StatsWindow::updateSkillArea()
    {
        mChanged = false;

        for (std::vector<MyGUI::Widget*>::iterator it = mSkillWidgets.begin(); it != mSkillWidgets.end(); ++it)
        {
            MyGUI::Gui::getInstance().destroyWidget(*it);
        }
        mSkillWidgets.clear();

        const int valueSize = 40;
        MyGUI::IntCoord coord1(10, 0, mSkillView->getWidth() - (10 + valueSize) - 24, 18);
        MyGUI::IntCoord coord2(coord1.left + coord1.width, coord1.top, valueSize, coord1.height);

        if (!mMajorSkills.empty())
            addSkills(mMajorSkills, "sSkillClassMajor", "Major Skills", coord1, coord2);

        if (!mMinorSkills.empty())
            addSkills(mMinorSkills, "sSkillClassMinor", "Minor Skills", coord1, coord2);

        if (!mMiscSkills.empty())
            addSkills(mMiscSkills, "sSkillClassMisc", "Misc Skills", coord1, coord2);

        MWBase::World *world = MWBase::Environment::get().getWorld();
        const MWWorld::ESMStore &store = world->getStore();
        const ESM::NPC *player =
            world->getPlayerPtr().get<ESM::NPC>()->mBase;

        // race tooltip
        const ESM::Race* playerRace = store.get<ESM::Race>().find(player->mRace);

        MyGUI::Widget* raceWidget;
        getWidget(raceWidget, "RaceText");
        ToolTips::createRaceToolTip(raceWidget, playerRace);
        getWidget(raceWidget, "Race_str");
        ToolTips::createRaceToolTip(raceWidget, playerRace);

        // class tooltip
        MyGUI::Widget* classWidget;

        const ESM::Class *playerClass =
            store.get<ESM::Class>().find(player->mClass);

        getWidget(classWidget, "ClassText");
        ToolTips::createClassToolTip(classWidget, *playerClass);
        getWidget(classWidget, "Class_str");
        ToolTips::createClassToolTip(classWidget, *playerClass);

        if (!mFactions.empty())
        {
            MWWorld::Ptr playerPtr = MWMechanics::getPlayer();
            const MWMechanics::NpcStats &PCstats = playerPtr.getClass().getNpcStats(playerPtr);
            const std::set<std::string> &expelled = PCstats.getExpelled();

            bool firstFaction=true;
            FactionList::const_iterator end = mFactions.end();
            for (FactionList::const_iterator it = mFactions.begin(); it != end; ++it)
            {
                const ESM::Faction *faction =
                    store.get<ESM::Faction>().find(it->first);
                if (faction->mData.mIsHidden == 1)
                    continue;

                if (firstFaction)
                {
                    // Add a line separator if there are items above
                    if (!mSkillWidgets.empty())
                        addSeparator(coord1, coord2);

                    addGroup(MWBase::Environment::get().getWindowManager()->getGameSettingString("sFaction", "Faction"), coord1, coord2);

                    firstFaction = false;
                }

                MyGUI::Widget* w = addItem(faction->mName, coord1, coord2);

                std::string text;

                text += std::string("#{fontcolourhtml=header}") + faction->mName;

                if (expelled.find(it->first) != expelled.end())
                    text += "\n#{fontcolourhtml=normal}#{sExpelled}";
                else
                {
                    int rank = it->second;
                    rank = std::max(0, std::min(9, rank));
                    text += std::string("\n#{fontcolourhtml=normal}") + faction->mRanks[rank];

                    if (rank < 9)
                    {
                        // player doesn't have max rank yet
                        text += std::string("\n\n#{fontcolourhtml=header}#{sNextRank} ") + faction->mRanks[rank+1];

                        ESM::RankData rankData = faction->mData.mRankData[rank+1];
                        const ESM::Attribute* attr1 = store.get<ESM::Attribute>().find(faction->mData.mAttribute[0]);
                        const ESM::Attribute* attr2 = store.get<ESM::Attribute>().find(faction->mData.mAttribute[1]);

                        text += "\n#{fontcolourhtml=normal}#{" + attr1->mName + "}: " + MyGUI::utility::toString(rankData.mAttribute1)
                                + ", #{" + attr2->mName + "}: " + MyGUI::utility::toString(rankData.mAttribute2);

                        text += "\n\n#{fontcolourhtml=header}#{sFavoriteSkills}";
                        text += "\n#{fontcolourhtml=normal}";
                        bool firstSkill = true;
                        for (int i=0; i<7; ++i)
                        {
                            if (faction->mData.mSkills[i] != -1)
                            {
                                if (!firstSkill)
                                    text += ", ";

                                firstSkill = false;

                                text += "#{"+ESM::Skill::sSkillNameIds[faction->mData.mSkills[i]]+"}";
                            }
                        }

                        text += "\n";

                        if (rankData.mSkill1 > 0)
                            text += "\n#{sNeedOneSkill} " + MyGUI::utility::toString(rankData.mSkill1);
                        if (rankData.mSkill2 > 0)
                            text += " #{sand} #{sNeedTwoSkills} " + MyGUI::utility::toString(rankData.mSkill2);
                    }
                }

                w->setUserString("ToolTipType", "Layout");
                w->setUserString("ToolTipLayout", "FactionToolTip");
                w->setUserString("Caption_FactionText", text);
            }
        }

        if (!mBirthSignId.empty())
        {
            // Add a line separator if there are items above
            if (!mSkillWidgets.empty())
                addSeparator(coord1, coord2);

            addGroup(MWBase::Environment::get().getWindowManager()->getGameSettingString("sBirthSign", "Sign"), coord1, coord2);
            const ESM::BirthSign *sign =
                store.get<ESM::BirthSign>().find(mBirthSignId);
            MyGUI::Widget* w = addItem(sign->mName, coord1, coord2);

            ToolTips::createBirthsignToolTip(w, mBirthSignId);
        }

        // Add a line separator if there are items above
        if (!mSkillWidgets.empty())
            addSeparator(coord1, coord2);

        addValueItem(MWBase::Environment::get().getWindowManager()->getGameSettingString("sReputation", "Reputation"),
                    MyGUI::utility::toString(static_cast<int>(mReputation)), "normal", coord1, coord2);

        for (int i=0; i<2; ++i)
        {
            mSkillWidgets[mSkillWidgets.size()-1-i]->setUserString("ToolTipType", "Layout");
            mSkillWidgets[mSkillWidgets.size()-1-i]->setUserString("ToolTipLayout", "TextToolTip");
            mSkillWidgets[mSkillWidgets.size()-1-i]->setUserString("Caption_Text", "#{sSkillsMenuReputationHelp}");
        }

        addValueItem(MWBase::Environment::get().getWindowManager()->getGameSettingString("sBounty", "Bounty"),
                    MyGUI::utility::toString(static_cast<int>(mBounty)), "normal", coord1, coord2);

        for (int i=0; i<2; ++i)
        {
            mSkillWidgets[mSkillWidgets.size()-1-i]->setUserString("ToolTipType", "Layout");
            mSkillWidgets[mSkillWidgets.size()-1-i]->setUserString("ToolTipLayout", "TextToolTip");
            mSkillWidgets[mSkillWidgets.size()-1-i]->setUserString("Caption_Text", "#{sCrimeHelp}");
        }

        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the scrollbar is hidden
        mSkillView->setVisibleVScroll(false);
        mSkillView->setCanvasSize (mSkillView->getWidth(), std::max(mSkillView->getHeight(), coord1.top));
        mSkillView->setVisibleVScroll(true);
    }

    void StatsWindow::onPinToggled()
    {
        Settings::Manager::setBool("stats pin", "Windows", mPinned);

        MWBase::Environment::get().getWindowManager()->setHMSVisibility(!mPinned);
    }

    void StatsWindow::onTitleDoubleClicked()
    {
        if (!mPinned)
            MWBase::Environment::get().getWindowManager()->toggleVisible(GW_Stats);
    }
}
