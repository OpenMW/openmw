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
#ifndef __MYGUI_RENDER_MANAGER_H__
#define __MYGUI_RENDER_MANAGER_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_RenderFormat.h"
#include "MyGUI_ITexture.h"
#include "MyGUI_IVertexBuffer.h"
#include "MyGUI_IRenderTarget.h"

namespace MyGUI
{

	class MYGUI_EXPORT RenderManager
	{
	public:
		RenderManager();
		virtual ~RenderManager() = 0;

		static RenderManager& getInstance();
		static RenderManager* getInstancePtr();

		virtual IVertexBuffer* createVertexBuffer() = 0;
		virtual void destroyVertexBuffer(IVertexBuffer* _buffer) = 0;

		virtual ITexture* createTexture(const std::string& _name) = 0;
		virtual void destroyTexture(ITexture* _texture) = 0;
		virtual ITexture* getTexture(const std::string& _name) = 0;

		//FIXME возможно перенести в структуру о рендер таргете
		virtual const IntSize& getViewSize() const = 0;

		virtual VertexColourType getVertexFormat() = 0;

		virtual bool isFormatSupported(PixelFormat _format, TextureUsage _usage) { return true; }

#if MYGUI_DEBUG_MODE == 1
		virtual bool checkTexture(ITexture* _texture) { return true; }
#endif

	private:
		static RenderManager* msInstance;
		bool mIsInitialise;
	};

} // namespace MyGUI

#endif // __MYGUI_RENDER_MANAGER_H__
