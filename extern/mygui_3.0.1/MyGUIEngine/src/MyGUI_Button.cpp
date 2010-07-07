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
#include "MyGUI_Button.h"
#include "MyGUI_ResourceSkin.h"
#include "MyGUI_StaticImage.h"
#include "MyGUI_InputManager.h"

namespace MyGUI
{

	Button::Button() :
		mIsMousePressed(false),
		mIsMouseFocus(false),
		mIsStateCheck(false),
		mImage(nullptr),
		mModeImage(false)
	{
	}

	void Button::_initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name)
	{
		Base::_initialise(_style, _coord, _align, _info, _parent, _croppedParent, _creator, _name);

		initialiseWidgetSkin(_info);
	}

	Button::~Button()
	{
		shutdownWidgetSkin();
	}

	void Button::baseChangeWidgetSkin(ResourceSkin* _info)
	{
		shutdownWidgetSkin();
		Base::baseChangeWidgetSkin(_info);
		initialiseWidgetSkin(_info);
	}

	void Button::initialiseWidgetSkin(ResourceSkin* _info)
	{
		// парсим свойства
		const MapString& properties = _info->getProperties();
		if (!properties.empty())
		{
			MapString::const_iterator iter = properties.find("ButtonPressed");
			if (iter != properties.end()) setButtonPressed(utility::parseValue<bool>(iter->second));
			iter = properties.find("StateCheck");
			if (iter != properties.end()) setStateCheck(utility::parseValue<bool>(iter->second));
			iter = properties.find("ModeImage");
			if (iter != properties.end()) setModeImage(utility::parseValue<bool>(iter->second));
		}

		for (VectorWidgetPtr::iterator iter=mWidgetChildSkin.begin(); iter!=mWidgetChildSkin.end(); ++iter)
		{
			if (*(*iter)->_getInternalData<std::string>() == "Image")
			{
				MYGUI_DEBUG_ASSERT( ! mImage, "widget already assigned");
				mImage = (*iter)->castType<StaticImage>();
			}
		}
	}

	void Button::shutdownWidgetSkin()
	{
		mImage = nullptr;
	}

	void Button::onMouseSetFocus(Widget* _old)
	{
		_setMouseFocus(true);

		Base::onMouseSetFocus(_old);
	}

	void Button::onMouseLostFocus(Widget* _new)
	{
		_setMouseFocus(false);

		Base::onMouseLostFocus(_new);
	}

	void Button::onMouseButtonPressed(int _left, int _top, MouseButton _id)
	{
		if (_id == MouseButton::Left)
		{
			mIsMousePressed = true;
			updateButtonState();
		}

		Base::onMouseButtonPressed(_left, _top, _id);
	}

	void Button::onMouseButtonReleased(int _left, int _top, MouseButton _id)
	{
		if (_id == MouseButton::Left)
		{
			mIsMousePressed = false;
			updateButtonState();
		}

		Base::onMouseButtonReleased(_left, _top, _id);
	}

	void Button::setImageIndex(size_t _index)
	{
		if (mImage) mImage->setImageIndex(_index);
	}

	size_t Button::getImageIndex()
	{
		if (mImage) return mImage->getImageIndex();
		return ITEM_NONE;
	}

	void Button::updateButtonState()
	{
		if (mIsStateCheck)
		{
			if (!mEnabled) { if (!_setState("disabled_checked")) _setState("disabled"); }
			else if (mIsMousePressed) { if (!_setState("pushed_checked")) _setState("pushed"); }
			else if (mIsMouseFocus) { if (!_setState("highlighted_checked")) _setState("pushed"); }
			else _setState("normal_checked");
		}
		else
		{
			if (!mEnabled) _setState("disabled");
			else if (mIsMousePressed) _setState("pushed");
			else if (mIsMouseFocus) _setState("highlighted");
			else _setState("normal");
		}
	}

	void Button::setStateCheck(bool _check)
	{
		if (mIsStateCheck == _check) return;
		mIsStateCheck = _check;
		updateButtonState();
	}

	void Button::_setMouseFocus(bool _focus)
	{
		mIsMouseFocus = _focus;
		updateButtonState();
	}

	void Button::setProperty(const std::string& _key, const std::string& _value)
	{
		/// @wproperty{Button, Button_Pressed, bool} Set pressed state.
		if (_key == "Button_Pressed") setButtonPressed(utility::parseValue<bool>(_value));
		else if (_key == "Button_ModeImage") setModeImage(utility::parseValue<bool>(_value));
		else if (_key == "Button_ImageResource") setImageResource(_value);
		else
		{
			Base::setProperty(_key, _value);
			return;
		}
		eventChangeProperty(this, _key, _value);
	}

	void Button::baseUpdateEnable()
	{
		updateButtonState();
		if (!mEnabled)
		{
			mIsMouseFocus = false;
		}
	}

	void Button::setModeImage(bool _value)
	{
		mModeImage = _value;
		updateButtonState();
	}

	bool Button::_setState(const std::string& _value)
	{
		if (mModeImage)
		{
			if (mImage)
				mImage->setItemName(_value);

			setState(_value);
			return true;
		}

		return setState(_value);
	}

	void Button::setImageResource(const std::string& _name)
	{
		if (mImage)
			mImage->setItemResource(_name);
		updateButtonState();
	}

} // namespace MyGUI
