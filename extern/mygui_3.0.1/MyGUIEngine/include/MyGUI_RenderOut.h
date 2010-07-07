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
#ifndef __MYGUI_RENDER_OUT_H__
#define __MYGUI_RENDER_OUT_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Utility.h"

namespace MyGUI
{
	namespace implement
	{
		void MYGUI_EXPORT render_out(const std::string& _value);
	}

	template<typename T> inline
	void MYGUI_OUT (T p) { implement::render_out(utility::toString(p)); }

	template<typename T1,  typename T2 > inline
	void MYGUI_OUT (T1 p1, T2 p2) { implement::render_out(utility::toString(p1, p2)); }

	template<typename T1,  typename T2,  typename T3 > inline
	void MYGUI_OUT (T1 p1, T2 p2, T3 p3) { implement::render_out(utility::toString(p1, p2, p3)); }

	template<typename T1,  typename T2,  typename T3, typename T4 > inline
	void MYGUI_OUT (T1 p1, T2 p2, T3 p3, T4 p4) { implement::render_out(utility::toString(p1, p2, p3, p4)); }

	template<typename T1,  typename T2,  typename T3, typename T4, typename T5 > inline
	void MYGUI_OUT (T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) { implement::render_out(utility::toString(p1, p2, p3, p4, p5)); }

	template<typename T1,  typename T2,  typename T3, typename T4, typename T5, typename T6 > inline
	void MYGUI_OUT (T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) { implement::render_out(utility::toString(p1, p2, p3, p4, p5, p6)); }

	template<typename T1,  typename T2,  typename T3, typename T4, typename T5, typename T6, typename T7 > inline
	void MYGUI_OUT (T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) { implement::render_out(utility::toString(p1, p2, p3, p4, p5, p6, p7)); }

	template<typename T1,  typename T2,  typename T3, typename T4, typename T5, typename T6, typename T7, typename T8 > inline
	void MYGUI_OUT (T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) { implement::render_out(utility::toString(p1, p2, p3, p4, p5, p6, p7, p8)); }

	template<typename T1,  typename T2 > inline
	void MYGUI_OUT_SPACES (T1 p1, T2 p2) { implement::render_out(utility::toString(p1, " ", p2)); }

	template<typename T1,  typename T2,  typename T3 > inline
	void MYGUI_OUT_SPACES (T1 p1, T2 p2, T3 p3) { implement::render_out(utility::toString(p1, " ", p2, " ", p3)); }

	template<typename T1,  typename T2,  typename T3, typename T4 > inline
	void MYGUI_OUT_SPACES (T1 p1, T2 p2, T3 p3, T4 p4) { implement::render_out(utility::toString(p1, " ", p2, " ", p3, " ", p4 )); }

} // namespace MyGUI

#endif // __MYGUI_RENDER_OUT_H__
