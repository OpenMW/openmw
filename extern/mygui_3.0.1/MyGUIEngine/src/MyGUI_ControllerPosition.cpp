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
#include "MyGUI_Precompiled.h"
#include "MyGUI_ControllerPosition.h"
#include "MyGUI_Gui.h"
#include "MyGUI_InputManager.h"
#include "MyGUI_WidgetManager.h"
#include "MyGUI_Widget.h"
#include "MyGUI_ActionController.h"

namespace MyGUI
{

	ControllerPosition::ControllerPosition() :
		mTime(1),
		mElapsedTime(0),
		mCalcPosition(false),
		mCalcSize(false)
	{
	}

	void ControllerPosition::setCoord(const IntCoord& _destCoord)
	{
		mDestCoord = _destCoord;
		mCalcPosition = true;
		mCalcSize = true;
	}

	void ControllerPosition::setSize(const IntSize& _destSize)
	{
		mDestCoord.width = _destSize.width;
		mDestCoord.height = _destSize.height;
		mCalcPosition = false;
		mCalcSize = true;
	}

	void ControllerPosition::setPosition(const IntPoint& _destPoint)
	{
		mDestCoord.left = _destPoint.left;
		mDestCoord.top = _destPoint.top;
		mCalcPosition = true;
		mCalcSize = false;
	}

	void ControllerPosition::prepareItem(Widget* _widget)
	{
		MYGUI_DEBUG_ASSERT(mTime > 0, "Time must be > 0");

		mStartCoord = _widget->getCoord();

		// вызываем пользовательский делегат для подготовки
		eventPreAction(_widget);
	}

	bool ControllerPosition::addTime(Widget* _widget, float _time)
	{
		mElapsedTime += _time;

		if (mElapsedTime < mTime)
		{
			IntCoord coord;
			eventFrameAction(mStartCoord, mDestCoord, coord, mElapsedTime/mTime);
			if (mCalcPosition)
			{
				if (mCalcSize) _widget->setCoord(coord);
				else _widget->setPosition(coord.point());
			}
			else if (mCalcSize) _widget->setSize(coord.size());

			// вызываем пользовательский делегат обновления
			eventUpdateAction(_widget);

			return true;
		}

		// поставить точно в конец
		IntCoord coord;
		eventFrameAction(mStartCoord, mDestCoord, coord, 1.0f);
		if (mCalcPosition)
		{
			if (mCalcSize) _widget->setCoord(coord);
			else _widget->setPosition(coord.point());
		}
		else if (mCalcSize) _widget->setSize(coord.size());

		// вызываем пользовательский делегат обновления
		eventUpdateAction(_widget);

		// вызываем пользовательский делегат пост обработки
		eventPostAction(_widget);

		return false;
	}

	void ControllerPosition::setProperty(const std::string& _key, const std::string& _value)
	{
		if (_key == "Time") setTime(utility::parseValue<float>(_value));
		else if (_key == "Coord") setCoord(utility::parseValue<IntCoord>(_value));
		else if (_key == "Size") setSize(utility::parseValue<IntSize>(_value));
		else if (_key == "Position") setPosition(utility::parseValue<IntPoint>(_value));
		else if (_key == "Function") setFunction(_value);
	}

	void ControllerPosition::setFunction(const std::string& _value)
	{
		if (_value == "Inertional") setAction(MyGUI::newDelegate(action::inertionalMoveFunction));
		else if (_value == "Accelerated") setAction(MyGUI::newDelegate(action::acceleratedMoveFunction<30>));
		else if (_value == "Slowed") setAction(MyGUI::newDelegate(action::acceleratedMoveFunction<4>));
		else if (_value == "Jump") setAction(MyGUI::newDelegate(action::jumpMoveFunction<5>));
	}

} // namespace MyGUI
