/*!
	@file
	@author		Albert Semenov
	@date		05/2008
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
#ifndef __MYGUI_WIDGET_TRANSLATE_H__
#define __MYGUI_WIDGET_TRANSLATE_H__

#include "MyGUI_Prerequest.h"

namespace MyGUI
{

	inline int getWidgetWidth(Widget* _widget, bool _vert)
	{
		return _vert ? _widget->getWidth() : _widget->getHeight();
	}

	inline int getWidgetHeight(Widget* _widget, bool _vert)
	{
		return _vert ? _widget->getHeight() : _widget->getWidth();
	}

	inline int getWidgetLeft(Widget* _widget, bool _vert)
	{
		return _vert ? _widget->getLeft() : _widget->getTop();
	}

	inline int getWidgetTop(Widget* _widget, bool _vert)
	{
		return _vert ? _widget->getTop() : _widget->getLeft();
	}

	inline void setWidgetSize(Widget* _widget, int _width, int _height, bool _vert)
	{
		_vert ? _widget->setSize(_width, _height) : _widget->setSize(_height, _width);
	}

	inline void setWidgetCoord(Widget* _widget, int _left, int _top, int _width, int _height, bool _vert)
	{
		_vert ? _widget->setCoord(_left, _top, _width, _height) : _widget->setCoord(_top, _left, _height, _width);
	}

	inline void convertWidgetCoord(IntCoord& _coord, bool _vert)
	{
		if ( ! _vert )
		{
			std::swap(_coord.left, _coord.top);
			std::swap(_coord.width, _coord.height);
		}
	}

} // namespace MyGUI

#endif // __MYGUI_WIDGET_TRANSLATE_H__
