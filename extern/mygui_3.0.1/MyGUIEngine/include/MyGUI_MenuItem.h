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
#ifndef __MYGUI_MENU_ITEM_H__
#define __MYGUI_MENU_ITEM_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Button.h"
#include "MyGUI_MenuCtrl.h"

namespace MyGUI
{

	class MYGUI_EXPORT MenuItem :
		public Button
	{
		MYGUI_RTTI_DERIVED( MenuItem )

	public:
		MenuItem();

		/** @copydoc Widget::setCaption(const UString& _value) */
		virtual void setCaption(const UString& _value);

		//! Replace an item name
		void setItemName(const UString& _value);
		//! Get item name
		const UString& getItemName();

		//! Replace an item name
		void setItemData(Any _value);

		//! Get item data
		template <typename ValueType>
		ValueType* getItemData(bool _throw = true)
		{
			return mOwner->getItemData<ValueType>(this, _throw);
		}

		//! Remove item
		void removeItem();

		//! Replace an item id at a specified position
		void setItemId(const std::string& _value);
		//! Get item id from specified position
		const std::string& getItemId();

		//! Get item index
		size_t getItemIndex();

		/** Create child item (submenu), MenuItem can have only one child */
		MenuCtrl* createItemChild();

		/** Create specific type child item (submenu), MenuItem can have only one child */
		template <typename Type>
		Type * createItemChildT()
		{
			return mOwner->createItemChildT<Type>(this);
		}

		/** Set item type (see MenuItemType) */
		void setItemType(MenuItemType _value);
		/** Get item type (see MenuItemType) */
		MenuItemType getItemType();

		/** Hide or show child item (submenu) */
		void setItemChildVisible(bool _value);

		/** Get parent MenuCtrl */
		MenuCtrl* getMenuCtrlParent() { return mOwner; }

		/** Get child item (submenu) */
		MenuCtrl* getItemChild();

		/** @copydoc Widget::setProperty(const std::string& _key, const std::string& _value) */
		virtual void setProperty(const std::string& _key, const std::string& _value);


	/*internal:*/
		virtual void _initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name);

	/*obsolete:*/
#ifndef MYGUI_DONT_USE_OBSOLETE

		MYGUI_OBSOLETE("use : void setItemChildVisible(bool _visible)")
		void showItemChild() { setItemChildVisible(true); }
		MYGUI_OBSOLETE("use : void setItemChildVisible(bool _visible)")
		void hideItemChild() { setItemChildVisible(false); }

#endif // MYGUI_DONT_USE_OBSOLETE

	protected:
		virtual ~MenuItem();

		virtual Widget* baseCreateWidget(WidgetStyle _style, const std::string& _type, const std::string& _skin, const IntCoord& _coord, Align _align, const std::string& _layer, const std::string& _name);

		virtual void baseChangeWidgetSkin(ResourceSkin* _info);
		void initialiseWidgetSkin(ResourceSkin* _info);
		void shutdownWidgetSkin();

	private:
		virtual void onMouseButtonPressed(int _left, int _top, MouseButton _id);
		virtual void onMouseButtonReleased(int _left, int _top, MouseButton _id);

	private:
		MenuCtrl* mOwner;

	};

} // namespace MyGUI

#endif // __MYGUI_MENU_ITEM_H__
