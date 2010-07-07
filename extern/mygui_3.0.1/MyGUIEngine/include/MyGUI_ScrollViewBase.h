/*!
	@file
	@author		Albert Semenov
	@date		10/2008
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
#ifndef __MYGUI_SCROLL_VIEW_BASE_H__
#define __MYGUI_SCROLL_VIEW_BASE_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Types.h"

namespace MyGUI
{

	class ScrollViewBase
	{

	protected:
		ScrollViewBase();
		virtual ~ScrollViewBase() { }

		void updateScrollSize();
		void updateScrollPosition();

		// размер данных
		virtual IntSize getContentSize() { return IntSize(); }
		// смещение данных
		virtual IntPoint getContentPosition() { return IntPoint(); }
		virtual void setContentPosition(const IntPoint& _value) { }
		// размер окна, через которые видно данные
		virtual IntSize getViewSize() const { return IntSize(); }
		// размер на который прокручиваются данные при щелчке по скролу
		virtual size_t getHScrollPage() { return 1; }
		virtual size_t getVScrollPage() { return 1; }

		virtual Align getContentAlign() { return Align::Center; }

		virtual void eraseContent() { }

	protected:
		VScroll* mVScroll;
		HScroll* mHScroll;
		Widget* mClient;

		bool mVisibleHScroll;
		bool mVisibleVScroll;

		size_t mVRange;
		size_t mHRange;

		// изменяется ли содержимое при ресайзе
		bool mChangeContentByResize;

	};

} // namespace MyGUI

#endif // __MYGUI_SCROLL_VIEW_BASE_H__
