/*!
	@file
	@author		Albert Semenov
	@date		11/2008
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
#ifndef __MYGUI_MENU_CTRL_H__
#define __MYGUI_MENU_CTRL_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Types.h"
#include "MyGUI_MenuItemType.h"
#include "MyGUI_Widget.h"
#include "MyGUI_Any.h"
#include "MyGUI_EventPair.h"
#include "MyGUI_MenuItemType.h"
#include "MyGUI_ControllerFadeAlpha.h"

namespace MyGUI
{

	typedef delegates::CDelegate2<MenuCtrl*, MenuItem*> EventHandle_MenuCtrlPtrMenuItemPtr;
	typedef delegates::CDelegate1<MenuCtrl*> EventHandle_MenuCtrlPtr;

	class MYGUI_EXPORT MenuCtrl :
		public Widget
	{
		MYGUI_RTTI_DERIVED( MenuCtrl )

	public:
		MenuCtrl();

		enum ItemImage
		{
			ItemImageNone,
			ItemImagePopup
		};

		struct ItemInfo
		{
			ItemInfo(MenuItem* _item, const UString& _name, MenuItemType _type, MenuCtrl* _submenu, const std::string& _id, Any _data) :
				item(_item),
				name(_name),
				type(_type),
				submenu(_submenu),
				id(_id),
				data(_data),
				width(0)
			{
			}

			/** Item */
			MenuItem* item;
			/** Item name*/
			UString name;
			/** Widget have separator after item */
			MenuItemType type;
			/** Sub menu (or nullptr if no submenu) */
			MenuCtrl* submenu;
			/** Item id*/
			std::string id;
			/** User data */
			Any data;
			/** Item width */
			int width;
		};

		typedef std::vector<ItemInfo> VectorMenuItemInfo;

	public:
		/** @copydoc Widget::setVisible */
		virtual void setVisible(bool _value);

		/** Hide or show Menu smooth */
		void setVisibleSmooth(bool _value);

		//------------------------------------------------------------------------------//
		// манипуляции айтемами

		//! Get number of items
		size_t getItemCount() const { return mItemsInfo.size(); }

		//! Insert an item into a array at a specified position
		MenuItem* insertItemAt(size_t _index, const UString& _name, MenuItemType _type = MenuItemType::Normal, const std::string& _id = "", Any _data = Any::Null);
		//! Insert an item into a array
		MenuItem* insertItem(MenuItem* _to, const UString& _name, MenuItemType _type = MenuItemType::Normal, const std::string& _id = "", Any _data = Any::Null);

		//! Add an item to the end of a array
		MenuItem* addItem(const UString& _name, MenuItemType _type = MenuItemType::Normal, const std::string& _id = "", Any _data = Any::Null);

		//! Remove item at a specified position
		void removeItemAt(size_t _index);
		//! Remove item
		void removeItem(MenuItem* _item);

		//! Remove all items
		void removeAllItems();


		//! Get item from specified position
		MenuItem* getItemAt(size_t _index);

		//! Get item index
		size_t getItemIndex(MenuItem* _item);

		//! Search item, returns the position of the first occurrence in array or ITEM_NONE if item not found
		size_t findItemIndex(MenuItem* _item);

		//! Search item, returns the item of the first occurrence in array or nullptr if item not found
		MenuItem* findItemWith(const UString& _name);

		//------------------------------------------------------------------------------//
		// манипуляции данными

		//! Replace an item data at a specified position
		void setItemDataAt(size_t _index, Any _data);
		//! Replace an item data
		void setItemData(MenuItem* _item, Any _data) { setItemDataAt(getItemIndex(_item), _data); }

		//! Clear an item data at a specified position
		void clearItemDataAt(size_t _index) { setItemDataAt(_index, Any::Null); }
		//! Clear an item data
		void clearItemData(MenuItem* _item) { clearItemDataAt(getItemIndex(_item)); }

		//! Get item data from specified position
		template <typename ValueType>
		ValueType * getItemDataAt(size_t _index, bool _throw = true)
		{
			MYGUI_ASSERT_RANGE(_index, mItemsInfo.size(), "MenuCtrl::getItemDataAt");
			return mItemsInfo[_index].data.castType<ValueType>(_throw);
		}
		//! Get item data
		template <typename ValueType>
		ValueType * getItemData(MenuItem* _item, bool _throw = true)
		{
			return getItemDataAt<ValueType>(getItemIndex(_item), _throw);
		}

		//! Replace an item id at a specified position
		void setItemIdAt(size_t _index, const std::string& _id);
		//! Replace an item id
		void setItemId(MenuItem* _item, const std::string& _id) { setItemIdAt(getItemIndex(_item), _id); }

		//! Get item id from specified position
		const std::string& getItemIdAt(size_t _index);
		//! Get item id
		const std::string& getItemId(MenuItem* _item) { return getItemIdAt(getItemIndex(_item)); }

		/** Get item by id */
		MenuItem* getItemById(const std::string& _id);

		/** Get item index by id */
		size_t getItemIndexById(const std::string& _id);
		//------------------------------------------------------------------------------//
		// манипуляции отображением

		//! Replace an item name at a specified position
		void setItemNameAt(size_t _index, const UString& _name);
		//! Replace an item name
		void setItemName(MenuItem* _item, const UString& _name) { setItemNameAt(getItemIndex(_item), _name); }

		//! Get item from specified position
		const UString& getItemNameAt(size_t _index);
		//! Get item from specified position
		const UString& getItemName(MenuItem* _item) { return getItemNameAt(getItemIndex(_item)); }

		//! Search item, returns the position of the first occurrence in array or ITEM_NONE if item not found
		size_t findItemIndexWith(const UString& _name);

		/** Show or hide item (submenu) at a specified position */
		void setItemChildVisibleAt(size_t _index, bool _visible);
		/** Show or hide item (submenu) */
		void setItemChildVisible(MenuItem* _item, bool _visible) { setItemChildVisibleAt(getItemIndex(_item), _visible); }

		//------------------------------------------------------------------------------//
		// остальные манипуляции

		/** Create specific type child item (submenu) for item by index */
		template <typename Type>
		Type * createItemChildTAt(size_t _index)
		{
			return static_cast<Type*>(createItemChildByType(_index, Type::getClassTypeName()));
		}

		/** Create specific type child item (submenu) for item */
		template <typename Type>
		Type * createItemChildT(MenuItem* _item) { return createItemChildTAt<Type>(getItemIndex(_item)); }

		/** Get child item (submenu) from item by index */
		MenuCtrl* getItemChildAt(size_t _index);

		/** Get child item (submenu) from item */
		MenuCtrl* getItemChild(MenuItem* _item) { return getItemChildAt(getItemIndex(_item)); }

		/** Create child item (submenu) for item by index */
		MenuCtrl* createItemChildAt(size_t _index) { return createItemChildTAt<MenuCtrl>(_index); }

		/** Create child item (submenu) for item */
		MenuCtrl* createItemChild(MenuItem* _item) { return createItemChildAt(getItemIndex(_item)); }

		/** Remove child item (submenu) for item by index */
		void removeItemChildAt(size_t _index);

		/** Remove child item (submenu) for item */
		void removeItemChild(MenuItem* _item) { removeItemChildAt(getItemIndex(_item)); }


		/** Get item type (see MenuItemType) from item by index */
		MenuItemType getItemTypeAt(size_t _index);

		/** Get item type (see MenuItemType) from item */
		MenuItemType getItemType(MenuItem* _item) { return getItemTypeAt(getItemIndex(_item)); }

		/** Set item type (see MenuItemType) from item by index */
		void setItemTypeAt(size_t _index, MenuItemType _type);
		/** Set item type (see MenuItemType) from item */
		void setItemType(MenuItem* _item, MenuItemType _type) { setItemTypeAt(getItemIndex(_item), _type); }

		/** Set mode when clicking on item with submenu generate eventMenuCtrlAccept and closes menu */
		void setPopupAccept(bool _value) { mPopupAccept = _value; }
		/** Get mode when clicking on item with submenu generate eventMenuCtrlAccept and closes menu */
		bool getPopupAccept() { return mPopupAccept; }

		/** Get parent menu item or nullptr if no item */
		MenuItem* getMenuItemParent() { return mOwner; }


	/*event:*/
		/** Event : Enter pressed or mouse clicked.\n
			signature : void method(MyGUI::MenuCtrl* _sender, MyGUI::MenuItem* _item)\n
			@param _sender widget that called this event
			@param _item Selected item
		*/
		EventHandle_MenuCtrlPtrMenuItemPtr eventMenuCtrlAccept;

		/** Event : Menu was closed by select or focus change.\n
			signature : void method(MyGUI::MenuCtrl* _sender)\n
			@param _sender widget that called this event
		*/
		EventHandle_MenuCtrlPtr eventMenuCtrlClose;


	/*internal:*/
		void _notifyDeleteItem(MenuItem* _item);
		void _notifyUpdateName(MenuItem* _item);
		void _wrapItemChild(MenuItem* _item, MenuCtrl* _widget);

		virtual void _initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name);

	/*obsolete:*/
#ifndef MYGUI_DONT_USE_OBSOLETE

		MYGUI_OBSOLETE("use : void Widget::setVisible(bool _value)")
		void showMenu() { setVisible(true); }
		MYGUI_OBSOLETE("use : void Widget::setVisible(bool _value)")
		void hideMenu() { setVisible(false); }
		MYGUI_OBSOLETE("use : bool Widget::isVisible()")
		bool isShowMenu() { return isVisible(); }

		MYGUI_OBSOLETE("use : void setItemChildVisibleAt(size_t _index, bool _visible)")
		void showItemChildAt(size_t _index) { setItemChildVisibleAt(_index, true); }
		MYGUI_OBSOLETE("use : void setItemChildVisible(MenuItem* _item, bool _visible)")
		void showItemChild(MenuItem* _item) { setItemChildVisible(_item, true); }
		MYGUI_OBSOLETE("use : void setItemChildVisibleAt(size_t _index, bool _visible)")
		void hideItemChildAt(size_t _index) { setItemChildVisibleAt(_index, false); }
		MYGUI_OBSOLETE("use : void setItemChildVisible(MenuItem* _item, bool _visible)")
		void hideItemChild(MenuItem* _item) { setItemChildVisible(_item, false); }

#endif // MYGUI_DONT_USE_OBSOLETE

	protected:
		virtual ~MenuCtrl();

		void baseChangeWidgetSkin(ResourceSkin* _info);

		// переопределяем для особого обслуживания
		virtual Widget* baseCreateWidget(WidgetStyle _style, const std::string& _type, const std::string& _skin, const IntCoord& _coord, Align _align, const std::string& _layer, const std::string& _name);

		virtual void onKeyChangeRootFocus(bool _focus);

	private:
		void initialiseWidgetSkin(ResourceSkin* _info);
		void shutdownWidgetSkin();

		void notifyRootKeyChangeFocus(Widget* _sender, bool _focus);
		void notifyMouseButtonClick(Widget* _sender);
		void notifyMouseSetFocus(Widget* _sender, Widget* _new);

		const std::string& getSkinByType(MenuItemType _type)
		{
			return _type == MenuItemType::Separator ? mSeparatorSkin : mSkinLine;
		}

		size_t getIconIndexByType(MenuItemType _type)
		{
			return _type == MenuItemType::Popup ? ItemImagePopup : ItemImageNone;
		}

		void update();

		void setButtonImageIndex(Button* _button, size_t _index);

		MenuItemType getItemType(bool _submenu, bool _separator)
		{
			if (_submenu) return MenuItemType::Popup;
			else if (_separator)  return MenuItemType::Separator;
			return MenuItemType::Normal;
		}

		void notifyMenuCtrlAccept(MenuItem* _item);

		Widget* createItemChildByType(size_t _index, const std::string& _type);

		void _wrapItem(MenuItem* _item, size_t _index, const UString& _name, MenuItemType _type, const std::string& _id, Any _data);

		ControllerFadeAlpha* createControllerFadeAlpha(float _alpha, float _coef, bool _enable);

		Widget* _getClientWidget();
		const Widget* _getClientWidget() const;

	protected:
		bool mHideByAccept;
		// нужно ли выбрасывать по нажатию
		bool mMenuDropMode;
		bool mIsMenuDrop;
		bool mHideByLostKey;

	private:
		VectorMenuItemInfo mItemsInfo;

		int mHeightLine;
		std::string mSkinLine;

		int mSubmenuImageSize;

		std::string mSubMenuSkin;
		std::string mSubMenuLayer;

		// флаг, чтобы отсеч уведомления от айтемов, при общем шутдауне виджета
		bool mShutdown;

		int mSeparatorHeight;
		std::string mSeparatorSkin;

		bool mAlignVert;
		int mDistanceButton;
		bool mPopupAccept;
		MenuItem* mOwner;
		bool mAnimateSmooth;

	};

} // namespace MyGUI

#endif // __MYGUI_MENU_CTRL_H__
