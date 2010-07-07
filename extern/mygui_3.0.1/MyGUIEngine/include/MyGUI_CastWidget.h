/*!
	@file
	@author		Albert Semenov
	@date		12/2007
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
#ifndef __MYGUI_CAST_WIDGET_H__
#define __MYGUI_CAST_WIDGET_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Widget.h"

namespace MyGUI
{

#ifndef MYGUI_DONT_USE_OBSOLETE

	// шаблонный класс для проверки типа виджета
	template <typename T>
	MYGUI_OBSOLETE("use : template<typename Type> Type* Widget::castType(bool _throw)")
	T* castWidget(Widget * _widget)
	{
		MYGUI_DEBUG_ASSERT(nullptr != _widget, "Error static cast, widget == nullptr");
		return _widget->castType<T>();
	}

#endif // MYGUI_DONT_USE_OBSOLETE

} // namespace MyGUI

#endif // __MYGUI_CAST_WIDGET_H__
