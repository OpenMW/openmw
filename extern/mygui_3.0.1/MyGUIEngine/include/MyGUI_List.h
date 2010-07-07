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
#ifndef __MYGUI_LIST_H__
#define __MYGUI_LIST_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Widget.h"
#include "MyGUI_Any.h"
#include "MyGUI_EventPair.h"

namespace MyGUI
{

	typedef delegates::CDelegate2<List*, size_t> EventHandle_ListPtrSizeT;

	class MYGUI_EXPORT List :
		public Widget
	{
		MYGUI_RTTI_DERIVED( List )

	public:
		List();

		//------------------------------------------------------------------------------//
		// манипуляции айтемами

		//! Get number of items
		size_t getItemCount() const { return mItemsInfo.size(); }

		//! Insert an item into a array at a specified position
		void insertItemAt(size_t _index, const UString& _name, Any _data = Any::Null);

		//! Add an item to the end of a array
		void addItem(const UString& _name, Any _data = Any::Null) { insertItemAt(ITEM_NONE, _name, _data); }

		//! Remove item at a specified position
		void removeItemAt(size_t _index);

		//! Remove all items
		void removeAllItems();

		//! Swap items at a specified positions
		void swapItemsAt(size_t _index1, size_t _index2);


		//! Search item, returns the position of the first occurrence in array or ITEM_NONE if item not found
		size_t findItemIndexWith(const UString& _name);


		//------------------------------------------------------------------------------//
		// манипуляции выделениями

		/** Get index of selected item (ITEM_NONE if none selected) */
		size_t getIndexSelected() { return mIndexSelect; }

		/** Select specified _index */
		void setIndexSelected(size_t _index);

		/** Clear item selection */
		void clearIndexSelected() { setIndexSelected(ITEM_NONE); }


		//------------------------------------------------------------------------------//
		// манипуляции данными

		//! Replace an item data at a specified position
		void setItemDataAt(size_t _index, Any _data);

		//! Clear an item data at a specified position
		void clearItemDataAt(size_t _index) { setItemDataAt(_index, Any::Null); }

		//! Get item data from specified position
		template <typename ValueType>
		ValueType * getItemDataAt(size_t _index, bool _throw = true)
		{
			MYGUI_ASSERT_RANGE(_index, mItemsInfo.size(), "List::getItemDataAt");
			return mItemsInfo[_index].second.castType<ValueType>(_throw);
		}


		//------------------------------------------------------------------------------//
		// манипуляции отображением

		//! Replace an item name at a specified position
		void setItemNameAt(size_t _index, const UString& _name);

		//! Get item name from specified position
		const UString& getItemNameAt(size_t _index);


		//------------------------------------------------------------------------------//
		// манипуляции выдимостью

		//! Move all elements so specified becomes visible
		void beginToItemAt(size_t _index);

		//! Move all elements so first becomes visible
		void beginToItemFirst() { if (getItemCount()) beginToItemAt(0); }

		//! Move all elements so last becomes visible
		void beginToItemLast() { if (getItemCount()) beginToItemAt(getItemCount() - 1); }

		//! Move all elements so selected becomes visible
		void beginToItemSelected() { if (getIndexSelected() != ITEM_NONE) beginToItemAt(getIndexSelected()); }

		//------------------------------------------------------------------------------//

		// видим ли мы элемент, полностью или нет
		/** Return true if item visible
			@param
				_index of item
			@param
				_fill false: function return true when whole item is visible
				      true: function return true when at least part of item is visible
		*/
		bool isItemVisibleAt(size_t _index, bool _fill = true);
		//! Same as List::isItemVisible for selected item
		bool isItemSelectedVisible(bool _fill = true) { return isItemVisibleAt(mIndexSelect, _fill); }


		//! Set scroll visible when it needed
		void setScrollVisible(bool _visible);
		//! Set scroll position
		void setScrollPosition(size_t _position);

		//------------------------------------------------------------------------------------//

		//! @copydoc Widget::setPosition(const IntPoint& _value)
		virtual void setPosition(const IntPoint& _value);
		//! @copydoc Widget::setSize(const IntSize& _value)
		virtual void setSize(const IntSize& _value);
		//! @copydoc Widget::setCoord(const IntCoord& _value)
		virtual void setCoord(const IntCoord& _value);

		/** @copydoc Widget::setPosition(int _left, int _top) */
		void setPosition(int _left, int _top) { setPosition(IntPoint(_left, _top)); }
		/** @copydoc Widget::setSize(int _width, int _height) */
		void setSize(int _width, int _height) { setSize(IntSize(_width, _height)); }
		/** @copydoc Widget::setCoord(int _left, int _top, int _width, int _height) */
		void setCoord(int _left, int _top, int _width, int _height) { setCoord(IntCoord(_left, _top, _width, _height)); }

		// возвращает максимальную высоту вмещающую все строки и родительский бордюр
		//! Return optimal height to fit all items in List
		int getOptimalHeight();

		/** @copydoc Widget::setProperty(const std::string& _key, const std::string& _value) */
		virtual void setProperty(const std::string& _key, const std::string& _value);

	/*event:*/
		/** Event : Enter pressed or double click.\n
			signature : void method(MyGUI::List* _sender, size_t _index)\n
			@param _sender widget that called this event
			@param _index of selected item
		*/
		EventPair<EventHandle_WidgetSizeT, EventHandle_ListPtrSizeT> eventListSelectAccept;

		/** Event : Selected item position changed.\n
			signature : void method(MyGUI::List* _sender, size_t _index)\n
			@param _sender widget that called this event
			@param _index of new item
		*/
		EventPair<EventHandle_WidgetSizeT, EventHandle_ListPtrSizeT> eventListChangePosition;

		/** Event : Item was selected by mouse.\n
			signature : void method(MyGUI::List* _sender, size_t _index)\n
			@param _sender widget that called this event
			@param _index of selected item
		*/
		EventPair<EventHandle_WidgetSizeT, EventHandle_ListPtrSizeT> eventListMouseItemActivate;

		/** Event : Mouse is over item.\n
			signature : void method(MyGUI::List* _sender, size_t _index)\n
			@param _sender widget that called this event
			@param _index of focused item
		*/
		EventPair<EventHandle_WidgetSizeT, EventHandle_ListPtrSizeT> eventListMouseItemFocus;

		/** Event : Position of scroll changed.\n
			signature : void method(MyGUI::List* _sender, size_t _position)\n
			@param _sender widget that called this event
			@param _position of scroll
		*/
		EventPair<EventHandle_WidgetSizeT, EventHandle_ListPtrSizeT> eventListChangeScroll;

	/*internal:*/
		// дебажная проверка на правильность выравнивания списка
		void _checkAlign();

		// вспомогательные методы для составных списков
		void _setItemFocus(size_t _position, bool _focus);
		void _sendEventChangeScroll(size_t _position);

		virtual void _initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name);

	/*obsolete:*/
#ifndef MYGUI_DONT_USE_OBSOLETE

		MYGUI_OBSOLETE("use : void Widget::setCoord(const IntCoord& _coord)")
		void setPosition(const IntCoord& _coord) { setCoord(_coord); }
		MYGUI_OBSOLETE("use : void Widget::setCoord(int _left, int _top, int _width, int _height)")
		void setPosition(int _left, int _top, int _width, int _height) { setCoord(_left, _top, _width, _height); }

		MYGUI_OBSOLETE("use : size_t List::getIndexSelected()")
		size_t getItemIndexSelected() { return getIndexSelected(); }
		MYGUI_OBSOLETE("use : void List::setIndexSelected(size_t _index)")
		void setItemSelectedAt(size_t _index) { setIndexSelected(_index); }
		MYGUI_OBSOLETE("use : void List::clearIndexSelected()")
		void clearItemSelected() { clearIndexSelected(); }

		MYGUI_OBSOLETE("use : void List::insertItemAt(size_t _index, const UString& _name)")
		void insertItem(size_t _index, const UString& _item) { insertItemAt(_index, _item); }
		MYGUI_OBSOLETE("use : void List::setItemNameAt(size_t _index, const UString& _name)")
		void setItem(size_t _index, const UString& _item) { setItemNameAt(_index, _item); }
		MYGUI_OBSOLETE("use : const UString& List::getItemNameAt(size_t _index)")
		const UString& getItem(size_t _index) { return getItemNameAt(_index); }
		MYGUI_OBSOLETE("use : void List::removeItemAt(size_t _index)")
		void deleteItem(size_t _index) { removeItemAt(_index); }
		MYGUI_OBSOLETE("use : void List::removeAllItems()")
		void deleteAllItems() { removeAllItems(); }
		MYGUI_OBSOLETE("use : size_t List::findItemIndexWith(const UString& _name)")
		size_t findItem(const UString& _item) { return findItemIndexWith(_item); }
		MYGUI_OBSOLETE("use : size_t List::getIndexSelected()")
		size_t getItemSelect() { return getIndexSelected(); }
		MYGUI_OBSOLETE("use : void List::clearIndexSelected()")
		void resetItemSelect() { clearIndexSelected(); }
		MYGUI_OBSOLETE("use : void List::setIndexSelected(size_t _index)")
		void setItemSelect(size_t _index) { setIndexSelected(_index); }
		MYGUI_OBSOLETE("use : void List::beginToItemAt(size_t _index)")
		void beginToIndex(size_t _index) { beginToItemAt(_index); }
		MYGUI_OBSOLETE("use : void List::beginToItemFirst()")
		void beginToStart() { beginToItemFirst(); }
		MYGUI_OBSOLETE("use : void List::beginToItemLast()")
		void beginToEnd() { beginToItemLast(); }
		MYGUI_OBSOLETE("use : void List::beginToItemSelected()")
		void beginToSelect() { beginToItemSelected(); }
		MYGUI_OBSOLETE("use : bool List::isItemVisibleAt(size_t _index, bool _fill)")
		bool isItemVisible(size_t _index, bool _fill = true) { return isItemVisibleAt(_index, _fill); }
		MYGUI_OBSOLETE("use : bool List::isItemSelectedVisible(bool _fill)")
		bool isItemSelectVisible(bool _fill = true) { return isItemSelectedVisible(_fill); }

#endif // MYGUI_DONT_USE_OBSOLETE

	protected:
		virtual ~List();

		void baseChangeWidgetSkin(ResourceSkin* _info);

		void onMouseWheel(int _rel);
		void onKeyLostFocus(Widget* _new);
		void onKeySetFocus(Widget* _old);
		void onKeyButtonPressed(KeyCode _key, Char _char);

		void notifyScrollChangePosition(VScroll* _sender, size_t _rel);
		void notifyMousePressed(Widget* _sender, int _left, int _top, MouseButton _id);
		void notifyMouseDoubleClick(Widget* _sender);
		void notifyMouseWheel(Widget* _sender, int _rel);
		void notifyMouseSetFocus(Widget* _sender, Widget* _old);
		void notifyMouseLostFocus(Widget* _sender, Widget* _new);

		void updateScroll();
		void updateLine(bool _reset = false);
		void _setScrollView(size_t _position);

		// перерисовывает от индекса до низа
		void _redrawItemRange(size_t _start = 0);

		// перерисовывает индекс
		void _redrawItem(size_t _index);

		// ищет и выделяет елемент
		void _selectIndex(size_t _index, bool _select);

		void _updateState() { setState(mIsFocus ? "pushed" : "normal"); }

	private:
		void initialiseWidgetSkin(ResourceSkin* _info);
		void shutdownWidgetSkin();
		void _checkMapping(const std::string& _owner);

		Widget* _getClientWidget();
		const Widget* _getClientWidget() const;

	private:
		std::string mSkinLine;
		VScroll* mWidgetScroll;

		// наши дети в строках
		VectorWidgetPtr mWidgetLines;

		int mHeightLine; // высота одной строки
		int mTopIndex; // индекс самого верхнего элемента
		int mOffsetTop; // текущее смещение
		int mRangeIndex; // размерность скрола
		size_t mLastRedrawLine; // последняя перерисованная линия

		size_t mIndexSelect; // текущий выделенный элемент или ITEM_NONE
		size_t mLineActive; // текущий виджет над которым мыша

		typedef std::pair<UString, Any> PairItem;
		typedef std::vector<PairItem> VectorItemInfo;
		VectorItemInfo mItemsInfo;

		// имеем ли мы фокус ввода
		bool mIsFocus;
		bool mNeedVisibleScroll;

		IntSize mOldSize;

	};

} // namespace MyGUI

#endif // __MYGUI_LIST_H__
