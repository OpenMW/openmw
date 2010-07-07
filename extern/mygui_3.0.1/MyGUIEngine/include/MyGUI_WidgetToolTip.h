/*!
	@file
	@author		Albert Semenov
	@date		07/2008
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
#ifndef __MYGUI_WIDGET_TOOLTIP_H__
#define __MYGUI_WIDGET_TOOLTIP_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Types.h"
#include "MyGUI_WidgetToolTip.h"

namespace MyGUI
{

	/** Info about tootip state */
	struct ToolTipInfo
	{
		enum ToolTipType
		{
			Hide,
			Show
		};

		ToolTipInfo(ToolTipType _type) :
			type(_type), index(ITEM_NONE) { }

		ToolTipInfo(ToolTipType _type, size_t _index, const IntPoint& _point) :
			type(_type), index(_index), point(_point) { }

		ToolTipType type;
		size_t index;
		IntPoint point;
	};

} // namespace MyGUI

#endif //__MYGUI_WIDGET_TOOLTIP_H__
