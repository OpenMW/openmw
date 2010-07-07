/*!
	@file
	@author		Albert Semenov
	@date		11/2007
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
#ifndef __MYGUI_WIDGET_DEFINES_H__
#define __MYGUI_WIDGET_DEFINES_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Enumerator.h"
#include <vector>
#include <map>

namespace MyGUI
{

	class Widget;
	typedef std::vector<Widget*> VectorWidgetPtr;
	typedef std::map<std::string, Widget*> MapWidgetPtr;
	typedef Enumerator<VectorWidgetPtr> EnumeratorWidgetPtr;

} // namespace MyGUI

#endif // __MYGUI_WIDGET_DEFINES_H__
