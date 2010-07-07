/*!
	@file
	@author		Albert Semenov
	@date		01/2008
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
#ifndef __MYGUI_ACTION_CONTROLLER_H__
#define __MYGUI_ACTION_CONTROLLER_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Types.h"
#include <math.h>

namespace MyGUI
{

	namespace action
	{

		/** Function used for hiding widget with one of controller event */
		void MYGUI_EXPORT actionWidgetHide(Widget* _widget);

		/** Function used for showing widget with one of controller event */
		void MYGUI_EXPORT actionWidgetShow(Widget* _widget);

		/** Function used for destroying widget with one of controller event */
		void MYGUI_EXPORT actionWidgetDestroy(Widget* _widget);

		/** Function for ControllerPosition per frame action : Move with constant speed */
		void MYGUI_EXPORT linearMoveFunction(const IntCoord& _startRect, const IntCoord& _destRect, IntCoord& _result, float _k);

		/** Function for ControllerPosition per frame action : Move with accelerated speed
			if N == 10 then this function is same as linearMoveFunction
			if N > 10 speed will be increasing
			if N < 10 speed will be decreasing
		*/
		template <int N>
		inline void acceleratedMoveFunction(const IntCoord& _startRect, const IntCoord& _destRect, IntCoord& _result, float _current_time)
		{
			float k = (float)pow (_current_time, N/10.f /*3 by default as Accelerated and 0.4 by default as Slowed*/);
			linearMoveFunction(_startRect, _destRect, _result, k);
		}

		/** Function for ControllerPosition per frame action : Move with accelerated speed a bit farther than necessary and then return it back */
		template <int N>
		inline void jumpMoveFunction(const IntCoord& _startRect, const IntCoord& _destRect, IntCoord& _result, float _current_time)
		{
			float k = pow (_current_time, 2) * (-2 - N/10.f) + _current_time * (3 + N/10.f);
			linearMoveFunction(_startRect, _destRect, _result, k);
		}

		/** Function for ControllerPosition per frame action : Start with zero speed increasing half time and then decreasing to zero */
		void MYGUI_EXPORT inertionalMoveFunction(const IntCoord& _startRect, const IntCoord& _destRect, IntCoord& _result, float _current_time);

	} // namespace action

} // namespace MyGUI

#endif // __MYGUI_ACTION_CONTROLLER_H__
