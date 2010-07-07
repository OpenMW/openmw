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
#ifndef __MYGUI_RENDER_TARGET_INFO_H__
#define __MYGUI_RENDER_TARGET_INFO_H__

#include "MyGUI_Prerequest.h"

namespace MyGUI
{

	struct MYGUI_EXPORT RenderTargetInfo
	{
		RenderTargetInfo() :
			maximumDepth(0),
			pixScaleX(1),
			pixScaleY(1),
			hOffset(0),
			vOffset(0),
			aspectCoef(1),
			leftOffset(0),
			topOffset(0)
		{
		}

		void setOffset(int _left, int _top) const
		{
			leftOffset = _left;
			topOffset = _top;
		}

		float maximumDepth;
		float pixScaleX;
		float pixScaleY;
		float hOffset;
		float vOffset;
		float aspectCoef;

		mutable int leftOffset;
		mutable int topOffset;
	};


} // namespace MyGUI

#endif // __MYGUI_RENDER_TARGET_INFO_H__
