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
#ifndef __MYGUI_CHILD_SKIN_INFO_H__
#define __MYGUI_CHILD_SKIN_INFO_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_WidgetStyle.h"

namespace MyGUI
{

	struct MYGUI_EXPORT ChildSkinInfo
	{
		ChildSkinInfo() : style(WidgetStyle::Child), align(Align::Default) { }

		ChildSkinInfo(const std::string& _type, const WidgetStyle& _style, const std::string& _skin, const IntCoord& _coord, const Align& _align, const std::string& _layer, const std::string& _name) :
			type(_type),
			skin(_skin),
			name(_name),
			layer(_layer),
			style(_style),
			coord(_coord),
			align(_align)
		{
			// set Child style by default
			if (style == WidgetStyle::MAX) style = WidgetStyle::Child;
		}

		void addParam(const std::string& _key, const std::string& _value)
		{
			params[_key] = _value;
		}

		std::string findValue(const std::string& _key) const
		{
			MapString::const_iterator iter = params.find(_key);
			if (iter != params.end()) return iter->second;
			return "";
		}

		std::string type, skin, name, layer;
		WidgetStyle style;
		IntCoord coord;
		Align align;
		MapString params;
	};

	typedef std::vector<ChildSkinInfo> VectorChildSkinInfo;

} // namespace MyGUI


#endif // __MYGUI_CHILD_SKIN_INFO_H__
