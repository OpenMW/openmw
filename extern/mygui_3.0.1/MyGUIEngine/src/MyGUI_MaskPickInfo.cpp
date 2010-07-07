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
#include "MyGUI_Precompiled.h"
#include "MyGUI_MaskPickInfo.h"
#include "MyGUI_ResourceManager.h"
#include "MyGUI_RenderManager.h"
#include "MyGUI_DataManager.h"

namespace MyGUI
{

	bool MaskPickInfo::load(const std::string& _file)
	{
		if (!DataManager::getInstance().isDataExist(_file))
			return false;

		RenderManager& render = RenderManager::getInstance();
		ITexture* texture = render.createTexture(_file);
		texture->loadFromFile(_file);

		uint8 * buffer = (uint8*)texture->lock(TextureUsage::Read);
		if (buffer == 0)
		{
			render.destroyTexture(texture);
			return false;
		}

		size_t pixel_size = texture->getNumElemBytes();

		width = texture->getWidth();
		height = texture->getHeight();
		size_t size = width * height;
		data.resize(size);

		size_t pos = 0;
		for (size_t pos_pix=0; pos_pix<size; pos_pix++)
		{
			uint8 white = 0;
			for (size_t in_pix=0; in_pix<pixel_size; in_pix++)
			{
				if (0xFF != buffer[pos])
				{
					white = 1;
				}
				pos++;
			}

			data[pos_pix] = white;
		}

		texture->unlock();
		render.destroyTexture(texture);

		return true;
	}

} // namespace MyGUI
