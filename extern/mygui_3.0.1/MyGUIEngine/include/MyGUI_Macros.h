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
#ifndef __MYGUI_MACROS_H__
#define __MYGUI_MACROS_H__

namespace MyGUI
{

	const size_t ITEM_NONE = ~(size_t)0;
	const int DEFAULT ((int)-1);
	const float ALPHA_MAX = 1.0f;
	const float ALPHA_MIN = 0.0f;

	//FIXME заменить на шаблоны
	#define MYGUI_FLAG_NONE  0
	#define MYGUI_FLAG(num)  (1<<(num))

} // namespace MyGUI


#endif // __MYGUI_MACROS_H__
