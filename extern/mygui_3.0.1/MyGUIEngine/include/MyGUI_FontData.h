/*!
	@file
	@author		Albert Semenov
	@date		06/2009
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
#ifndef __MYGUI_FONT_DATA_H__
#define __MYGUI_FONT_DATA_H__

#include "MyGUI_Prerequest.h"

namespace MyGUI
{

	struct MYGUI_EXPORT FontCodeType
	{
		enum Enum
		{
			Selected = 6,
			SelectedBack = 7,
			Cursor = 8,
			Tab = 9,
			LF = 0x000A,
			CR = 0x000D,
			Space = 0x0020,
			LatinStart = 0x0021,
			NEL = 0x0085,
			LatinEnd = 0x00A6,
			MAX
		};

		FontCodeType(Enum _value = MAX) : value(_value) { }

		friend bool operator == (FontCodeType const& a, FontCodeType const& b) { return a.value == b.value; }
		friend bool operator != (FontCodeType const& a, FontCodeType const& b) { return a.value != b.value; }

	private:
		Enum value;
	};

	// информация об одном символе
	struct GlyphInfo
	{
		FloatRect uvRect;
		Char codePoint;
		int width;

		GlyphInfo() : codePoint(0), width(0) { }
	};

	typedef std::vector<GlyphInfo> VectorGlyphInfo;

	// информация об диапазоне
	//FIXME move to std::pair
	class PairCodePoint
	{
	public:
		PairCodePoint() : first(0), last(0) { }
		PairCodePoint(Char _first, Char _last) : first(_first), last(_last) { }

		// проверяет входит ли символ в диапазон
		bool isExist(Char _code) const { return _code >= first && _code <= last; }

	public:
		Char first;
		Char last;
	};

	// инфомация о диапазоне символов
	class RangeInfo
	{
	public:
		RangeInfo() : first(0), last(0) { }
		RangeInfo(Char _first, Char _last) :
			first(_first),
			last(_last)
		{
			range.resize(last - first + 1);
		}

		// проверяет входит ли символ в диапазон
		bool isExist(Char _code) const { return _code >= first && _code <= last; }

		// возвращает указатель на глиф, или 0, если код не входит в диапазон
		GlyphInfo* getInfo(Char _code) { return isExist(_code) ? &range[_code - first] : nullptr; }
		void setInfo(Char _code, GlyphInfo* _value) { if (isExist(_code)) range[_code - first] = *_value; }

	public:
		Char first;
		Char last;
		VectorGlyphInfo range;
	};

	// FIXME move to resource font
	class PairCodeCoord
	{
	public:
		PairCodeCoord() : code(0) { }
		PairCodeCoord(Char _code, const IntCoord& _coord) :
			code(_code),
			coord(_coord)
		{
		}

		bool operator < (const PairCodeCoord& _value) const { return code < _value.code; }

		Char code;
		IntCoord coord;
	};

} // namespace MyGUI

#endif // __MYGUI_FONT_DATA_H__
