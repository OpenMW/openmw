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
#ifndef __MYGUI_I_TEXTURE_H__
#define __MYGUI_I_TEXTURE_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_IRenderTarget.h"
#include "MyGUI_RenderFormat.h"
#include <string>

namespace MyGUI
{
	class ITexture;

	class MYGUI_EXPORT ITextureInvalidateListener
	{
	public:
		virtual void textureInvalidate(ITexture* _texture) = 0;
	};

	class MYGUI_EXPORT ITexture
	{
	public:
		virtual ~ITexture() { }

		virtual const std::string& getName() const = 0;

		virtual void createManual(int _width, int _height, TextureUsage _usage, PixelFormat _format) = 0;
		virtual void loadFromFile(const std::string& _filename) = 0;
		virtual void saveToFile(const std::string& _filename) = 0;

		virtual void setInvalidateListener(ITextureInvalidateListener* _listener) { }

		virtual void destroy() = 0;

		virtual void* lock(TextureUsage _access) = 0;
		virtual void unlock() = 0;
		virtual bool isLocked() = 0;

		virtual int getWidth() = 0;
		virtual int getHeight() = 0;

		virtual PixelFormat getFormat() = 0;
		virtual TextureUsage getUsage() = 0;
		virtual size_t getNumElemBytes() = 0;

		virtual IRenderTarget* getRenderTarget() { return nullptr; }

	};

} // namespace MyGUI

#endif // __MYGUI_I_TEXTURE_H__
