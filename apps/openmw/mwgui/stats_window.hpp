#ifndef MWGUI_STATS_WINDOW_H
#define MWGUI_STATS_WINDOW_H

#include <components/esm_store/store.hpp>

#include <sstream>
#include <set>
#include <string>
#include <utility>

#include "../mwmechanics/stat.hpp"
#include "window_pinnable_base.hpp"

namespace MWGui
{
    class WindowManager;

    class StatsWindow : public WindowPinnableBase
    {
        public:
            typedef std::map<std::string, int> FactionList;

            typedef std::vector<int> SkillList;

            StatsWindow(WindowManager& parWindowManager);

            /// automatically updates all the data in the stats window, but only if it has changed.
            void onFrame();

            void setBar(const std::string& name, const std::string& tname, int val, int max);
            void setPlayerName(const std::string& playerName);

            /// Set value for the given ID.
            void setValue (const std::string& id, const MWMechanics::Stat<int>& value);
            void setValue (const std::string& id, const MWMechanics::DynamicStat<int>& value);
            void setValue (const std::string& id, const std::string& value);
            void setValue (const std::string& id, int value);
            void setValue(const ESM::Skill::SkillEnum parSkill, const MWMechanics::Stat<float>& value);

            void configureSkills (const SkillList& major, const SkillList& minor);
            void setReputation (int reputation) { this->mReputation = reputation; }
            void setBounty (int bounty) { this->mBounty = bounty; }
            void updateSkillArea();

        private:
            void addSkills(const SkillList &skills, const std::string &titleId, const std::string &titleDefault, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
            void addSeparator(MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
            void addGroup(const std::string &label, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
            MyGUI::TextBox* addValueItem(const std::string& text, const std::string &value, const std::string& state, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
            MyGUI::Widget* addItem(const std::string& text, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
            void updateScroller();

            void setFactions (const FactionList& factions);
            void setBirthSign (const std::string &signId);

            void onScrollChangePosition(MyGUI::ScrollBar* scroller, size_t pos);
            void onWindowResize(MyGUI::Window* window);
            void onMouseWheel(MyGUI::Widget* _sender, int _rel);

            static const int sLineHeight;

            MyGUI::Widget* mLeftPane;
            MyGUI::Widget* mRightPane;

            MyGUI::WidgetPtr mSkillAreaWidget, mSkillClientWidget;
            MyGUI::ScrollBar* mSkillScrollerWidget;
            int mLastPos, mClientHeight;

            SkillList mMajorSkills, mMinorSkills, mMiscSkills;
            std::map<int, MWMechanics::Stat<float> > mSkillValues;
            std::map<int, MyGUI::TextBox*> mSkillWidgetMap;
            std::map<std::string, MyGUI::WidgetPtr> mFactionWidgetMap;
            FactionList mFactions; ///< Stores a list of factions and the current rank
            std::string mBirthSignId;
            int mReputation, mBounty;
            std::vector<MyGUI::WidgetPtr> mSkillWidgets; //< Skills and other information

            bool mChanged;

        protected:
            virtual void onPinToggled();
    };
}
#endif

