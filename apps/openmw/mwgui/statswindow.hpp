#ifndef MWGUI_STATS_WINDOW_H
#define MWGUI_STATS_WINDOW_H

#include "../mwmechanics/stat.hpp"
#include "windowpinnablebase.hpp"

#include <components/esm/loadskil.hpp>

namespace MWGui
{
    class WindowManager;

    class StatsWindow : public WindowPinnableBase, public NoDrop
    {
        public:
            typedef std::map<std::string, int> FactionList;

            typedef std::vector<int> SkillList;

            StatsWindow(DragAndDrop* drag);

            /// automatically updates all the data in the stats window, but only if it has changed.
            void onFrame(float dt);

            void setBar(const std::string& name, const std::string& tname, int val, int max);
            void setPlayerName(const std::string& playerName);

            /// Set value for the given ID.
            void setValue (const std::string& id, const MWMechanics::AttributeValue& value);
            void setValue (const std::string& id, const MWMechanics::DynamicStat<float>& value);
            void setValue (const std::string& id, const std::string& value);
            void setValue (const std::string& id, int value);
            void setValue(const ESM::Skill::SkillEnum parSkill, const MWMechanics::SkillValue& value);

            void configureSkills (const SkillList& major, const SkillList& minor);
            void setReputation (int reputation) { if (reputation != mReputation) mChanged = true; this->mReputation = reputation; }
            void setBounty (int bounty) { if (bounty != mBounty) mChanged = true; this->mBounty = bounty; }
            void updateSkillArea();

            virtual void onOpen() { onWindowResize(mMainWidget->castType<MyGUI::Window>()); }

        private:
            void addSkills(const SkillList &skills, const std::string &titleId, const std::string &titleDefault, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
            void addSeparator(MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
            void addGroup(const std::string &label, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
            std::pair<MyGUI::TextBox*, MyGUI::TextBox*> addValueItem(const std::string& text, const std::string &value, const std::string& state, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
            MyGUI::Widget* addItem(const std::string& text, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);

            void setFactions (const FactionList& factions);
            void setExpelled (const std::set<std::string>& expelled);
            void setBirthSign (const std::string &signId);

            void onWindowResize(MyGUI::Window* window);
            void onMouseWheel(MyGUI::Widget* _sender, int _rel);

            MyGUI::Widget* mLeftPane;
            MyGUI::Widget* mRightPane;

            MyGUI::ScrollView* mSkillView;

            SkillList mMajorSkills, mMinorSkills, mMiscSkills;
            std::map<int, MWMechanics::SkillValue > mSkillValues;
            std::map<int, std::pair<MyGUI::TextBox*, MyGUI::TextBox*> > mSkillWidgetMap;
            std::map<std::string, MyGUI::Widget*> mFactionWidgetMap;
            FactionList mFactions; ///< Stores a list of factions and the current rank
            std::string mBirthSignId;
            int mReputation, mBounty;
            std::vector<MyGUI::Widget*> mSkillWidgets; //< Skills and other information
            std::set<std::string> mExpelled;

            bool mChanged;
            const int mMinFullWidth;

        protected:
            virtual void onPinToggled();
            virtual void onTitleDoubleClicked();
    };
}
#endif
