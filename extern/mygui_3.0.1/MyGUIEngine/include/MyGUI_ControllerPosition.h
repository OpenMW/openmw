/*!
	@file
	@author		Evmenov Georgiy
	@date		03/2008
	@module
*/
/*
	This file is part of MyGUI.

	MyGUI is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MyGUI is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with MyGUI.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __MYGUI_CONTROLLER_POSITION_H__
#define __MYGUI_CONTROLLER_POSITION_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Delegate.h"
#include "MyGUI_Types.h"
#include "MyGUI_WidgetDefines.h"
#include "MyGUI_ControllerItem.h"

namespace MyGUI
{

	/** This controller used for smooth changing position of widget in time */
	class MYGUI_EXPORT ControllerPosition :
		public ControllerItem
	{
		MYGUI_RTTI_DERIVED( ControllerPosition )

	public:
		typedef delegates::CDelegate4<const IntCoord&, const IntCoord&, IntCoord&, float> FrameAction;

		ControllerPosition();
		virtual ~ControllerPosition() { }

		void setCoord(const IntCoord& _value);

		void setSize(const IntSize& _value);

		void setPosition(const IntPoint& _value);

		void setFunction(const std::string& _value);

		/**
			@param _value seconds in which widget planned to reach destination coordinate
		*/
		void setTime(float _value) { mTime = _value; }

		/**
			@param _action applied to widget every frame (see ControllerPosition::eventFrameAction)
		*/
		void setAction(FrameAction::IDelegate* _value) { eventFrameAction = _value; }

		virtual void setProperty(const std::string& _key, const std::string& _value);

	private:
		bool addTime(Widget* _widget, float _time);
		void prepareItem(Widget* _widget);

		float getElapsedTime() { return mElapsedTime; }

		IntCoord mStartCoord;
		IntCoord mDestCoord;
		float mTime;
		float mElapsedTime;

		// controller changing position
		bool mCalcPosition;
		// controller changing size
		bool mCalcSize;

		/** Event : Every frame action while controller exist.\n
			signature : void method(const IntRect& _startRect, const IntRect& _destRect, IntRect& _result, float _current_time)\n
			@param _startRect start coordinate of widget
			@param _destRect destination coordinate
			@param _result resultRect
			@param _current_time elapsed time (_current_time is real elapsed time divided by _time(see constructor) so _current_time == 1 mean that _time seconds elapsed)
		*/
		FrameAction eventFrameAction;
	};

}

#endif // __MYGUI_CONTROLLER_POSITION_H__
