/*!
	@file
	@author		Albert Semenov
	@date		26/2009
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
#ifndef __MYGUI_SUB_WIDGET_INFO_H__
#define __MYGUI_SUB_WIDGET_INFO_H__

#include "MyGUI_Prerequest.h"

namespace MyGUI
{

	// информация, об одном сабвиджете
	struct SubWidgetInfo
	{
		SubWidgetInfo(const std::string& _type, const IntCoord& _coord, Align _align) :
			coord(_coord),
			align(_align),
			type(_type)
		{
		}

		IntCoord coord;
		Align align;
		std::string type;
	};

	typedef std::vector<SubWidgetInfo> VectorSubWidgetInfo;

} // namespace MyGUI

#endif // __MYGUI_SUB_WIDGET_INFO_H__
