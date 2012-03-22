#ifndef MWGUI_STATS_WINDOW_H
#define MWGUI_STATS_WINDOW_H

#include <components/esm_store/store.hpp>

#include <sstream>
#include <set>
#include <string>
#include <utility>

#include "../mwmechanics/stat.hpp"
#include "window_base.hpp"

namespace MWGui
{
    class WindowManager;

    class StatsWindow : public WindowBase
    {
        public:
            typedef std::pair<std::string, int> Faction;
            typedef std::vector<Faction> FactionList;

            typedef std::vector<int> SkillList;

            StatsWindow(WindowManager& parWindowManager);

            void setBar(const std::string& name, const std::string& tname, int val, int max);
            void setPlayerName(const std::string& playerName);

            /// Set value for the given ID.
            void setValue (const std::string& id, const MWMechanics::Stat<int>& value);
            void setValue (const std::string& id, const MWMechanics::DynamicStat<int>& value);
            void setValue (const std::string& id, const std::string& value);
            void setValue (const std::string& id, int value);
            void setValue(const ESM::Skill::SkillEnum parSkill, const MWMechanics::Stat<float>& value);

            void configureSkills (const SkillList& major, const SkillList& minor);
            void setFactions (const std::vector<Faction>& factions);
            void setBirthSign (const std::string &signId);
            void setReputation (int reputation) { this->reputation = reputation; }
            void setBounty (int bounty) { this->bounty = bounty; }
            void updateSkillArea();

        private:
            enum ColorStyle
            {
                CS_Sub,
                CS_Normal,
                CS_Super
            };
            void setStyledText(MyGUI::TextBox* widget, ColorStyle style, const std::string &value);
            void addSkills(const SkillList &skills, const std::string &titleId, const std::string &titleDefault, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
            void addSeparator(MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
            void addGroup(const std::string &label, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
            MyGUI::TextBox* addValueItem(const std::string text, const std::string &value, ColorStyle style, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
            void addItem(const std::string text, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
            void updateScroller();

            void onScrollChangePosition(MyGUI::ScrollBar* scroller, size_t pos);
            void onWindowResize(MyGUI::Window* window);

            static const int lineHeight;

            MyGUI::WidgetPtr skillAreaWidget, skillClientWidget;
            MyGUI::ScrollBar* skillScrollerWidget;
            int lastPos, clientHeight;

            SkillList majorSkills, minorSkills, miscSkills;
            std::map<int, MWMechanics::Stat<float> > skillValues;
            std::map<int, MyGUI::TextBox*> skillWidgetMap;
            std::map<std::string, MyGUI::WidgetPtr> factionWidgetMap;
            FactionList factions; ///< Stores a list of factions and the current rank
            std::string birthSignId;
            int reputation, bounty;
            std::vector<MyGUI::WidgetPtr> skillWidgets; //< Skills and other information
    };
}
#endif

