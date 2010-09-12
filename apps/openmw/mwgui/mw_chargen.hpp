#ifndef MWGUI_CHARGEN_H
#define MWGUI_CHARGEN_H

#include <components/esm_store/store.hpp>

#include <openengine/gui/layout.hpp>

#include <boost/array.hpp>

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

	class RaceDialog : public OEngine::GUI::Layout
	{
	public:
		RaceDialog();

		void selectPreviousSex();
		void selectNextSex();

		void selectPreviousFace();
		void selectNextFace();

		void selectPreviousHair();
		void selectNextHair();

	protected:
		void onHeadRotate(MyGUI::VScroll* _sender, size_t _position);

		void onSelectPreviousSex(MyGUI::Widget* _sender);
		void onSelectNextSex(MyGUI::Widget* _sender);

		void onSelectPreviousFace(MyGUI::Widget* _sender);
		void onSelectNextFace(MyGUI::Widget* _sender);

		void onSelectPreviousHair(MyGUI::Widget* _sender);
		void onSelectNextHair(MyGUI::Widget* _sender);

		void onSelectRace(MyGUI::List* _sender, size_t _index);

	private:
		void updateRaces();
		void updateSkills();
		void updateSpecials();

		MyGUI::CanvasPtr  appearanceBox;
		MyGUI::ListPtr    raceList;
		MyGUI::HScrollPtr headRotate;

		MyGUI::WidgetPtr skillList;
		std::vector<MyGUI::WidgetPtr> skillItems;

		MyGUI::WidgetPtr specialsList;
		std::vector<MyGUI::WidgetPtr> specialsItems;

		int sexIndex, faceIndex, hairIndex;
		int faceCount, hairCount;
	};
}
#endif
