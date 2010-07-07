/*!
	@file
	@author		Albert Semenov
	@date		11/2007
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
#include "MyGUI_LayerItem.h"
#include <algorithm>

namespace MyGUI
{

	LayerItem::LayerItem() :
		mLayer(nullptr),
		mLayerNode(nullptr),
		mSaveLayerNode(nullptr),
		mTexture(nullptr)
	{
	}

	void LayerItem::addChildItem(LayerItem* _item)
	{
		mLayerItems.push_back(_item);
		if (mLayerNode != nullptr)
		{
			_item->attachToLayerItemNode(mLayerNode, false);
		}
	}

	void LayerItem::removeChildItem(LayerItem* _item)
	{
		VectorLayerItem::iterator item = std::remove(mLayerItems.begin(), mLayerItems.end(), _item);
		MYGUI_ASSERT(item != mLayerItems.end(), "item not found");
		mLayerItems.erase(item);
	}

	void LayerItem::addChildNode(LayerItem* _item)
	{
		mLayerNodes.push_back(_item);
		if (mLayerNode != nullptr)
		{
			// создаем оверлаппеду новый айтем
			ILayerNode* child_node = mLayerNode->createChildItemNode();
			_item->attachToLayerItemNode(child_node, true);
		}
	}

	void LayerItem::removeChildNode(LayerItem* _item)
	{
		VectorLayerItem::iterator item = std::remove(mLayerNodes.begin(), mLayerNodes.end(), _item);
		MYGUI_ASSERT(item != mLayerNodes.end(), "item not found");
		mLayerNodes.erase(item);
	}

	void LayerItem::addRenderItem(ISubWidget* _item)
	{
		mDrawItems.push_back(_item);
	}

	void LayerItem::removeAllRenderItems()
	{
		detachFromLayerItemNode(false);
		mDrawItems.clear();
	}

	void LayerItem::setRenderItemTexture(ITexture* _texture)
	{
		mTexture = _texture;
		if (mLayerNode)
		{
			ILayerNode* node = mLayerNode;
			// позже сделать детач без текста
			detachFromLayerItemNode(false);
			attachToLayerItemNode(node, false);
		}
	}

	void LayerItem::saveLayerItem()
	{
		mSaveLayerNode = mLayerNode;
	}

	void LayerItem::restoreLayerItem()
	{
		mLayerNode = mSaveLayerNode;
		if (mLayerNode)
		{
			attachToLayerItemNode(mLayerNode, false);
		}
	}

	void LayerItem::attachItemToNode(ILayer* _layer, ILayerNode* _node)
	{
		mLayer = _layer;
		mLayerNode = _node;

		attachToLayerItemNode(_node, true);
	}

	void LayerItem::detachFromLayer()
	{
		// мы уже отдетачены в доску
		if (nullptr == mLayer) return;

		// такого быть не должно
		MYGUI_ASSERT(mLayerNode, "_item->mLayerNode == nullptr");

		// отписываемся от пиккинга
		mLayerNode->detachLayerItem(this);

		// при детаче обнулиться
		ILayerNode * save = mLayerNode;

		// физически отсоединяем
		detachFromLayerItemNode(true);

		// отсоединяем леер и обнуляем у рутового виджета
		mLayer->destroyChildItemNode(save);
		mLayerNode = nullptr;
		mLayer = nullptr;
	}

	void LayerItem::upLayerItem()
	{
		if (mLayerNode) mLayerNode->getLayer()->upChildItemNode(mLayerNode);
	}

	void LayerItem::attachToLayerItemNode(ILayerNode* _item, bool _deep)
	{
		MYGUI_DEBUG_ASSERT(nullptr != _item, "attached item must be valid");

		// сохраняем, чтобы последующие дети могли приаттачиться
		mLayerNode = _item;

		for (VectorSubWidget::iterator skin = mDrawItems.begin(); skin != mDrawItems.end(); ++skin)
		{
			(*skin)->createDrawItem(mTexture, _item);
		}

		for (VectorLayerItem::iterator item = mLayerItems.begin(); item != mLayerItems.end(); ++item)
		{
			(*item)->attachToLayerItemNode(_item, _deep);
		}

		for (VectorLayerItem::iterator item = mLayerNodes.begin(); item != mLayerNodes.end(); ++item)
		{
			// создаем оверлаппеду новый айтем
			if (_deep)
			{
				ILayerNode* child_node = _item->createChildItemNode();
				(*item)->attachToLayerItemNode(child_node, _deep);
			}
		}
	}

	void LayerItem::detachFromLayerItemNode(bool _deep)
	{
		for (VectorLayerItem::iterator item = mLayerItems.begin(); item != mLayerItems.end(); ++item)
		{
			(*item)->detachFromLayerItemNode(_deep);
		}

		for (VectorLayerItem::iterator item = mLayerNodes.begin(); item != mLayerNodes.end(); ++item)
		{
			if (_deep)
			{
				ILayerNode* node = (*item)->mLayerNode;
				(*item)->detachFromLayerItemNode(_deep);
				if (node)
				{
					node->getLayer()->destroyChildItemNode(node);
				}
			}
		}

		// мы уже отаттачены
		ILayerNode* node = mLayerNode;
		if (node)
		{
			//for (VectorWidgetPtr::iterator widget = mWidgetChildSkin.begin(); widget != mWidgetChildSkin.end(); ++widget) (*widget)->_detachFromLayerItemKeeperByStyle(_deep);
			for (VectorSubWidget::iterator skin = mDrawItems.begin(); skin != mDrawItems.end(); ++skin)
			{
				(*skin)->destroyDrawItem();
			}

			// при глубокой очистке, если мы оверлаппед, то для нас создавали айтем
			/*if (_deep && !this->isRootWidget() && mWidgetStyle == WidgetStyle::Overlapped)
			{
				node->destroyItemNode();
			}*/
			// очищаем
			mLayerNode = nullptr;
		}
	}

} // namespace MyGUI
