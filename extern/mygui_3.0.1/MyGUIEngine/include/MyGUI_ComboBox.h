/*!
	@file
	@author		Albert Semenov
	@date		12/2007
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
#ifndef __MYGUI_COMBO_BOX_H__
#define __MYGUI_COMBO_BOX_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Edit.h"
#include "MyGUI_List.h"
#include "MyGUI_Any.h"
#include "MyGUI_EventPair.h"
#include "MyGUI_ControllerFadeAlpha.h"

namespace MyGUI
{

	typedef delegates::CDelegate2<ComboBox*, size_t> EventHandle_ComboBoxPtrSizeT;

	class MYGUI_EXPORT ComboBox :
		public Edit
	{
		MYGUI_RTTI_DERIVED( ComboBox )

	public:
		ComboBox();

		//------------------------------------------------------------------------------//
		// манипуляции айтемами

		//! Get number of items
		size_t getItemCount() const { return mList->getItemCount(); }

		//! Insert an item into a array at a specified position
		void insertItemAt(size_t _index, const UString& _name, Any _data = Any::Null);

		//! Add an item to the end of a array
		void addItem(const UString& _name, Any _data = Any::Null) { return insertItemAt(ITEM_NONE, _name, _data); }

		//! Remove item at a specified position
		void removeItemAt(size_t _index);

		//! Remove all items
		void removeAllItems();


		//! Search item, returns the position of the first occurrence in array or ITEM_NONE if item not found
		size_t findItemIndexWith(const UString& _name);


		//------------------------------------------------------------------------------//
		// манипуляции выделениями

		//! Get index of selected item (ITEM_NONE if none selected)
		size_t getIndexSelected() { return mItemIndex; }

		//! Select specified _index
		void setIndexSelected(size_t _index);

		//! Clear item selection
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
			return mList->getItemDataAt<ValueType>(_index, _throw);
		}


		//------------------------------------------------------------------------------//
		// манипуляции отображением

		//! Replace an item name at a specified position
		void setItemNameAt(size_t _index, const UString& _name);

		//! Get item name from specified position
		const UString& getItemNameAt(size_t _index) { return mList->getItemNameAt(_index); }


		//------------------------------------------------------------------------------//
		// манипуляции выдимостью

		//! Move all elements so specified becomes visible
		void beginToItemAt(size_t _index) { mList->beginToItemAt(_index); }

		//! Move all elements so first becomes visible
		void beginToItemFirst() { if (getItemCount()) beginToItemAt(0); }

		//! Move all elements so last becomes visible
		void beginToItemLast() { if (getItemCount()) beginToItemAt(getItemCount() - 1); }

		//! Move all elements so selected becomes visible
		void beginToItemSelected() { if (getIndexSelected() != ITEM_NONE) beginToItemAt(getIndexSelected()); }


		//------------------------------------------------------------------------------------//
		// методы для управления отображением

		//! Set drop list mode (text can not be edited)
		void setComboModeDrop(bool _value);
		//! Get drop list mode flag
		bool getComboModeDrop() { return mModeDrop; }

		//! Set smooth show of list
		void setSmoothShow(bool _value) { mShowSmooth = _value; }
		//! Get smooth show of list flag
		bool getSmoothShow() { return mShowSmooth; }

		//! Get max list height
		void setMaxListHeight(int _value) { mMaxHeight = _value; }
		//! Set max list height
		int getMaxListHeight() { return mMaxHeight; }

		/** @copydoc Widget::setProperty(const std::string& _key, const std::string& _value) */
		virtual void setProperty(const std::string& _key, const std::string& _value);

	/*event:*/
		/** Event : Enter pressed in combo mode or item selected in drop.\n
			signature : void method(MyGUI::ComboBox* _sender, size_t _index)
			@param _sender widget that called this event
			@param _index item
		*/
		EventPair<EventHandle_WidgetVoid, EventHandle_ComboBoxPtrSizeT> eventComboAccept;

		/** Event : Position changed.\n
			signature : void method(MyGUI::ComboBox* _sender, size_t _index)
			@param _sender widget that called this event
			@param _index of new position
		*/
		EventPair<EventHandle_WidgetSizeT, EventHandle_ComboBoxPtrSizeT> eventComboChangePosition;


	/*internal:*/
		virtual void _initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name);

	/*obsolete:*/
#ifndef MYGUI_DONT_USE_OBSOLETE

		MYGUI_OBSOLETE("use : size_t ComboBox::getIndexSelected()")
		size_t getItemIndexSelected() { return getIndexSelected(); }
		MYGUI_OBSOLETE("use : void ComboBox::setIndexSelected(size_t _index)")
		void setItemSelectedAt(size_t _index) { setIndexSelected(_index); }
		MYGUI_OBSOLETE("use : void ComboBox::clearIndexSelected()")
		void clearItemSelected() { clearIndexSelected(); }

		MYGUI_OBSOLETE("use : void ComboBox::insertItemAt(size_t _index, const UString& _name)")
		void insertItem(size_t _index, const UString& _name) { insertItemAt(_index, _name); }
		MYGUI_OBSOLETE("use : void ComboBox::setItemNameAt(size_t _index, const UString& _name)")
		void setItem(size_t _index, const UString& _item) { setItemNameAt(_index, _item); }
		MYGUI_OBSOLETE("use : const UString& ComboBox::getItemNameAt(size_t _index)")
		const UString& getItem(size_t _index) { return getItemNameAt(_index); }
		MYGUI_OBSOLETE("use : void ComboBox::removeItemAt(size_t _index)")
		void deleteItem(size_t _index) { removeItemAt(_index); }
		MYGUI_OBSOLETE("use : void ComboBox::removeAllItems()")
		void deleteAllItems() { removeAllItems(); }
		MYGUI_OBSOLETE("use : size_t ComboBox::getIndexSelected()")
		size_t getItemSelect() { return getIndexSelected(); }
		MYGUI_OBSOLETE("use : void void ComboBox::clearIndexSelected()")
		void resetItemSelect() { clearIndexSelected(); }
		MYGUI_OBSOLETE("use : void ComboBox::setIndexSelected(size_t _index)")
		void setItemSelect(size_t _index) { setIndexSelected(_index); }

#endif // MYGUI_DONT_USE_OBSOLETE

	protected:
		virtual ~ComboBox();

		virtual void onKeyButtonPressed(KeyCode _key, Char _char);

		virtual void baseChangeWidgetSkin(ResourceSkin* _info);

	private:
		void notifyButtonPressed(Widget* _sender, int _left, int _top, MouseButton _id);
		void notifyListLostFocus(Widget* _sender, MyGUI::Widget* _new);
		void notifyListSelectAccept(List* _widget, size_t _position);
		void notifyListMouseItemActivate(List* _widget, size_t _position);
		void notifyListChangePosition(List* _widget, size_t _position);
		void notifyMouseWheel(Widget* _sender, int _rel);
		void notifyMousePressed(Widget* _sender, int _left, int _top, MouseButton _id);
		void notifyEditTextChange(Edit* _sender);

		void showList();
		void hideList();

		void initialiseWidgetSkin(ResourceSkin* _info);
		void shutdownWidgetSkin();

		void actionWidgetHide(Widget* _widget);

		ControllerFadeAlpha* createControllerFadeAlpha(float _alpha, float _coef, bool _enable);

	private:
		Button* mButton;
		List* mList;

		bool mListShow;
		int mMaxHeight;
		size_t mItemIndex;
		bool mModeDrop;
		bool mDropMouse;
		bool mShowSmooth;
		bool mManualList;

	};

} // namespace MyGUI

#endif // __MYGUI_COMBO_BOX_H__
