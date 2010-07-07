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
#ifndef __MYGUI_I_SUB_WIDGET_TEXT_H__
#define __MYGUI_I_SUB_WIDGET_TEXT_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_ISubWidget.h"
#include "MyGUI_Colour.h"

namespace MyGUI
{

	class MYGUI_EXPORT ISubWidgetText : public ISubWidget
	{
		MYGUI_RTTI_DERIVED( ISubWidgetText )

	public:
		virtual ~ISubWidgetText()  { }

		/** Get index of start of selection */
		virtual size_t getTextSelectionStart() { return 0; }
		/** Get index of end of selection */
		virtual size_t getTextSelectionEnd() { return 0; }
		/** Set text selection */
		virtual void setTextSelection(size_t _start, size_t _end) { }

		// интенсивность выделенного текста
		virtual bool getSelectBackground() { return true; }
		virtual void setSelectBackground(bool _normal) { }

		// нужно ли инвертировать выделение
		virtual bool getInvertSelected() { return true; }
		virtual void setInvertSelected(bool _value) { }

		// управление видимостью курсора
		virtual bool isVisibleCursor() { return false; }
		virtual void setVisibleCursor(bool _value) { }

		// управление положением курсора
		virtual size_t getCursorPosition() { return 0; }
		virtual void setCursorPosition(size_t _index) { }

		virtual void setWordWrap(bool _value) { }

		// возвращает положение курсора по произвольному положению
		// позиция абсолютная, без учета смещений
		virtual size_t getCursorPosition(const IntPoint& _point) { return 0; }

		// возвращает положение курсора в обсолютных координатах
		virtual IntCoord getCursorCoord(size_t _position) { return IntCoord(); }

		// возвращает положение курсора в обсолютных координатах
		IntPoint getCursorPoint(size_t _position)
		{
			const IntCoord& coord = getCursorCoord(_position);
			return IntPoint(coord.left + coord.width / 2, coord.top + coord.height / 2);
		}

		// возвращает положение курсора в обсолютных координатах
		IntRect getCursorRect(size_t _position)
		{
			const IntCoord& coord = getCursorCoord(_position);
			return IntRect(coord.left, coord.top, coord.left + coord.width, coord.top + coord.height);
		}

		// возвращает размер текста в пикселях
		virtual IntSize getTextSize() { return IntSize(); }

		// устанавливает смещение текста в пикселях
		virtual void setViewOffset(const IntPoint& _point) { }
		virtual IntPoint getViewOffset() { return IntPoint(); }

		virtual void setCaption(const UString& _value) { }
		virtual const UString& getCaption() { static UString caption; return caption; }

		virtual void setTextColour(const Colour& _value) { }
		virtual const Colour& getTextColour() { return Colour::Zero; }

		virtual void setFontName(const std::string& _value) { }
		virtual const std::string& getFontName() { static std::string name; return name; }

		virtual void setFontHeight(int _value) { }
		virtual int getFontHeight() { return 0; }

		virtual void setTextAlign(Align _value) { }
		virtual Align getTextAlign() { return Align::Default; }

		virtual void setShiftText(bool _value) { }

	};

} // namespace MyGUI

#endif // __MYGUI_I_SUB_WIDGET_TEXT_H__
