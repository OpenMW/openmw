/*!
	@file
	@author		Albert Semenov
	@date		09/2008
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
#ifndef __MYGUI_IMAGE_INFO_H__
#define __MYGUI_IMAGE_INFO_H__

#include "MyGUI_Prerequest.h"

namespace MyGUI
{

	struct ImageItem
	{
		ImageItem() : frame_rate(0) { }

		float frame_rate;
		std::vector<FloatRect> images;
	};

	typedef std::vector<ImageItem> VectorImages;

	struct ImageIndexInfo
	{
		ImageIndexInfo(
			const std::string& _texture,
			const IntSize& _size,
			const float _rate,
			const std::vector<IntPoint>& _frames
			) :
			texture(_texture),
			size(_size),
			rate(_rate),
			frames(_frames)
		{
		}

		const std::string& texture;
		const IntSize& size;
		const float rate;
		const std::vector<IntPoint>& frames;
	};

} // namespace MyGUI

#endif // __MYGUI_IMAGE_INFO_H__
