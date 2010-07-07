/*!
	@file
	@author		Albert Semenov
	@date		04/2009
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
#ifndef __MYGUI_RENDER_FORMAT_H__
#define __MYGUI_RENDER_FORMAT_H__

#include "MyGUI_Macros.h"

namespace MyGUI
{

	struct MYGUI_EXPORT VertexColourType
	{
		enum Enum
		{
			ColourARGB, // D3D style compact colour
			ColourABGR, // GL style compact colour
			MAX
		};

		VertexColourType(Enum _value = MAX) : value(_value) { }

		friend bool operator == (VertexColourType const& a, VertexColourType const& b) { return a.value == b.value; }
		friend bool operator != (VertexColourType const& a, VertexColourType const& b) { return a.value != b.value; }

	private:
		Enum value;
	};

	struct MYGUI_EXPORT PixelFormat
	{
		enum Enum
		{
			Unknow,
			L8, // 1 byte pixel format, 1 byte luminance
			L8A8, // 2 byte pixel format, 1 byte luminance, 1 byte alpha
			R8G8B8, // 24-bit pixel format, 8 bits for red, green and blue.
			R8G8B8A8 // 32-bit pixel format, 8 bits for red, green, blue and alpha.
		};

		PixelFormat(Enum _value = Unknow) : value(_value) { }

		friend bool operator == (PixelFormat const& a, PixelFormat const& b) { return a.value == b.value; }
		friend bool operator != (PixelFormat const& a, PixelFormat const& b) { return a.value != b.value; }

	private:
		Enum value;
	};

	struct MYGUI_EXPORT TextureUsage
	{
		enum Enum
		{
			Default = MYGUI_FLAG_NONE,
			Static = MYGUI_FLAG(0),
			Dynamic = MYGUI_FLAG(1),
			Stream = MYGUI_FLAG(2),
			Read = MYGUI_FLAG(3),
			Write = MYGUI_FLAG(4),
			RenderTarget = MYGUI_FLAG(5)
		};

		TextureUsage(Enum _value = Default) : value(_value) { }

		friend bool operator == (TextureUsage const& a, TextureUsage const& b) { return a.value == b.value; }
		friend bool operator != (TextureUsage const& a, TextureUsage const& b) { return a.value != b.value; }

		TextureUsage& operator |= (TextureUsage const& _other) { value = Enum(int(value) | int(_other.value)); return *this; }
		friend TextureUsage operator | (Enum const& a, Enum const& b) { return TextureUsage(Enum(int(a) | int(b))); }
		friend TextureUsage operator | (TextureUsage const& a, TextureUsage const& b) { return TextureUsage(Enum(int(a.value) | int(b.value))); }

		bool isValue(Enum _value) { return 0 != (value & _value); }

	private:
		Enum value;
	};

} // namespace MyGUI


#endif // __MYGUI_RENDER_FORMAT_H__
