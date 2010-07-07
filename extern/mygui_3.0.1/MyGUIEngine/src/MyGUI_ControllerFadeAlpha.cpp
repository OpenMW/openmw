/*!
	@file
	@author		Albert Semenov
	@date		01/2008
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
#include "MyGUI_ControllerFadeAlpha.h"
#include "MyGUI_Gui.h"
#include "MyGUI_InputManager.h"
#include "MyGUI_WidgetManager.h"
#include "MyGUI_Widget.h"

namespace MyGUI
{

	ControllerFadeAlpha::ControllerFadeAlpha() :
		mAlpha(1),
		mCoef(1),
		mEnabled(true)
	{
	}

	void ControllerFadeAlpha::prepareItem(Widget* _widget)
	{
		// подготовка виджета, блокируем если только нужно
		if (!mEnabled) _widget->setEnabledSilent(mEnabled);

		if ((ALPHA_MIN != mAlpha) && (!_widget->isVisible()))
		{
			_widget->setAlpha(ALPHA_MIN);
			_widget->setVisible(true);
		}

		// отписываем его от ввода
		if (!mEnabled) InputManager::getInstance().unlinkWidget(_widget);

		// вызываем пользовательский делегат для подготовки
		eventPreAction(_widget);
	}

	bool ControllerFadeAlpha::addTime(Widget* _widget, float _time)
	{
		float alpha = _widget->getAlpha();

		// проверяем нужно ли к чему еще стремиться
		if (mAlpha > alpha)
		{
			alpha += _time * mCoef;
			if (mAlpha > alpha)
			{
				_widget->setAlpha(alpha);
				eventUpdateAction(_widget);
				return true;
			}
			else
			{
				_widget->setAlpha(mAlpha);
			}
		}
		else if (mAlpha < alpha)
		{
			alpha -= _time * mCoef;
			if (mAlpha < alpha)
			{
				_widget->setAlpha(alpha);
				eventUpdateAction(_widget);
				return true;
			}
			else
			{
				_widget->setAlpha(mAlpha);
			}
		}

		// вызываем пользовательский делегат пост обработки
		eventPostAction(_widget);

		return false;
	}

	void ControllerFadeAlpha::setProperty(const std::string& _key, const std::string& _value)
	{
		if (_key == "Alpha") setAlpha(utility::parseValue<float>(_value));
		else if (_key == "Coef") setCoef(utility::parseValue<float>(_value));
		else if (_key == "Enabled") setEnabled(utility::parseValue<bool>(_value));
	}

} // namespace MyGUI
