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
#ifndef __MYGUI_HSCROLL_H__
#define __MYGUI_HSCROLL_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_VScroll.h"

namespace MyGUI
{

	class MYGUI_EXPORT HScroll :
		public VScroll
	{
		MYGUI_RTTI_DERIVED( HScroll )

	public:
		HScroll();

		//! @copydoc VScroll::getLineSize()
		virtual int getLineSize();

		//! @copydoc VScroll::setTrackSize(int _value)
		virtual void setTrackSize(int _value);
		//! @copydoc VScroll::getTrackSize()
		virtual int getTrackSize();

	/*internal:*/
		virtual void _initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name);

	protected:
		virtual ~HScroll();

		void baseChangeWidgetSkin(ResourceSkin* _info);

	private:
		void initialiseWidgetSkin(ResourceSkin* _info);
		void shutdownWidgetSkin();

		void TrackMove(int _left, int _top);
		void updateTrack();

	};

} // namespace MyGUI

#endif // __MYGUI_HSCROLL_H__
