#ifndef MWGUI_CHARGEN_H
#define MWGUI_CHARGEN_H

#include <components/esm_store/store.hpp>

#include <openengine/gui/layout.hpp>

#include <boost/array.hpp>

namespace MWWorld
{
    class Environment;
}

/*
  This file contains classes corresponding to all the dialogs
  for the character generation, layouts are defined in resources/mygui/ *.xml.

  Each class inherites GUI::Layout and loads the XML file, and
  provides some helper functions to manipulate the elements of the
  window.

  The windows are never created or destroyed (except at startup and
  shutdown), they are only hid. You can control visibility with
  setVisible().
 */

namespace MWGui
{
	using namespace MyGUI;

    typedef delegates::CDelegate0 EventHandle_Void;

	class RaceDialog : public OEngine::GUI::Layout
	{
	public:
        RaceDialog(MWWorld::Environment& environment, bool showNext);

        enum Gender
        {
            GM_Male,
            GM_Female
        };

        const std::string &getRace() const { return currentRace; }
        Gender getGender() const { return genderIndex == 0 ? GM_Male : GM_Female; }
        // getFace()
        // getHair()

        void setRace(const std::string &race);
        void setGender(Gender gender) { genderIndex = gender == GM_Male ? 0 : 1; }
        // setFace()
        // setHair()

        // Events

        /** Event : Back button clicked.\n
			signature : void method()\n
		*/
		EventHandle_Void eventBack;

        /** Event : Dialog finished, OK button clicked.\n
			signature : void method()\n
		*/
		EventHandle_Void eventDone;

    protected:
		void onHeadRotate(MyGUI::VScroll* _sender, size_t _position);

		void onSelectPreviousGender(MyGUI::Widget* _sender);
		void onSelectNextGender(MyGUI::Widget* _sender);

		void onSelectPreviousFace(MyGUI::Widget* _sender);
		void onSelectNextFace(MyGUI::Widget* _sender);

		void onSelectPreviousHair(MyGUI::Widget* _sender);
		void onSelectNextHair(MyGUI::Widget* _sender);

		void onSelectRace(MyGUI::List* _sender, size_t _index);

        void onOkClicked(MyGUI::Widget* _sender);
        void onBackClicked(MyGUI::Widget* _sender);

	private:
		void updateRaces();
		void updateSkills();
		void updateSpellPowers();

        MWWorld::Environment& environment;

		MyGUI::CanvasPtr  appearanceBox;
		MyGUI::ListPtr    raceList;
		MyGUI::HScrollPtr headRotate;

		MyGUI::WidgetPtr skillList;
		std::vector<MyGUI::WidgetPtr> skillItems;

		MyGUI::WidgetPtr spellPowerList;
		std::vector<MyGUI::WidgetPtr> spellPowerItems;

        int genderIndex, faceIndex, hairIndex;
		int faceCount, hairCount;

        std::string currentRace;
	};
}
#endif
