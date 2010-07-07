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
#ifndef __MYGUI_MASK_PICK_INFO_H__
#define __MYGUI_MASK_PICK_INFO_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Types.h"

namespace MyGUI
{

	class MYGUI_EXPORT MaskPickInfo
	{
	public:
		MaskPickInfo() : width(0), height(0) { }

		bool load(const std::string& _file);

		bool pick(const IntPoint& _point, const IntCoord& _coord) const
		{
			if ((0 == _coord.width) || (0 == _coord.height)) return false;

			int x = ((_point.left * width)-1) / _coord.width;
			int y = ((_point.top * height)-1) / _coord.height;

			return 0 != data[(size_t)(y * width + x)];
		}

		bool empty() const
		{
			return data.empty();
		}

	private:
		std::vector<uint8> data;
		int width, height;
	};

} // namespace MyGUI

#endif // __MYGUI_MASK_PICK_INFO_H__
