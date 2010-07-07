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
#ifndef __MYGUI_SHARED_LAYER_H__
#define __MYGUI_SHARED_LAYER_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Types.h"
#include "MyGUI_ILayer.h"
#include "MyGUI_SharedLayerNode.h"

namespace MyGUI
{

	class MYGUI_EXPORT SharedLayer :
		public ILayer
	{
		MYGUI_RTTI_DERIVED( SharedLayer )

	public:
		SharedLayer();
		virtual ~SharedLayer();

		virtual void deserialization(xml::ElementPtr _node, Version _version);

		// создаем дочерний нод
		virtual ILayerNode* createChildItemNode();
		// удаляем дочерний нод
		virtual void destroyChildItemNode(ILayerNode* _node);

		// поднимаем дочерний нод
		virtual void upChildItemNode(ILayerNode* _node);

		// список детей
		virtual EnumeratorILayerNode getEnumerator();

		// возвращает виджет по позиции
		virtual ILayerItem* getLayerItemByPoint(int _left, int _top);

		virtual IntPoint getPosition(int _left, int _top) const;

		virtual const IntSize& getSize() const;

		// рисует леер
		virtual void renderToTarget(IRenderTarget* _target, bool _update);

		virtual void dumpStatisticToLog();

	protected:
		bool mIsPick;
		SharedLayerNode* mChildItem;
	};

} // namespace MyGUI

#endif // __MYGUI_SHARED_LAYER_H__
