/*!
	@file
	@author		Evmenov Georgiy
	@date		03/2008
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
#include "MyGUI_ControllerEdgeHide.h"
#include "MyGUI_Gui.h"
#include "MyGUI_InputManager.h"
#include "MyGUI_WidgetManager.h"
#include "MyGUI_Widget.h"

namespace MyGUI
{

#ifndef M_PI
	const float M_PI = 3.141593f;
#endif

	ControllerEdgeHide::ControllerEdgeHide() :
		mTime(1.0),
		mRemainPixels(0),
		mShadowSize(0),
		mElapsedTime(0)
	{
	}

	void ControllerEdgeHide::prepareItem(Widget* _widget)
	{
		recalculateTime(_widget);
		// вызываем пользовательский делегат для подготовки
		eventPreAction(_widget);
	}

	bool ControllerEdgeHide::addTime(Widget* _widget, float _time)
	{
		Widget* keyFocus = InputManager::getInstance().getKeyFocusWidget();
		Widget* mouseFocus = InputManager::getInstance().getMouseFocusWidget();

		while ((keyFocus != nullptr) && (_widget != keyFocus))
			keyFocus = keyFocus->getParent();
		while ((mouseFocus != nullptr) && (_widget != mouseFocus))
			mouseFocus = mouseFocus->getParent();

		// if our widget or its children have focus
		bool haveFocus = ((keyFocus != nullptr) || (mouseFocus != nullptr)) || (_widget->isVisible() == false);

		mElapsedTime += (1 - 2*haveFocus) * _time;

		if (mElapsedTime >= mTime)
		{
			mElapsedTime = mTime;
		}
		if (mElapsedTime <= 0)
		{
			mElapsedTime = 0.0f;
			return true;
		}

		float k = sin(M_PI * mElapsedTime/mTime - M_PI/2);
		if (k<0) k = (-pow(-k, 0.7f) + 1)/2;
		else k = (pow(k, 0.7f) + 1)/2;

		MyGUI::IntCoord coord = _widget->getCoord();
		// if widget was moved
		if (coord != mLastCoord)
		{
			// if still moving - leave it alone
			if (haveFocus)
				return true;
			else
				recalculateTime(_widget);
		}

		IntSize view_size;
		if (_widget->getCroppedParent() == nullptr)
			view_size = _widget->getLayer()->getSize();
		else
			view_size = ((Widget*)_widget->getCroppedParent())->getSize();

		bool nearBorder = false;

		if ((coord.left <= 0) && !(coord.right() >= view_size.width - 1))
		{
			coord.left = - int( float(coord.width - mRemainPixels - mShadowSize) * k);
			nearBorder = true;
		}
		else if ((coord.top <= 0) && !(coord.bottom() >= view_size.height - 1))
		{
			coord.top = - int( float(coord.height - mRemainPixels - mShadowSize) * k);
			nearBorder = true;
		}
		else if ((coord.right() >= view_size.width - 1) && !(coord.left <= 0))
		{
			coord.left = int(float(view_size.width - 1) - float(mRemainPixels)*k - float(coord.width) * (1.f - k));
			nearBorder = true;
		}
		else if ((coord.bottom() >= view_size.height-1) && !(coord.top <= 0))
		{
			coord.top = int(float(view_size.height-1) - float(mRemainPixels)*k - float(coord.height) * (1.f - k));
			nearBorder = true;
		}

		if (nearBorder)
		{
			_widget->setCoord(coord);
		}
		else
		{
			mElapsedTime = 0;
		}
		mLastCoord = coord;

		eventUpdateAction(_widget);

		return true;
	}

	void ControllerEdgeHide::setProperty(const std::string& _key, const std::string& _value)
	{
		if (_key == "Time") setTime(utility::parseValue<float>(_value));
		else if (_key == "RemainPixels") setRemainPixels(utility::parseValue<int>(_value));
		else if (_key == "ShadowSize") setShadowSize(utility::parseValue<int>(_value));
	}

	void ControllerEdgeHide::recalculateTime(Widget* _widget)
	{
		float k = 0;
		const MyGUI::IntCoord& coord = _widget->getCoord();		IntSize view_size;
		if (_widget->getCroppedParent() == nullptr)
			view_size = _widget->getLayer()->getSize();
		else
			view_size = ((Widget*)_widget->getCroppedParent())->getSize();


		// check if widget is near any border and not near opposite borders at same time
		if ((coord.left <= 0) && !(coord.right() >= view_size.width - 1))
		{
			k = - (float) coord.left / (coord.width - mRemainPixels - mShadowSize);
		}
		else if ((coord.top <= 0) && !(coord.bottom() >= view_size.height - 1))
		{
			k = - (float)coord.top / (coord.height - mRemainPixels - mShadowSize);
		}
		else if ((coord.right() >= view_size.width - 1) && !(coord.left <= 0))
		{
			k = (float)(coord.right() - view_size.width + 1 ) / (coord.width - mRemainPixels);
		}
		else if ((coord.bottom() >= view_size.height - 1) && !(coord.top <= 0))
		{
			k = (float)(coord.bottom() - view_size.height + 1 ) / (coord.height - mRemainPixels);
		}

		//mElapsedTime = (asin(k)/M_PI + 1./2) * mTime;
		// this is reversed formula from ControllerEdgeHide::addTime k calculation
		if (k > 0.5f)
			mElapsedTime = (asin( pow( 2*k - 1, 1/0.7f))/M_PI + 1.f/2) * mTime;
		else
			mElapsedTime = (asin(-pow(-2*k + 1, 1/0.7f))/M_PI + 1.f/2) * mTime;
	}

} // namespace MyGUI
