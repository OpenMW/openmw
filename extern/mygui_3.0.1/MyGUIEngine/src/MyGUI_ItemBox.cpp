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
#include "MyGUI_ItemBox.h"
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

	ItemBox::ItemBox() :
		mCountItemInLine(0),
		mCountLines(0),
		mFirstVisibleIndex(0),
		mFirstOffsetIndex(0),
		mIndexSelect(ITEM_NONE),
		mIndexActive(ITEM_NONE),
		mIndexAccept(ITEM_NONE),
		mIndexRefuse(ITEM_NONE),
		mIsFocus(false),
		mItemDrag(nullptr),
		mAlignVert(true)
	{
		mChangeContentByResize = true;
	}

	void ItemBox::_initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name)
	{
		Base::_initialise(_style, _coord, _align, _info, _parent, _croppedParent, _creator, _name);

		initialiseWidgetSkin(_info);
	}

	ItemBox::~ItemBox()
	{
		shutdownWidgetSkin();
	}

	void ItemBox::baseChangeWidgetSkin(ResourceSkin* _info)
	{
		shutdownWidgetSkin();
		Base::baseChangeWidgetSkin(_info);
		initialiseWidgetSkin(_info);
	}

	void ItemBox::initialiseWidgetSkin(ResourceSkin* _info)
	{
		// нам нужен фокус клавы
		mNeedKeyFocus = true;
		mDragLayer = "DragAndDrop";

		const MapString& properties = _info->getProperties();
		if (!properties.empty())
		{
			MapString::const_iterator iter = properties.find("AlignVert");
			if (iter != properties.end()) mAlignVert = utility::parseBool(iter->second);
			iter = properties.find("DragLayer");
			if (iter != properties.end()) mDragLayer = iter->second;
		}

		for (VectorWidgetPtr::iterator iter=mWidgetChildSkin.begin(); iter!=mWidgetChildSkin.end(); ++iter)
		{
			if (*(*iter)->_getInternalData<std::string>() == "VScroll")
			{
				MYGUI_DEBUG_ASSERT( ! mVScroll, "widget already assigned");
				mVScroll = (*iter)->castType<VScroll>();
				mVScroll->eventScrollChangePosition = newDelegate(this, &ItemBox::notifyScrollChangePosition);
			}
			if (*(*iter)->_getInternalData<std::string>() == "HScroll")
			{
				MYGUI_DEBUG_ASSERT( ! mHScroll, "widget already assigned");
				mHScroll = (*iter)->castType<HScroll>();
				mHScroll->eventScrollChangePosition = newDelegate(this, &ItemBox::notifyScrollChangePosition);
			}
			else if (*(*iter)->_getInternalData<std::string>() == "Client")
			{
				MYGUI_DEBUG_ASSERT( ! mWidgetClient, "widget already assigned");
				mWidgetClient = (*iter);
				mWidgetClient->eventMouseWheel = newDelegate(this, &ItemBox::notifyMouseWheel);
				mWidgetClient->eventMouseButtonPressed = newDelegate(this, &ItemBox::notifyMouseButtonPressed);
				mClient = mWidgetClient;
			}
		}
		// сли нет скрола, то клиенская зона не обязательно
		//MYGUI_ASSERT(nullptr != mWidgetClient, "Child Widget Client not found in skin (ItemBox must have Client) skin ='" << _info->getSkinName() << "'");

		// подписываем клиент для драгэндропа
		if (mWidgetClient != nullptr)
			mWidgetClient->_requestGetContainer = newDelegate(this, &ItemBox::_requestGetContainer);

		requestItemSize();

		updateScrollSize();
		updateScrollPosition();
	}

	void ItemBox::shutdownWidgetSkin()
	{
		mVScroll = nullptr;
		mHScroll = nullptr;
		mClient = nullptr;
		mWidgetClient = nullptr;
	}

	void ItemBox::setPosition(const IntPoint& _point)
	{
		Base::setPosition(_point);
	}

	void ItemBox::setSize(const IntSize& _size)
	{
		Base::setSize(_size);
		updateFromResize();
	}

	void ItemBox::setCoord(const IntCoord& _coord)
	{
		Base::setCoord(_coord);
		updateFromResize();
	}

	void ItemBox::requestItemSize()
	{
		IntCoord coord(0, 0, 1, 1);

		// спрашиваем размер иконок
		requestCoordItem(this, coord, false);

		mSizeItem = coord.size();
		MYGUI_ASSERT((mSizeItem.width > 0 && mSizeItem.height > 0), "(mSizeItem.width > 0 && mSizeItem.height > 0)  at requestCoordWidgetItem");
	}

	void ItemBox::updateFromResize()
	{
		requestItemSize();

		updateScrollSize();
		updateScrollPosition();

		_updateAllVisible(true);
		_resetContainer(true);
	}

	void ItemBox::_updateAllVisible(bool _redraw)
	{
		int count_visible = 0;
		if (mAlignVert)
		{
			count_visible = (_getClientWidget()->getHeight() / mSizeItem.height) + 2;
		}
		else
		{
			count_visible = (_getClientWidget()->getWidth() / mSizeItem.width) + 2;
		}

		size_t start = (mFirstVisibleIndex * mCountItemInLine);
		size_t count = (count_visible * mCountItemInLine) + start;

		size_t index = 0;
		for (size_t pos = start; pos<count; ++pos, ++index)
		{
			// дальше нет айтемов
			if (pos >= mItemsInfo.size()) break;

			Widget* item = getItemWidget(index);
			if (mAlignVert)
			{
				item->setPosition(((int)index % mCountItemInLine) * mSizeItem.width - mContentPosition.left,
					(((int)index / mCountItemInLine) * mSizeItem.height)  - mFirstOffsetIndex);
			}
			else
			{
				item->setPosition((((int)index / mCountItemInLine) * mSizeItem.width)  - mFirstOffsetIndex,
					((int)index % mCountItemInLine) * mSizeItem.height - mContentPosition.top);
			}

			item->setSize(mSizeItem);
			item->setVisible(true);

			if (_redraw)
			{
				IBDrawItemInfo data(pos, mIndexSelect, mIndexActive, mIndexAccept, mIndexRefuse, true, false);
				requestDrawItem(this, item, data);
			}

		}

		// все виджеты еще есть, то их надо бы скрыть
		while (index < mVectorItems.size())
		{
			mVectorItems[index]->setVisible(false);
			index ++;
		}

	}

	Widget* ItemBox::getItemWidget(size_t _index)
	{
		// еще нет такого виджета, нуно создать
		if (_index == mVectorItems.size())
		{

			requestItemSize();

			Widget* item = _getClientWidget()->createWidget<Widget>("Default", IntCoord(0, 0, mSizeItem.width, mSizeItem.height), Align::Default);

			// вызываем запрос на создание виджета
			requestCreateWidgetItem(this, item);

			item->eventMouseWheel = newDelegate(this, &ItemBox::notifyMouseWheel);
			item->eventRootMouseChangeFocus = newDelegate(this, &ItemBox::notifyRootMouseChangeFocus);
			item->eventMouseButtonPressed = newDelegate(this, &ItemBox::notifyMouseButtonPressed);
			item->eventMouseButtonReleased = newDelegate(this, &ItemBox::notifyMouseButtonReleased);
			item->eventMouseButtonDoubleClick = newDelegate(this, &ItemBox::notifyMouseButtonDoubleClick);
			item->eventMouseDrag = newDelegate(this, &ItemBox::notifyMouseDrag);
			item->_requestGetContainer = newDelegate(this, &ItemBox::_requestGetContainer);
			item->eventKeyButtonPressed = newDelegate(this, &ItemBox::notifyKeyButtonPressed);
			item->eventKeyButtonReleased = newDelegate(this, &ItemBox::notifyKeyButtonReleased);

			item->_setInternalData((size_t)mVectorItems.size());

			mVectorItems.push_back(item);
		}

		// запрашивать только последовательно
		MYGUI_ASSERT_RANGE(_index, mVectorItems.size(), "ItemBox::getItemWidget");

		return mVectorItems[_index];
	}

	void ItemBox::onMouseWheel(int _rel)
	{
		notifyMouseWheel(nullptr, _rel);

		Base::onMouseWheel(_rel);
	}

	void ItemBox::onKeySetFocus(Widget* _old)
	{
		mIsFocus = true;
		setState("pushed");

		Base::onKeySetFocus(_old);
	}

	void ItemBox::onKeyLostFocus(Widget* _new)
	{
		mIsFocus = false;
		setState("normal");

		Base::onKeyLostFocus(_new);
	}

	void ItemBox::resetCurrentActiveItem()
	{
		// сбрасываем старую подсветку
		if (mIndexActive != ITEM_NONE)
		{
			size_t start = (size_t)(mFirstVisibleIndex * mCountItemInLine);
			size_t index = mIndexActive;
			mIndexActive = ITEM_NONE;

			// если видим, то обновляем
			if ((mIndexActive >= start) && (mIndexActive < (start + mVectorItems.size())))
			{
				IBDrawItemInfo data(index, mIndexSelect, mIndexActive, mIndexAccept, mIndexRefuse, false, false);

				requestDrawItem(this, mVectorItems[mIndexActive - start], data);
			}
		}
	}

	void ItemBox::findCurrentActiveItem()
	{
		MYGUI_DEBUG_ASSERT(mIndexActive == ITEM_NONE, "use : resetCurrentActiveItem() before findCurrentActiveItem()");

		const IntPoint& point = InputManager::getInstance().getMousePositionByLayer();

		// сначала проверяем клиентскую зону
		const IntRect& rect = _getClientAbsoluteRect();
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
					IBDrawItemInfo data(index, mIndexSelect, mIndexActive, mIndexAccept, mIndexRefuse, false, false);

					requestDrawItem(this, item, data);
				}

				break;
			}
		}
	}

	void ItemBox::_requestGetContainer(Widget* _sender, Widget*& _container, size_t& _index)
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

	void ItemBox::_setContainerItemInfo(size_t _index, bool _set, bool _accept)
	{
		if (_index == ITEM_NONE) return;
		MYGUI_ASSERT_RANGE(_index, mItemsInfo.size(), "ItemBox::_setContainerItemInfo");

		mIndexAccept = (_set && _accept ) ? _index : ITEM_NONE;
		mIndexRefuse = (_set && !_accept) ? _index : ITEM_NONE;

		size_t start = (size_t)(mFirstVisibleIndex * mCountItemInLine);
		if ((_index >= start) && (_index < (start + mVectorItems.size())))
		{
			IBDrawItemInfo data(_index, mIndexSelect, mIndexActive, mIndexAccept, mIndexRefuse, false, false);
			requestDrawItem(this, mVectorItems[_index - start], data);
		}
	}

	void ItemBox::setItemDataAt(size_t _index, Any _data)
	{
		MYGUI_ASSERT_RANGE(_index, mItemsInfo.size(), "ItemBox::setItemData");
		mItemsInfo[_index].data = _data;

		size_t start = (size_t)(mFirstVisibleIndex * mCountItemInLine);
		if ((_index >= start) && (_index < (start + mVectorItems.size())))
		{
			IBDrawItemInfo data(_index, mIndexSelect, mIndexActive, mIndexAccept, mIndexRefuse, true, false);
			requestDrawItem(this, mVectorItems[_index - start], data);
		}

		_resetContainer(true);
	}

	void ItemBox::insertItemAt(size_t _index, Any _data)
	{
		MYGUI_ASSERT_RANGE_INSERT(_index, mItemsInfo.size(), "ItemBox::insertItemAt");
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

		_updateAllVisible(true);
	}

	void ItemBox::removeItemAt(size_t _index)
	{
		MYGUI_ASSERT_RANGE(_index, mItemsInfo.size(), "ItemBox::removeItemAt");

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

		_updateAllVisible(true);
	}

	void ItemBox::removeAllItems()
	{
		if (0 == mItemsInfo.size()) return;
		_resetContainer(false);

		mItemsInfo.clear();

		mIndexSelect = ITEM_NONE;
		mIndexActive = ITEM_NONE;

		updateScrollSize();
		updateScrollPosition();

		_updateAllVisible(true);
	}

	void ItemBox::redrawItemAt(size_t _index)
	{
		MYGUI_ASSERT_RANGE(_index, mItemsInfo.size(), "ItemBox::redrawItemAt");

		size_t start = (size_t)(mFirstVisibleIndex * mCountItemInLine);
		if ((_index >= start) && (_index < (start + mVectorItems.size())))
		{
			IBDrawItemInfo data(_index, mIndexSelect, mIndexActive, mIndexAccept, mIndexRefuse, true, false);
			requestDrawItem(this, mVectorItems[_index - start], data);
		}
	}

	void ItemBox::setIndexSelected(size_t _index)
	{
		MYGUI_ASSERT_RANGE_AND_NONE(_index, mItemsInfo.size(), "ItemBox::setIndexSelected");
		if (_index == mIndexSelect) return;

		size_t start = (size_t)(mFirstVisibleIndex * mCountItemInLine);

		// сбрасываем старое выделение
		if (mIndexSelect != ITEM_NONE)
		{
			size_t index = mIndexSelect;
			mIndexSelect = ITEM_NONE;

			if ((index >= start) && (index < (start + mVectorItems.size())))
			{
				IBDrawItemInfo data(index, mIndexSelect, mIndexActive, mIndexAccept, mIndexRefuse, false, false);
				requestDrawItem(this, mVectorItems[index - start], data);
			}
		}

		mIndexSelect = _index;
		if (mIndexSelect != ITEM_NONE)
		{
			if ((_index >= start) && (_index < (start + mVectorItems.size())))
			{
				IBDrawItemInfo data(_index, mIndexSelect, mIndexActive, mIndexAccept, mIndexRefuse, false, false);
				requestDrawItem(this, mVectorItems[_index - start], data);
			}
		}

	}

	void ItemBox::notifyMouseButtonDoubleClick(Widget* _sender)
	{
		size_t index = getIndexByWidget(_sender);

		eventSelectItemAccept(this, index);
	}

	void ItemBox::setItemBoxAlignVert(bool _vert)
	{
		if (mAlignVert == _vert) return;
		mAlignVert = _vert;

		mCountItemInLine = -1;
		updateFromResize();
	}

	void ItemBox::notifyKeyButtonPressed(Widget* _sender, KeyCode _key, Char _char)
	{
		eventNotifyItem(this, IBNotifyItemData(getIndexByWidget(_sender), IBNotifyItemData::KeyPressed, _key, _char));
	}

	void ItemBox::notifyKeyButtonReleased(Widget* _sender, KeyCode _key)
	{
		eventNotifyItem(this, IBNotifyItemData(getIndexByWidget(_sender), IBNotifyItemData::KeyReleased, _key));
	}

	size_t ItemBox::getIndexByWidget(Widget* _widget)
	{
		MYGUI_ASSERT(_widget, "ItemBox::getIndexByWidget : Widget == nullptr");
		if (_widget == _getClientWidget()) return ITEM_NONE;
		MYGUI_ASSERT(_widget->getParent() == _getClientWidget(), "ItemBox::getIndexByWidget : Widget is not child");

		size_t index = calcIndexByWidget(_widget);
		MYGUI_ASSERT_RANGE(index, mItemsInfo.size(), "ItemBox::getIndexByWidget");

		return index;
	}

	size_t ItemBox::_getContainerIndex(const IntPoint& _point)
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

	void ItemBox::_resetContainer(bool _update)
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

	Widget* ItemBox::getWidgetByIndex(size_t _index)
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

	void ItemBox::onMouseButtonPressed(int _left, int _top, MouseButton _id)
	{
		Base::onMouseButtonPressed(_left, _top, _id);
	}

	void ItemBox::onMouseButtonReleased(int _left, int _top, MouseButton _id)
	{
		Base::onMouseButtonReleased(_left, _top, _id);
	}

	void ItemBox::onMouseDrag(int _left, int _top)
	{
		Base::onMouseDrag(_left, _top);
	}

	void ItemBox::removeDropItems()
	{
		if (mItemDrag) mItemDrag->setVisible(false);
	}

	void ItemBox::updateDropItems()
	{
		if (nullptr == mItemDrag)
		{
			// спрашиваем размер иконок
			IntCoord coord;

			requestCoordItem(this, coord, true);

			mPointDragOffset = coord.point();

			// создаем и запрашиваем детей
			mItemDrag = Gui::getInstance().createWidget<Widget>("Default", IntCoord(0, 0, coord.width, coord.height), Align::Default, mDragLayer);
			requestCreateWidgetItem(this, mItemDrag);
		}

		const IntPoint& point = InputManager::getInstance().getMousePosition();

		mItemDrag->setPosition(point.left - mClickInWidget.left + mPointDragOffset.left, point.top - mClickInWidget.top + mPointDragOffset.top);
		mItemDrag->setVisible(true);
	}

	void ItemBox::updateDropItemsState(const DDWidgetState& _state)
	{
		IBDrawItemInfo data;
		data.drop_accept = _state.accept;
		data.drop_refuse = _state.refuse;

		data.select = false;
		data.active = false;

		data.index = mDropSenderIndex;
		data.update = _state.update;
		data.drag = true;

		requestDrawItem(this, mItemDrag, data);
	}

	void ItemBox::notifyMouseDrag(Widget* _sender, int _left, int _top)
	{
		mouseDrag();
	}

	void ItemBox::notifyMouseButtonPressed(Widget* _sender, int _left, int _top, MouseButton _id)
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

	void ItemBox::notifyMouseButtonReleased(Widget* _sender, int _left, int _top, MouseButton _id)
	{
		mouseButtonReleased(_id);
		size_t index = calcIndexByWidget(_sender);
		// солличество айтемов может измениться
		if (index >= getItemCount()) return;
		eventNotifyItem(this, IBNotifyItemData(index, IBNotifyItemData::MouseReleased, _left, _top, _id));
	}

	void ItemBox::notifyRootMouseChangeFocus(Widget* _sender, bool _focus)
	{
		size_t index = calcIndexByWidget(_sender);
		if (_focus)
		{
			MYGUI_ASSERT_RANGE(index, mItemsInfo.size(), "ItemBox::notifyRootMouseChangeFocus");

			// сбрасываем старый
			if (mIndexActive != ITEM_NONE)
			{
				size_t old_index = mIndexActive;
				mIndexActive = ITEM_NONE;
				IBDrawItemInfo data(old_index, mIndexSelect, mIndexActive, mIndexAccept, mIndexRefuse, false, false);
				requestDrawItem(this, mVectorItems[old_index - (mFirstVisibleIndex * mCountItemInLine)], data);
			}

			mIndexActive = index;
			IBDrawItemInfo data(index, mIndexSelect, mIndexActive, mIndexAccept, mIndexRefuse, false, false);
			requestDrawItem(this, mVectorItems[*_sender->_getInternalData<size_t>()], data);
		}
		else
		{
			// при сбросе виджет может быть уже скрыт, и соответсвенно отсутсвовать индекс
			// сбрасываем индекс, только если мы и есть актив
			if (index < mItemsInfo.size() && mIndexActive == index)
			{
				mIndexActive = ITEM_NONE;
				IBDrawItemInfo data(index, mIndexSelect, mIndexActive, mIndexAccept, mIndexRefuse, false, false);
				requestDrawItem(this, mVectorItems[*_sender->_getInternalData<size_t>()], data);
			}
		}
	}

	void ItemBox::updateMetrics()
	{
		if (mAlignVert)
		{
			// колличество айтемов на одной строке
			mCountItemInLine = _getClientWidget()->getWidth() / mSizeItem.width;
		}
		else
		{
			// колличество айтемов на одной строке
			mCountItemInLine = _getClientWidget()->getHeight() / mSizeItem.height;
		}

		if (1 > mCountItemInLine) mCountItemInLine = 1;

		// колличество строк
		mCountLines = mItemsInfo.size() / mCountItemInLine;
		if (0 != (mItemsInfo.size() % mCountItemInLine)) mCountLines ++;

		if (mAlignVert)
		{
			mContentSize.width = (mSizeItem.width * mCountItemInLine);
			mContentSize.height = (mSizeItem.height * mCountLines);
		}
		else
		{
			mContentSize.width = (mSizeItem.width * mCountLines);
			mContentSize.height = (mSizeItem.height * mCountItemInLine);
		}
	}

	void ItemBox::notifyScrollChangePosition(VScroll* _sender, size_t _index)
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

	void ItemBox::notifyMouseWheel(Widget* _sender, int _rel)
	{
		if (mAlignVert)
		{
			if (mContentSize.height <= 0) return;

			int offset = mContentPosition.top;
			if (_rel < 0) offset += mSizeItem.height;
			else offset -= mSizeItem.height;

			if (offset >= mContentSize.height - _getClientWidget()->getHeight()) offset = mContentSize.height - _getClientWidget()->getHeight();
			else if (offset < 0) offset = 0;

			if (mContentPosition.top == offset) return;

			// сбрасываем старую подсветку
			// так как при прокрутке, мышь может находиться над окном
			resetCurrentActiveItem();

			mContentPosition.top = offset;
		}
		else
		{
			if (mContentSize.width <= 0) return;

			int offset = mContentPosition.left;
			if (_rel < 0) offset += mSizeItem.width;
			else  offset -= mSizeItem.width;

			if (offset >= mContentSize.width - _getClientWidget()->getWidth()) offset = mContentSize.width - _getClientWidget()->getWidth();
			else if (offset < 0) offset = 0;

			if (mContentPosition.left == offset) return;

			// сбрасываем старую подсветку
			// так как при прокрутке, мышь может находиться над окном
			resetCurrentActiveItem();

			mContentPosition.left = offset;
		}

		setContentPosition(mContentPosition);

		// заново ищем и подсвечиваем айтем
		if (!mNeedDrop)
			findCurrentActiveItem();

		if (nullptr != mVScroll) mVScroll->setScrollPosition(mContentPosition.top);
		if (nullptr != mHScroll) mHScroll->setScrollPosition(mContentPosition.left);
	}

	void ItemBox::setContentPosition(const IntPoint& _point)
	{
		mContentPosition = _point;

		int old = mFirstVisibleIndex;

		if (mAlignVert)
		{
			mFirstVisibleIndex = mContentPosition.top / mSizeItem.height;
			mFirstOffsetIndex = mContentPosition.top % mSizeItem.height;
		}
		else
		{
			mFirstVisibleIndex = mContentPosition.left / mSizeItem.width;
			mFirstOffsetIndex = mContentPosition.left % mSizeItem.width;
		}

		_updateAllVisible(old != mFirstVisibleIndex);
		_resetContainer(true);
	}

	void ItemBox::redrawAllItems()
	{
		_updateAllVisible(true);
	}

	void ItemBox::resetDrag()
	{
		endDrop(true);
	}

	size_t ItemBox::calcIndexByWidget(Widget* _widget)
	{
		return *_widget->_getInternalData<size_t>() + (mFirstVisibleIndex * mCountItemInLine);
	}

	IntSize ItemBox::getContentSize()
	{
		return mContentSize;
	}

	IntPoint ItemBox::getContentPosition()
	{
		return mContentPosition;
	}

	IntSize ItemBox::getViewSize() const
	{
		return _getClientWidget()->getSize();
	}

	void ItemBox::eraseContent()
	{
		updateMetrics();
	}

	size_t ItemBox::getHScrollPage()
	{
		return mSizeItem.width;
	}

	size_t ItemBox::getVScrollPage()
	{
		return mSizeItem.height;
	}

	Align ItemBox::getContentAlign()
	{
		return Align::Default;
	}

	IntRect ItemBox::_getClientAbsoluteRect()
	{
		return _getClientWidget()->getAbsoluteRect();
	}

	Widget* ItemBox::_getClientWidget()
	{
		return mWidgetClient == nullptr ? this : mWidgetClient;
	}

	const Widget* ItemBox::_getClientWidget() const
	{
		return mWidgetClient == nullptr ? this : mWidgetClient;
	}

} // namespace MyGUI
