/*!
	@file
	@author		Albert Semenov
	@date		05/2008
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
#ifndef __MYGUI_TILE_RECT_H__
#define __MYGUI_TILE_RECT_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_XmlDocument.h"
#include "MyGUI_Types.h"
#include "MyGUI_ISubWidgetRect.h"
#include "MyGUI_ResourceSkin.h"

namespace MyGUI
{

	class RenderItem;

	class MYGUI_EXPORT TileRect : public ISubWidgetRect
	{
		MYGUI_RTTI_DERIVED( TileRect )

	public:
		TileRect();
		virtual ~TileRect();

		void setAlpha(float _alpha);

		virtual void setVisible(bool _visible);

		virtual void createDrawItem(ITexture* _texture, ILayerNode * _node);
		virtual void destroyDrawItem();

		// метод для отрисовки себя
		virtual void doRender();

		virtual void setStateData(IStateInfo * _data);

	/*internal:*/
		void _updateView();
		void _correctView();

		void _setAlign(const IntSize& _oldsize, bool _update);
		void _setAlign(const IntCoord& _oldcoord, bool _update);

		virtual void _setUVSet(const FloatRect& _rect);
		virtual void _setColour(const Colour& _value);

	protected:
		FloatRect mRectTexture;
		bool mEmptyView;

		uint32 mCurrentColour;

		FloatRect mCurrentTexture;
		IntCoord mCurrentCoord;

		ILayerNode* mNode;
		RenderItem* mRenderItem;

		IntSize mTileSize;
		size_t mCountVertex;

		float mRealTileWidth;
		float mRealTileHeight;

		float mTextureHeightOne;
		float mTextureWidthOne;

		bool mTileH;
		bool mTileV;
	};

} // namespace MyGUI

#endif // __MYGUI_TILE_RECT_H__
