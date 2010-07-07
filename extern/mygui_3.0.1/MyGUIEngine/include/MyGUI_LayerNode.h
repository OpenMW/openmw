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
#ifndef __MYGUI_LAYER_NODE_H__
#define __MYGUI_LAYER_NODE_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_ILayer.h"
#include "MyGUI_ILayerNode.h"
#include "MyGUI_RenderItem.h"

namespace MyGUI
{

	class LayerItem;
	typedef std::vector<RenderItem*> VectorRenderItem;
	typedef std::vector<ILayerItem*> VectorLayerItem;

	class MYGUI_EXPORT LayerNode : public ILayerNode
	{
		MYGUI_RTTI_DERIVED( LayerNode )

	public:
		explicit LayerNode(ILayer* _layer, ILayerNode * _parent = nullptr);
		virtual ~LayerNode();

		// леер, которому мы принадлежим
		virtual ILayer* getLayer() { return mLayer; }

		// возвращает отца или nullptr
		virtual ILayerNode* getParent() { return mParent; }

		// создаем дочерний нод
		virtual ILayerNode* createChildItemNode();
		// удаляем дочерний нод
		virtual void destroyChildItemNode(ILayerNode* _node);

		// поднимаем дочерний нод
		virtual void upChildItemNode(ILayerNode* _node);

		// список детей
		virtual EnumeratorILayerNode getEnumerator();

		// добавляем айтем к ноду
		virtual void attachLayerItem(ILayerItem* _item);
		// удаляем айтем из нода
		virtual void detachLayerItem(ILayerItem* _item);

		// добавляет саб айтем и возвращает рендер айтем
		virtual RenderItem* addToRenderItem(ITexture* _texture, ISubWidget* _item);
		// необходимо обновление нода
		virtual void outOfDate(RenderItem* _item);

		// возвращает виджет по позиции
		virtual ILayerItem* getLayerItemByPoint(int _left, int _top);

		// рисует леер
		virtual void renderToTarget(IRenderTarget* _target, bool _update);

		virtual void dumpStatisticToLog(size_t _level);

		bool isOutOfDate() { return mOutOfDate; }

	protected:
		void updateCompression();

	protected:
		// список двух очередей отрисовки, для сабскинов и текста
		VectorRenderItem mFirstRenderItems;
		VectorRenderItem mSecondRenderItems;

		// список всех рутовых виджетов
		// у перекрывающегося слоя здесь только один
		VectorLayerItem mLayerItems;

		// список такиж как мы, для построения дерева
		VectorILayerNode mChildItems;

		ILayerNode * mParent;
		ILayer* mLayer;
		bool mOutOfDate;
	};

} // namespace MyGUI

#endif // __MYGUI_LAYER_NODE_H__
