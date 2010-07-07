/*!
	@file
	@author		Albert Semenov
	@date		02/2008
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
#ifndef __MYGUI_MAIN_SKIN_H__
#define __MYGUI_MAIN_SKIN_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_SubSkin.h"

namespace MyGUI
{

	class MYGUI_EXPORT MainSkin : public SubSkin
	{
		MYGUI_RTTI_DERIVED( MainSkin )

	public:
		MainSkin();
		virtual ~MainSkin();

	/*internal:*/
		void _setAlign(const IntSize& _oldsize, bool _update);
		void _setAlign(const IntCoord& _oldsize, bool _update) { SubSkin::_setAlign(_oldsize, _update); }
	};

} // namespace MyGUI

#endif // __MYGUI_SUB_SKIN_H__
