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
#include "MyGUI_Precompiled.h"
#include "MyGUI_ListCtrl.h"
#include "MyGUI_Button.h"
#include "MyGUI_VScroll.h"
#include "MyGUI_HScroll.h"
#include "MyGUI_ResourceSkin.h"
#include "MyGUI_InputManager.h"
#include "MyGUI_Gui.h"
#include "MyGUI_WidgetTranslate.h"
#include "MyGUI_WidgetManager.h"

namespace MyGUI
{

	ListCtrl::ListCtrl() :
		mIndexSelect(ITEM_NONE),
		mIndexActive(ITEM_NONE),
		mIndexAccept(ITEM_NONE),
		mIndexRefuse(ITEM_NONE),
		mIsFocus(false),
		mItemDrag(nullptr),
		mScrollViewPage(1)
	{
		mChangeContentByResize = true;
	}

	void ListCtrl::_initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name)
	{
		Base::_initialise(_style, _coord, _align, _info, _parent, _croppedParent, _creator, _name);

		initialiseWidgetSkin(_info);
	}

	ListCtrl::~ListCtrl()
	{
		shutdownWidgetSkin();
	}

	size_t ListCtrl::getHScrollPage()
	{
		return mScrollViewPage;
	}

	size_t ListCtrl::getVScrollPage()
	{
		return mScrollViewPage;
	}

	void ListCtrl::baseChangeWidgetSkin(ResourceSkin* _info)
	{
		shutdownWidgetSkin();
		Base::baseChangeWidgetSkin(_info);
		initialiseWidgetSkin(_info);
	}

	void ListCtrl::initialiseWidgetSkin(ResourceSkin* _info)
	{
		// нам нужен фокус клавы
		mNeedKeyFocus = true;
		mDragLayer = "DragAndDrop";

		const MapString& properties = _info->getProperties();
		if (!properties.empty())
		{
			MapString::const_iterator iter = properties.end();
			iter = properties.find("DragLayer");
			if (iter != properties.end()) mDragLayer = iter->second;
		}

		for (VectorWidgetPtr::iterator iter=mWidgetChildSkin.begin(); iter!=mWidgetChildSkin.end(); ++iter)
		{
			if (*(*iter)->_getInternalData<std::string>() == "VScroll")
			{
				MYGUI_DEBUG_ASSERT( ! mVScroll, "widget already assigned");
				mVScroll = (*iter)->castType<VScroll>();
				mVScroll->eventScrollChangePosition = newDelegate(this, &ListCtrl::notifyScrollChangePosition);
			}
			if (*(*iter)->_getInternalData<std::string>() == "HScroll")
			{
				MYGUI_DEBUG_ASSERT( ! mHScroll, "widget already assigned");
				mHScroll = (*iter)->castType<HScroll>();
				mHScroll->eventScrollChangePosition = newDelegate(this, &ListCtrl::notifyScrollChangePosition);
			}
			else if (*(*iter)->_getInternalData<std::string>() == "Client")
			{
				MYGUI_DEBUG_ASSERT( ! mWidgetClient, "widget already assigned");
				mWidgetClient = (*iter);
				mWidgetClient->eventMouseWheel = newDelegate(this, &ListCtrl::notifyMouseWheel);
				mWidgetClient->eventMouseButtonPressed = newDelegate(this, &ListCtrl::notifyMouseButtonPressed);
				mClient = mWidgetClient;
			}
		}
		// сли нет скрола, то клиенская зона не обязательно
		//MYGUI_ASSERT(nullptr != mWidgetClient, "Child Widget Client not found in skin (ListCtrl must have Client) skin ='" << _info->getSkinName() << "'");

		// подписываем клиент для драгэндропа
		_getClientWidget()->_requestGetContainer = newDelegate(this, &ListCtrl::_requestGetContainer);

		updateFromResize();
	}

	void ListCtrl::shutdownWidgetSkin()
	{
		mVScroll = nullptr;
		mHScroll = nullptr;
		mClient = nullptr;
		mWidgetClient = nullptr;
	}

	void ListCtrl::setPosition(const IntPoint& _point)
	{
		Base::setPosition(_point);
	}

	void ListCtrl::setSize(const IntSize& _size)
	{
		Base::setSize(_size);
		updateFromResize();
	}

	void ListCtrl::setCoord(const IntCoord& _coord)
	{
		Base::setCoord(_coord);
		updateFromResize();
	}

	void ListCtrl::updateFromResize()
	{
		updateMetrics();

		updateScrollSize();
		updateScrollPosition();

		_updateAllVisible(ITEM_NONE, true, true);
		_resetContainer(true);
	}

	void ListCtrl::_updateAllVisible(size_t _index, bool _needUpdateContetntSize, bool _update)
	{

		bool change = false;

		int top = 0;
		size_t widget_index = 0;

		for (size_t index=0; index<mItemsInfo.size(); ++index)
		{
			ItemDataInfo& info = mItemsInfo[index];

			// айтем сверху не виден
			if ((top + info.size.height) < (mContentPosition.top))
			{
			}
			// айтем снизу и не виден
			else if (top > ((mContentPosition.top) + _getClientWidget()->getHeight()))
			{
			}
			// айтем встрял в видимость
			else
			{
				Widget* item = getItemWidget(widget_index);
				widget_index++;

				if (index == _index || ITEM_NONE == _index)
				{
					item->_setInternalData((size_t)index);

					item->setPosition(-mContentPosition.left, top - (mContentPosition.top));
					item->setVisible(true);

					IBDrawItemInfo data(index, mIndexSelect, mIndexActive, mIndexAccept, mIndexRefuse, _update, false);

					IntCoord coord(IntPoint(), info.size);
					requestDrawItem(this, item, data, coord);

					if (info.size != coord.size())
						change = true;

					info.size = coord.size();
					item->setSize(mClient->getWidth()/*mContentSize.width*/, info.size.height);
				}

			}

			top += info.size.height;
		}

		// если виджеты еще есть, то их надо скрыть
		while (widget_index < mVectorItems.size())
		{
			Widget* item = mVectorItems[widget_index];
			widget_index ++;

			item->setVisible(false);
			item->_setInternalData((size_t)ITEM_NONE);
		}

		if (change && _needUpdateContetntSize)
		{
			updateMetrics();

			updateScrollSize();
			updateScrollPosition();
		}
	}

	Widget* ListCtrl::getItemWidget(size_t _index)
	{
		// еще нет такого виджета, нуно создать
		if (_index == mVectorItems.size())
		{

			Widget* item = _getClientWidget()->createWidget<Widget>("Default", IntCoord(), Align::Default);

			// вызываем запрос на создание виджета
			requestCreateWidgetItem(this, item);

			item->eventMouseWheel = newDelegate(this, &ListCtrl::notifyMouseWheel);
			item->eventRootMouseChangeFocus = newDelegate(this, &ListCtrl::notifyRootMouseChangeFocus);
			item->eventMouseButtonPressed = newDelegate(this, &ListCtrl::notifyMouseButtonPressed);
			item->eventMouseButtonReleased = newDelegate(this, &ListCtrl::notifyMouseButtonReleased);
			item->eventMouseButtonDoubleClick = newDelegate(this, &ListCtrl::notifyMouseButtonDoubleClick);
			item->eventMouseDrag = newDelegate(this, &ListCtrl::notifyMouseDrag);
			item->_requestGetContainer = newDelegate(this, &ListCtrl::_requestGetContainer);
			item->eventKeyButtonPressed = newDelegate(this, &ListCtrl::notifyKeyButtonPressed);
			item->eventKeyButtonReleased = newDelegate(this, &ListCtrl::notifyKeyButtonReleased);

			mVectorItems.push_back(item);
		}

		// запрашивать только последовательно
		MYGUI_ASSERT_RANGE(_index, mVectorItems.size(), "ListCtrl::getItemWidget");

		return mVectorItems[_index];
	}

	void ListCtrl::onMouseWheel(int _rel)
	{
		notifyMouseWheel(nullptr, _rel);

		Base::onMouseWheel(_rel);
	}

	void ListCtrl::onKeySetFocus(Widget* _old)
	{
		mIsFocus = true;
		setState("pushed");

		Base::onKeySetFocus(_old);
	}

	void ListCtrl::onKeyLostFocus(Widget* _new)
	{
		mIsFocus = false;
		setState("normal");

		Base::onKeyLostFocus(_new);
	}

	void ListCtrl::resetCurrentActiveItem()
	{
		// сбрасываем старую подсветку
		if (mIndexActive != ITEM_NONE)
		{
			//size_t start = (size_t)mFirstVisibleIndex;
			size_t index = mIndexActive;
			mIndexActive = ITEM_NONE;

			//FIXME потом только один попробовать обновить
			_updateAllVisible(index, true, false);

			// если видим, то обновляем
			/*if ((mIndexActive >= start) && (mIndexActive < (start + mVectorItems.size())))
			{
				IBDrawItemInfo data(index, mIndexSelect, mIndexActive, mIndexAccept, mIndexRefuse, false, false);

				IntCoord coord(IntPoint(), mItemsInfo[index].size);

				requestDrawItem(this, mVectorItems[mIndexActive - start], data, coord);

				mItemsInfo[index].size = coord.size();

			}*/
		}
	}

	void ListCtrl::findCurrentActiveItem()
	{
		MYGUI_DEBUG_ASSERT(mIndexActive == ITEM_NONE, "use : resetCurrentActiveItem() before findCurrentActiveItem()");

		const IntPoint& point = InputManager::getInstance().getMousePositionByLayer();

		// сначала проверяем клиентскую зону
		const IntRect& rect = _getClientWidget()->getAbsoluteRect();
		if ((point.left < rect.left) || (point.left > rect.right) || (point.top < rect.top) || (point.top > rect.bottom))
		{
			return;
		}

		for (size_t pos=0; pos<mVectorItems.size(); ++pos)
		{
			Widget* item = mVectorItems[pos];
			const IntRect& abs_rect = item->getAbsoluteRect();
			if ((point.left>= abs_rect.left) && (point.left <= abs_rect.right) && (point.top>= abs_rect.top) && (point.top <= abs_rect.bottom))
			{

				size_t index = calcIndexByWidget(item);
				// при переборе индекс может быть больше, так как может создасться сколько угодно
				if (index < mItemsInfo.size())
				{

					mIndexActive = index;
					//FIXME потом только один попробовать обновить
					_updateAllVisible(index, true, false);

					/*IBDrawItemInfo data(index, mIndexSelect, mIndexActive, mIndexAccept, mIndexRefuse, false, false);
					IntCoord coord(IntPoint(), mItemsInfo[index].size);
					requestDrawItem(this, item, data, coord);
					mItemsInfo[index].size = coord.size();*/

				}

				break;
			}
		}
	}

	void ListCtrl::_requestGetContainer(Widget* _sender, Widget*& _container, size_t& _index)
	{
		if (_sender == _getClientWidget())
		{
			_container = this;
			_index = ITEM_NONE;
		}
		else
		{
			size_t index = calcIndexByWidget(_sender);
			if (index < mItemsInfo.size())
			{
				_container = this;
				_index = index;
			}
		}
	}

	void ListCtrl::_setContainerItemInfo(size_t _index, bool _set, bool _accept)
	{
		if (_index == ITEM_NONE) return;
		MYGUI_ASSERT_RANGE(_index, mItemsInfo.size(), "ListCtrl::_setContainerItemInfo");

		mIndexAccept = (_set && _accept ) ? _index : ITEM_NONE;
		mIndexRefuse = (_set && !_accept) ? _index : ITEM_NONE;

		//FIXME потом только один попробовать обновить
		_updateAllVisible(_index, true, false);

		/*size_t start = (size_t)mFirstVisibleIndex;
		if ((_index >= start) && (_index < (start + mVectorItems.size())))
		{

			IBDrawItemInfo data(_index, mIndexSelect, mIndexActive, mIndexAccept, mIndexRefuse, false, false);

			IntCoord coord(IntPoint(), mItemsInfo[_index].size);

			requestDrawItem(this, mVectorItems[_index - start], data, coord);

			mItemsInfo[_index].size = coord.size();

		}*/
	}

	void ListCtrl::setItemDataAt(size_t _index, Any _data)
	{
		MYGUI_ASSERT_RANGE(_index, mItemsInfo.size(), "ListCtrl::setItemData");
		mItemsInfo[_index].data = _data;

		//FIXME потом только один попробовать обновить
		_updateAllVisible(_index, true, true);

		/*size_t start = (size_t)mFirstVisibleIndex;
		if ((_index >= start) && (_index < (start + mVectorItems.size())))
		{
			IBDrawItemInfo data(_index, mIndexSelect, mIndexActive, mIndexAccept, mIndexRefuse, true, false);
			IntCoord coord(IntPoint(), mItemsInfo[_index].size);
			requestDrawItem(this, mVectorItems[_index - start], data, coord);
			mItemsInfo[_index].size = coord.size();
		}*/

		_resetContainer(true);
	}

	void ListCtrl::insertItemAt(size_t _index, Any _data)
	{
		MYGUI_ASSERT_RANGE_INSERT(_index, mItemsInfo.size(), "ListCtrl::insertItemAt");
		if (_index == ITEM_NONE) _index = mItemsInfo.size();

		_resetContainer(false);

		resetCurrentActiveItem();

		mItemsInfo.insert(mItemsInfo.begin() + _index, ItemDataInfo(_data));

		// расчитываем новый индекс выделения
		if (mIndexSelect != ITEM_NONE)
		{
			if (mIndexSelect >= _index)
			{
				mIndexSelect ++;
			}
		}

		updateScrollSize();
		updateScrollPosition();

		findCurrentActiveItem();

		_updateAllVisible(ITEM_NONE, true, true);
	}

	void ListCtrl::removeItemAt(size_t _index)
	{
		MYGUI_ASSERT_RANGE(_index, mItemsInfo.size(), "ListCtrl::removeItemAt");

		_resetContainer(false);
		resetCurrentActiveItem();

		mItemsInfo.erase(mItemsInfo.begin() + _index);

		// расчитываем новый индекс выделения
		if (mIndexSelect != ITEM_NONE)
		{
			if (mItemsInfo.empty())
			{
				mIndexSelect = ITEM_NONE;
			}
			else if ((mIndexSelect > _index) || (mIndexSelect == mItemsInfo.size()))
			{
				mIndexSelect --;
			}
		}

		updateScrollSize();
		updateScrollPosition();

		findCurrentActiveItem();

		_updateAllVisible(ITEM_NONE, true, true);
	}

	void ListCtrl::removeAllItems()
	{
		if (0 == mItemsInfo.size()) return;
		_resetContainer(false);

		mItemsInfo.clear();

		mIndexSelect = ITEM_NONE;
		mIndexActive = ITEM_NONE;

		updateScrollSize();
		updateScrollPosition();

		_updateAllVisible(ITEM_NONE, true, true);
	}

	void ListCtrl::redrawItemAt(size_t _index)
	{
		MYGUI_ASSERT_RANGE(_index, mItemsInfo.size(), "ListCtrl::redrawItemAt");

		//FIXME потом только один попробовать обновить
		_updateAllVisible(_index, true, true);

		/*size_t start = (size_t)mFirstVisibleIndex;
		if ((_index >= start) && (_index < (start + mVectorItems.size())))
		{
			IBDrawItemInfo data(_index, mIndexSelect, mIndexActive, mIndexAccept, mIndexRefuse, true, false);
			IntCoord coord(IntPoint(), mItemsInfo[_index].size);
			requestDrawItem(this, mVectorItems[_index - start], data, coord);
			mItemsInfo[_index].size = coord.size();
		}*/
	}

	void ListCtrl::setIndexSelected(size_t _index)
	{
		MYGUI_ASSERT_RANGE_AND_NONE(_index, mItemsInfo.size(), "ListCtrl::setIndexSelected");
		if (_index == mIndexSelect) return;

		//size_t start = (size_t)mFirstVisibleIndex;

		// сбрасываем старое выделение
		if (mIndexSelect != ITEM_NONE)
		{

			size_t index = mIndexSelect;
			mIndexSelect = ITEM_NONE;

			//FIXME потом только один попробовать обновить
			_updateAllVisible(index, true, false);

			/*if ((index >= start) && (index < (start + mVectorItems.size())))
			{
				IBDrawItemInfo data(index, mIndexSelect, mIndexActive, mIndexAccept, mIndexRefuse, false, false);
				IntCoord coord(IntPoint(), mItemsInfo[index].size);
				requestDrawItem(this, mVectorItems[index - start], data, coord);
				mItemsInfo[index].size = coord.size();
			}*/
		}

		mIndexSelect = _index;
		if (mIndexSelect != ITEM_NONE)
		{

			//FIXME потом только один попробовать обновить
			_updateAllVisible(_index, true, false);

			/*if ((_index >= start) && (_index < (start + mVectorItems.size())))
			{
				IBDrawItemInfo data(_index, mIndexSelect, mIndexActive, mIndexAccept, mIndexRefuse, false, false);
				IntCoord coord(IntPoint(), mItemsInfo[_index].size);
				requestDrawItem(this, mVectorItems[_index - start], data, coord);
				mItemsInfo[_index].size = coord.size();
			}*/
		}

	}

	void ListCtrl::notifyMouseButtonDoubleClick(Widget* _sender)
	{
		size_t index = getIndexByWidget(_sender);

		eventSelectItemAccept(this, index);
	}

	void ListCtrl::notifyKeyButtonPressed(Widget* _sender, KeyCode _key, Char _char)
	{
		eventNotifyItem(this, IBNotifyItemData(getIndexByWidget(_sender), IBNotifyItemData::KeyPressed, _key, _char));
	}

	void ListCtrl::notifyKeyButtonReleased(Widget* _sender, KeyCode _key)
	{
		eventNotifyItem(this, IBNotifyItemData(getIndexByWidget(_sender), IBNotifyItemData::KeyReleased, _key));
	}

	size_t ListCtrl::getIndexByWidget(Widget* _widget)
	{
		MYGUI_ASSERT(_widget, "ListCtrl::getIndexByWidget : Widget == nullptr");
		if (_widget == _getClientWidget()) return ITEM_NONE;
		MYGUI_ASSERT(_widget->getParent() == _getClientWidget(), "ListCtrl::getIndexByWidget : Widget is not child");

		size_t index = calcIndexByWidget(_widget);
		MYGUI_ASSERT_RANGE(index, mItemsInfo.size(), "ListCtrl::getIndexByWidget");

		return index;
	}

	size_t ListCtrl::_getContainerIndex(const IntPoint& _point)
	{
		for (VectorWidgetPtr::iterator iter=mVectorItems.begin(); iter!=mVectorItems.end(); ++iter)
		{
			if ((*iter)->isVisible())
			{
				if ((*iter)->getAbsoluteRect().inside(_point))
				{
					return getIndexByWidget(*iter);
				}
			}
		}
		return ITEM_NONE;
	}

	void ListCtrl::_resetContainer(bool _update)
	{
		// обязательно у базового
		Base::_resetContainer(_update);

		if ( ! _update)
		{
			WidgetManager& instance = WidgetManager::getInstance();
			for (VectorWidgetPtr::iterator iter=mVectorItems.begin(); iter!=mVectorItems.end(); ++iter)
			{
				instance.unlinkFromUnlinkers(*iter);
			}
		}
	}

	Widget* ListCtrl::getWidgetByIndex(size_t _index)
	{
		for (VectorWidgetPtr::iterator iter=mVectorItems.begin(); iter!=mVectorItems.end(); ++iter)
		{
			if ((*iter)->isVisible())
			{
				size_t index = getIndexByWidget(*iter);

				if (index == _index) return (*iter);
			}
		}
		return nullptr;
	}

	void ListCtrl::onMouseButtonPressed(int _left, int _top, MouseButton _id)
	{
		Base::onMouseButtonPressed(_left, _top, _id);
	}

	void ListCtrl::onMouseButtonReleased(int _left, int _top, MouseButton _id)
	{
		Base::onMouseButtonReleased(_left, _top, _id);
	}

	void ListCtrl::onMouseDrag(int _left, int _top)
	{
		Base::onMouseDrag(_left, _top);
	}

	void ListCtrl::removeDropItems()
	{
		if (mItemDrag) mItemDrag->setVisible(false);
	}

	void ListCtrl::updateDropItems()
	{
		if (nullptr == mItemDrag)
		{
			// спрашиваем размер иконок
			IntCoord coord(0, 0, 50, 50);

			//requestCoordItem(this, coord, true);

			mPointDragOffset = coord.point();

			// создаем и запрашиваем детей
			mItemDrag = Gui::getInstance().createWidget<Widget>("Default", IntCoord(0, 0, coord.width, coord.height), Align::Default, mDragLayer);
			requestCreateWidgetItem(this, mItemDrag);
		}

		const IntPoint& point = InputManager::getInstance().getMousePositionByLayer();

		mItemDrag->setPosition(point.left - mClickInWidget.left + mPointDragOffset.left, point.top - mClickInWidget.top + mPointDragOffset.top);
		mItemDrag->setVisible(true);
	}

	void ListCtrl::updateDropItemsState(const DDWidgetState& _state)
	{
		IBDrawItemInfo data;
		data.drop_accept = _state.accept;
		data.drop_refuse = _state.refuse;

		data.select = false;
		data.active = false;

		data.index = mDropSenderIndex;
		data.update = _state.update;
		data.drag = true;

		IntCoord coord;

		requestDrawItem(this, mItemDrag, data, coord);

	}

	void ListCtrl::notifyMouseDrag(Widget* _sender, int _left, int _top)
	{
		mouseDrag();
	}

	void ListCtrl::notifyMouseButtonPressed(Widget* _sender, int _left, int _top, MouseButton _id)
	{
		mouseButtonPressed(_id);

		if ( MouseButton::Left == _id)
		{
			size_t old = mIndexSelect;

			if (_sender == _getClientWidget())
			{
				// сбрасываем выделение
				setIndexSelected(ITEM_NONE);
			}
			else
			{
				// индекс отправителя
				mDropSenderIndex = getIndexByWidget(_sender);

				// выделенный елемент
				setIndexSelected(mDropSenderIndex);
			}

			// смещение внутри виджета, куда кликнули мышкой
			mClickInWidget = InputManager::getInstance().getLastLeftPressed() - _sender->getAbsolutePosition();

			// отсылаем событие
			eventMouseItemActivate(this, mIndexSelect);
			// смену позиции отсылаем только при реальном изменении
			if (old != mIndexSelect) eventChangeItemPosition(this, mIndexSelect);
		}

		eventNotifyItem(this, IBNotifyItemData(getIndexByWidget(_sender), IBNotifyItemData::MousePressed, _left, _top, _id));
	}

	void ListCtrl::notifyMouseButtonReleased(Widget* _sender, int _left, int _top, MouseButton _id)
	{
		mouseButtonReleased(_id);
		eventNotifyItem(this, IBNotifyItemData(getIndexByWidget(_sender), IBNotifyItemData::MouseReleased, _left, _top, _id));
	}

	void ListCtrl::notifyRootMouseChangeFocus(Widget* _sender, bool _focus)
	{
		size_t index = calcIndexByWidget(_sender);
		if (_focus)
		{
			MYGUI_ASSERT_RANGE(index, mItemsInfo.size(), "ListCtrl::notifyRootMouseChangeFocus");

			// сбрасываем старый
			if (mIndexActive != ITEM_NONE)
			{
				size_t old_index = mIndexActive;
				mIndexActive = ITEM_NONE;

				//FIXME потом только один попробовать обновить
				_updateAllVisible(old_index, true, false);

				/*IBDrawItemInfo data(old_index, mIndexSelect, mIndexActive, mIndexAccept, mIndexRefuse, false, false);
				IntCoord coord(IntPoint(), mItemsInfo[old_index].size);
				requestDrawItem(this, mVectorItems[old_index - mFirstVisibleIndex], data, coord);
				mItemsInfo[old_index].size = coord.size();*/

			}

			mIndexActive = index;

			//FIXME потом только один попробовать обновить
			_updateAllVisible(index, true, false);

			/*IBDrawItemInfo data(index, mIndexSelect, mIndexActive, mIndexAccept, mIndexRefuse, false, false);
			IntCoord coord(IntPoint(), mItemsInfo[index].size);
			requestDrawItem(this, mVectorItems[*_sender->_getInternalData<size_t>()], data, coord);
			mItemsInfo[index].size = coord.size();*/

		}
		else
		{
			// при сбросе виджет может быть уже скрыт, и соответсвенно отсутсвовать индекс
			// сбрасываем индекс, только если мы и есть актив
			if (index < mItemsInfo.size() && mIndexActive == index)
			{
				mIndexActive = ITEM_NONE;

				//FIXME потом только один попробовать обновить
				_updateAllVisible(index, true, false);

				/*IBDrawItemInfo data(index, mIndexSelect, mIndexActive, mIndexAccept, mIndexRefuse, false, false);
				IntCoord coord(IntPoint(), mItemsInfo[index].size);
				requestDrawItem(this, mVectorItems[*_sender->_getInternalData<size_t>()], data, coord);
				mItemsInfo[index].size = coord.size();*/

			}
		}
	}

	void ListCtrl::updateMetrics()
	{
		IntSize size;

		for (VectorItemInfo::const_iterator item=mItemsInfo.begin(); item!=mItemsInfo.end(); ++item)
		{
			if (size.width < item->size.width)
				size.width = item->size.width;
			size.height += item->size.height;
		}

		mContentSize = size;
	}

	void ListCtrl::notifyScrollChangePosition(VScroll* _sender, size_t _index)
	{
		if (_sender == mVScroll)
		{
			mContentPosition.top = (int)_index;
		}
		else if (_sender == mHScroll)
		{
			mContentPosition.left = (int)_index;
		}

		setContentPosition(mContentPosition);
	}

	void ListCtrl::setContentPosition(const IntPoint& _point)
	{
		mContentPosition = _point;

		_updateAllVisible(ITEM_NONE, true, true);
		_resetContainer(true);
	}

	void ListCtrl::notifyMouseWheel(Widget* _sender, int _rel)
	{
		if (mContentSize.height <= 0) return;

		int offset = mContentPosition.top;
		if (_rel < 0) offset += mScrollViewPage;
		else offset -= mScrollViewPage;

		if (mContentSize.height <= _getClientWidget()->getHeight()) return;

		if (offset >= mContentSize.height - _getClientWidget()->getHeight()) offset = mContentSize.height - _getClientWidget()->getHeight();
		else if (offset < 0) offset = 0;

		if (mContentPosition.top == offset) return;

		// сбрасываем старую подсветку
		// так как при прокрутке, мышь может находиться над окном
		resetCurrentActiveItem();

		mContentPosition.top = offset;

		setContentPosition(mContentPosition);

		// заново ищем и подсвечиваем айтем
		if (!mNeedDrop)
			findCurrentActiveItem();

		if (nullptr != mVScroll) mVScroll->setScrollPosition(mContentPosition.top);
		if (nullptr != mHScroll) mHScroll->setScrollPosition(mContentPosition.left);
	}

	void ListCtrl::resetDrag()
	{
		endDrop(true);
	}

	IntSize ListCtrl::getContentSize()
	{
		return mContentSize;
	}

	IntPoint ListCtrl::getContentPosition()
	{
		return mContentPosition;
	}

	IntSize ListCtrl::getViewSize() const
	{
		return _getClientWidget()->getSize();
	}

	void ListCtrl::eraseContent()
	{
		_updateAllVisible(ITEM_NONE, false, true);
		updateMetrics();
	}

	Align ListCtrl::getContentAlign()
	{
		return Align::Default;
	}

	Widget* ListCtrl::_getClientWidget()
	{
		return mWidgetClient == nullptr ? this : mWidgetClient;
	}

	const Widget* ListCtrl::_getClientWidget() const
	{
		return mWidgetClient == nullptr ? this : mWidgetClient;
	}

} // namespace MyGUI
