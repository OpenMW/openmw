/*!
	@file
	@author		Albert Semenov
	@date		09/2009
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
#ifndef __MYGUI_TEXTURE_UTILITY_H__
#define __MYGUI_TEXTURE_UTILITY_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Colour.h"
#include "MyGUI_RenderFormat.h"

namespace MyGUI
{

	namespace texture_utility
	{

		MYGUI_EXPORT const IntSize& getTextureSize(const std::string& _texture, bool _cache = true);
		MYGUI_EXPORT uint32 toColourARGB(const Colour& _colour);

		MYGUI_FORCEINLINE void convertColour(uint32& _colour, VertexColourType _format)
		{
			if (_format == VertexColourType::ColourABGR)
				_colour = ((_colour & 0x00FF0000) >> 16) | ((_colour & 0x000000FF) << 16) | (_colour & 0xFF00FF00);
		}


	} // namespace texture_utility

} // namespace MyGUI

#endif // __MYGUI_TEXTURE_UTILITY_H__
