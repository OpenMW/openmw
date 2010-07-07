/*!
	@file
	@author		Albert Semenov
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

#include "MyGUI_Precompiled.h"
#include "MyGUI_ActionController.h"
#include "MyGUI_Widget.h"
#include "MyGUI_WidgetManager.h"

namespace MyGUI
{

	namespace action
	{

		void actionWidgetHide(Widget* _widget)
		{
			_widget->setVisible(false);
		}

		void actionWidgetShow(Widget* _widget)
		{
			_widget->setVisible(true);
		}

		void actionWidgetDestroy(Widget* _widget)
		{
			WidgetManager::getInstance().destroyWidget(_widget);
		}

		void linearMoveFunction(const IntCoord& _startRect, const IntCoord& _destRect, IntCoord& _result, float _k)
		{
			_result.set(_startRect.left   - int( float(_startRect.left   - _destRect.left)   * _k ),
			            _startRect.top    - int( float(_startRect.top    - _destRect.top)    * _k ),
			            _startRect.width  - int( float(_startRect.width  - _destRect.width)  * _k ),
			            _startRect.height - int( float(_startRect.height - _destRect.height) * _k )
			           );
		}

		void inertionalMoveFunction(const IntCoord& _startRect, const IntCoord& _destRect, IntCoord& _result, float _current_time)
		{
#ifndef M_PI
			const float M_PI = 3.141593f;
#endif
			float k = sin(M_PI * _current_time - M_PI/2.0f);
			if (k<0) k = (-pow(-k, 0.7f) + 1)/2;
			else k = (pow(k, 0.7f) + 1)/2;
			linearMoveFunction(_startRect, _destRect, _result, k);
		}

	} // namespace action

} // namespace MyGUI
