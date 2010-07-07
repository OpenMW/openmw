/*!
	@file
	@author		Albert Semenov
	@date		02/2008
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
#ifndef __MYGUI_RENDER_ITEM_H__
#define __MYGUI_RENDER_ITEM_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_ISubWidget.h"
#include "MyGUI_IVertexBuffer.h"
#include "MyGUI_VertexData.h"
#include "MyGUI_IRenderTarget.h"

namespace MyGUI
{

	typedef std::pair<ISubWidget*, size_t> DrawItemInfo;
	typedef std::vector<DrawItemInfo> VectorDrawItem;

	class MYGUI_EXPORT RenderItem
	{
	public:
		RenderItem();
		virtual ~RenderItem();

		void renderToTarget(IRenderTarget* _target, bool _update);

		void setTexture(ITexture* _value);
		ITexture* getTexture();

		void addDrawItem(ISubWidget* _item, size_t _count);
		void removeDrawItem(ISubWidget* _item);
		void reallockDrawItem(ISubWidget* _item, size_t _count);

		void outOfDate() { mOutDate = true; }

		size_t getNeedVertexCount() const { return mNeedVertexCount; }
		size_t getVertexCount() const { return mCountVertex; }

		bool getCurrentUpdate() const { return mCurrentUpdate; }
		Vertex* getCurrentVertextBuffer() const { return mCurrentVertext; }

		void setLastVertexCount(size_t _count) { mLastVertextCount = _count; }

		IRenderTarget* getRenderTarget() { return mRenderTarget; }

		bool getCompression();

	private:
#if MYGUI_DEBUG_MODE == 1
		std::string mTextureName;
#endif

		ITexture* mTexture;

		size_t mNeedVertexCount;

		bool mOutDate;
		VectorDrawItem mDrawItems;

		// колличество отрендренных реально вершин
		size_t mCountVertex;

		bool mCurrentUpdate;
		Vertex* mCurrentVertext;
		size_t mLastVertextCount;

		IVertexBuffer* mVertexBuffer;
		IRenderTarget* mRenderTarget;

		bool mCompression;
	};

} // namespace MyGUI

#endif // __MYGUI_RENDER_ITEM_H__
