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
#include "MyGUI_Precompiled.h"
#include "MyGUI_MenuItem.h"
#include "MyGUI_SkinManager.h"
#include "MyGUI_SubWidgetManager.h"

namespace MyGUI
{

	MenuItem::MenuItem() :
		mOwner(nullptr)
	{
	}

	void MenuItem::_initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name)
	{
		Base::_initialise(_style, _coord, _align, _info, _parent, _croppedParent, _creator, _name);

		Widget* parent = getParent();
		MYGUI_ASSERT(parent, "MenuItem must have parent MenuCtrl");
		if (!parent->isType<MenuCtrl>())
		{
			Widget* client = parent;
			parent = client->getParent();
			MYGUI_ASSERT(parent, "MenuItem must have parent MenuCtrl");
			MYGUI_ASSERT(parent->getClientWidget() == client, "MenuItem must have parent MenuCtrl");
			MYGUI_ASSERT(parent->isType<MenuCtrl>(), "MenuItem must have parent MenuCtrl");
		}
		mOwner = parent->castType<MenuCtrl>();

		initialiseWidgetSkin(_info);

		// нам нуженфокус клавы
		this->mNeedKeyFocus = true;
	}

	MenuItem::~MenuItem()
	{
		shutdownWidgetSkin();
		mOwner->_notifyDeleteItem(this);
	}

	Widget* MenuItem::baseCreateWidget(WidgetStyle _style, const std::string& _type, const std::string& _skin, const IntCoord& _coord, Align _align, const std::string& _layer, const std::string& _name)
	{
		Widget* widget = Base::baseCreateWidget(_style, _type, _skin, _coord, _align, _layer, _name);
		MenuCtrl* child = widget->castType<MenuCtrl>(false);
		if (child) mOwner->_wrapItemChild(this, child);
		return widget;
	}

	void MenuItem::baseChangeWidgetSkin(ResourceSkin* _info)
	{
		shutdownWidgetSkin();
		Button::baseChangeWidgetSkin(_info);
		initialiseWidgetSkin(_info);
	}

	void MenuItem::initialiseWidgetSkin(ResourceSkin* _info)
	{
	}

	void MenuItem::shutdownWidgetSkin()
	{
	}

	void MenuItem::onMouseButtonPressed(int _left, int _top, MouseButton _id)
	{
		Base::onMouseButtonPressed(_left, _top, _id);
	}

	void MenuItem::onMouseButtonReleased(int _left, int _top, MouseButton _id)
	{
		Base::onMouseButtonReleased(_left, _top, _id);
	}

	void MenuItem::setCaption(const UString& _value)
	{
		Button::setCaption(_value);
		mOwner->_notifyUpdateName(this);
	}

	const UString& MenuItem::getItemName()
	{
		return mOwner->getItemName(this);
	}

	void MenuItem::setItemName(const UString& _value)
	{
		mOwner->setItemName(this, _value);
	}

	void MenuItem::setItemData(Any _data)
	{
		mOwner->setItemData(this, _data);
	}

	void MenuItem::removeItem()
	{
		mOwner->removeItem(this);
	}

	void MenuItem::setItemId(const std::string& _id)
	{
		mOwner->setItemId(this, _id);
	}

	const std::string& MenuItem::getItemId()
	{
		return mOwner->getItemId(this);
	}

	size_t MenuItem::getItemIndex()
	{
		return mOwner->getItemIndex(this);
	}

	MenuCtrl* MenuItem::createItemChild()
	{
		return mOwner->createItemChild(this);
	}

	void MenuItem::setItemType(MenuItemType _type)
	{
		mOwner->setItemType(this, _type);
	}

	MenuItemType MenuItem::getItemType()
	{
		return mOwner->getItemType(this);
	}

	void MenuItem::setItemChildVisible(bool _visible)
	{
		mOwner->setItemChildVisible(this, _visible);
	}

	MenuCtrl* MenuItem::getItemChild()
	{
		return mOwner->getItemChild(this);
	}

	void MenuItem::setProperty(const std::string& _key, const std::string& _value)
	{
		if (_key == "MenuItem_Id") setItemId(_value);
		else if (_key == "MenuItem_Type") setItemType(utility::parseValue<MenuItemType>(_value));
		else
		{
			Base::setProperty(_key, _value);
			return;
		}
		eventChangeProperty(this, _key, _value);
	}

} // namespace MyGUI
