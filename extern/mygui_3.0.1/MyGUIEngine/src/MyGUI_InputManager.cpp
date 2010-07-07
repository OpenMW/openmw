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
#include "MyGUI_InputManager.h"
#include "MyGUI_Widget.h"
#include "MyGUI_WidgetManager.h"
#include "MyGUI_Gui.h"
#include "MyGUI_WidgetManager.h"

namespace MyGUI
{
	const unsigned long INPUT_TIME_DOUBLE_CLICK = 250; //measured in milliseconds
	const float INPUT_DELAY_FIRST_KEY = 0.4f;
	const float INPUT_INTERVAL_KEY = 0.05f;

	MYGUI_INSTANCE_IMPLEMENT( InputManager )

	void InputManager::initialise()
	{
		MYGUI_ASSERT(!mIsInitialise, INSTANCE_TYPE_NAME << " initialised twice");
		MYGUI_LOG(Info, "* Initialise: " << INSTANCE_TYPE_NAME);

		mWidgetMouseFocus = 0;
		mWidgetKeyFocus = 0;
		mLayerMouseFocus = 0;
		mIsWidgetMouseCapture = false;
		mIsShiftPressed = false;
		mIsControlPressed = false;
		mHoldKey = KeyCode::None;
		mHoldChar = 0;
		mFirstPressKey = true;
		mTimerKey = 0.0f;
		mOldAbsZ = 0;

		WidgetManager::getInstance().registerUnlinker(this);
		Gui::getInstance().eventFrameStart += newDelegate(this, &InputManager::frameEntered);

		MYGUI_LOG(Info, INSTANCE_TYPE_NAME << " successfully initialized");
		mIsInitialise = true;
	}

	void InputManager::shutdown()
	{
		if (!mIsInitialise) return;
		MYGUI_LOG(Info, "* Shutdown: " << INSTANCE_TYPE_NAME);

		Gui::getInstance().eventFrameStart -= newDelegate(this, &InputManager::frameEntered);
		WidgetManager::getInstance().unregisterUnlinker(this);

		MYGUI_LOG(Info, INSTANCE_TYPE_NAME << " successfully shutdown");
		mIsInitialise = false;
	}

	bool InputManager::injectMouseMove(int _absx, int _absy, int _absz)
	{
		// запоминаем позицию
		mMousePosition.set(_absx, _absy);

		// вычисляем прирост по колеса
		int relz = _absz - mOldAbsZ;
		mOldAbsZ = _absz;

		// проверка на скролл
		if (relz != 0)
		{
			bool isFocus = isFocusMouse();
			if (mWidgetMouseFocus != nullptr) mWidgetMouseFocus->onMouseWheel(relz);
			return isFocus;
		}

		if (mIsWidgetMouseCapture)
		{
			if (mWidgetMouseFocus != nullptr)
			{
				if (mLayerMouseFocus != nullptr)
				{
					IntPoint point = mLayerMouseFocus->getPosition(_absx, _absy);
					mWidgetMouseFocus->onMouseDrag(point.left, point.top);
				}
			}
			else
				mIsWidgetMouseCapture = false;
			return true;
		}

		Widget* old_mouse_focus = mWidgetMouseFocus;

		// ищем активное окно
		Widget* item = LayerManager::getInstance().getWidgetFromPoint(_absx, _absy);

		// ничего не изменилось
		if (mWidgetMouseFocus == item)
		{
			bool isFocus = isFocusMouse();
			if (mWidgetMouseFocus != nullptr)
			{
				if (mLayerMouseFocus != nullptr)
				{
					IntPoint point = mLayerMouseFocus->getPosition(_absx, _absy);
					mWidgetMouseFocus->onMouseMove(_absx, _absy);
				}
			}
			return isFocus;
		}

		if (item)
		{
			// поднимаемся до рута
			Widget* root = item;
			while (root->getParent()) root = root->getParent();

			// проверяем на модальность
			if (!mVectorModalRootWidget.empty())
			{
				if (root != mVectorModalRootWidget.back())
				{
					item = nullptr;
				}
			}

			if (item != nullptr)
			{
				mLayerMouseFocus = root->getLayer();
			}
		}

		// в методе может пропасть наш виджет
		WidgetManager::getInstance().addWidgetToUnlink(item);


		//-------------------------------------------------------------------------------------//
		// новый вид рутового фокуса мыши
		Widget* save_widget = nullptr;

		// спускаемся по новому виджету и устанавливаем рутовый фокус
		Widget* root_focus = item;
		while (root_focus != nullptr)
		{
			if (root_focus->mRootMouseActive)
			{
				save_widget = root_focus;
				break;
			}
			root_focus->mRootMouseActive = true;

			// в методе может пропасть наш виджет
			WidgetManager::getInstance().addWidgetToUnlink(root_focus);
			root_focus->onMouseChangeRootFocus(true);
			WidgetManager::getInstance().removeWidgetFromUnlink(root_focus);

			if (root_focus)
				root_focus = root_focus->getParent();
		}

		// спускаемся по старому виджету и сбрасываем фокус
		root_focus = mWidgetMouseFocus;
		while (root_focus != nullptr)
		{
			if (root_focus == save_widget)
			{
				break;
			}
			root_focus->mRootMouseActive = false;

			// в методе может пропасть наш виджет
			WidgetManager::getInstance().addWidgetToUnlink(root_focus);
			root_focus->onMouseChangeRootFocus(false);
			WidgetManager::getInstance().removeWidgetFromUnlink(root_focus);

			if (root_focus)
				root_focus = root_focus->getParent();
		}
		//-------------------------------------------------------------------------------------//

		// смена фокуса, проверяем на доступность виджета
		if ((mWidgetMouseFocus != nullptr) && (mWidgetMouseFocus->isEnabled()))
		{
			mWidgetMouseFocus->onMouseLostFocus(item);
		}

		WidgetManager::getInstance().removeWidgetFromUnlink(item);


		if ((item != nullptr) && (item->isEnabled()))
		{
			item->onMouseMove(_absx, _absy);
			item->onMouseSetFocus(mWidgetMouseFocus);
		}

		// запоминаем текущее окно
		mWidgetMouseFocus = item;

		if (old_mouse_focus != mWidgetMouseFocus)
			eventChangeMouseFocus(mWidgetMouseFocus);

		return isFocusMouse();
	}

	bool InputManager::injectMousePress(int _absx, int _absy, MouseButton _id)
	{
		Widget* old_key_focus = mWidgetKeyFocus;

		// если мы щелкнули не на гуй
		if (!isFocusMouse())
		{
			resetKeyFocusWidget();

			if (old_key_focus != mWidgetKeyFocus)
				eventChangeKeyFocus(mWidgetKeyFocus);

			return false;
		}

		// если активный элемент заблокирован
		//FIXME
		if (!mWidgetMouseFocus->isEnabled())
			return true;

		// захватываем только по левой клавише и только если виджету надо
		if (MouseButton::Left == _id)
		{
			// захват окна
			mIsWidgetMouseCapture = true;
			// запоминаем место нажатия
			if (mLayerMouseFocus != nullptr)
			{
				IntPoint point = mLayerMouseFocus->getPosition(_absx, _absy);
				mLastLeftPressed = point;
			}
		}

		// ищем вверх тот виджет который может принимать фокус
		Widget* item = mWidgetMouseFocus;
		while ((item != nullptr) && (!item->isNeedKeyFocus()))
			item = item->getParent();

		// устанавливаем перед вызовом т.к. возможно внутри ктонить поменяет фокус под себя
		setKeyFocusWidget(item);

		if (mWidgetMouseFocus != nullptr)
		{
			mWidgetMouseFocus->onMouseButtonPressed(_absx, _absy, _id);

			// после пресса может сброситься
			if (mWidgetMouseFocus)
			{
				// поднимаем виджет, надо подумать что делать если поменялся фокус клавы
				LayerManager::getInstance().upLayerItem(mWidgetMouseFocus);

				// поднимаем пикинг Overlapped окон
				Widget* pick = mWidgetMouseFocus;
				do
				{
					// если оверлаппед, то поднимаем пикинг
					if (pick->getWidgetStyle() == WidgetStyle::Overlapped)
					{
						if (pick->getParent()) pick->getParent()->_forcePeek(pick);
					}

					pick = pick->getParent();
				}
				while (pick);
			}
		}

		if (old_key_focus != mWidgetKeyFocus)
			eventChangeKeyFocus(mWidgetKeyFocus);

		return true;
	}

	bool InputManager::injectMouseRelease(int _absx, int _absy, MouseButton _id)
	{
		if (isFocusMouse())
		{
			// если активный элемент заблокирован
			if (!mWidgetMouseFocus->isEnabled())
				return true;

			mWidgetMouseFocus->onMouseButtonReleased(_absx, _absy, _id);

			if (mIsWidgetMouseCapture)
			{
				// сбрасываем захват
				mIsWidgetMouseCapture = false;

				// после вызова, виджет может быть сброшен
				if (nullptr != mWidgetMouseFocus)
				{
					if ((MouseButton::Left == _id) && mTimer.getMilliseconds() < INPUT_TIME_DOUBLE_CLICK)
					{
						mWidgetMouseFocus->onMouseButtonClick();
						// после вызова, виджет может быть сброшен
						if (nullptr != mWidgetMouseFocus) mWidgetMouseFocus->onMouseButtonDoubleClick();
					}
					else
					{
						// проверяем над тем ли мы окном сейчас что и были при нажатии
						Widget* item = LayerManager::getInstance().getWidgetFromPoint(_absx, _absy);
						if ( item == mWidgetMouseFocus)
						{
							mWidgetMouseFocus->onMouseButtonClick();
						}
						mTimer.reset();
					}
				}
			}

			// для корректного отображения
			injectMouseMove(_absx, _absy, mOldAbsZ);

			return true;
		}

		return false;
	}

	bool InputManager::injectKeyPress(KeyCode _key, Char _text)
	{
		// проверка на переключение языков
		firstEncoding(_key, true);

		// запоминаем клавишу
		storeKey(_key, _text);

		bool wasFocusKey = isFocusKey();

		//Pass keystrokes to the current active text widget
		if (isFocusKey())
		{
			mWidgetKeyFocus->onKeyButtonPressed(_key, _text);
		}

		return wasFocusKey;
	}

	bool InputManager::injectKeyRelease(KeyCode _key)
	{
		// проверка на переключение языков
		firstEncoding(_key, false);

		// сбрасываем клавишу
		resetKey();

		bool wasFocusKey = isFocusKey();

		if (isFocusKey()) mWidgetKeyFocus->onKeyButtonReleased(_key);

		return wasFocusKey;
	}

	void InputManager::firstEncoding(KeyCode _key, bool bIsKeyPressed)
	{
		if ((_key == KeyCode::LeftShift) || (_key == KeyCode::RightShift))
			mIsShiftPressed = bIsKeyPressed;
		if ((_key == KeyCode::LeftControl) || (_key == KeyCode::RightControl))
			mIsControlPressed = bIsKeyPressed;
	}

	void InputManager::setKeyFocusWidget(Widget* _widget)
	{
		if (_widget == mWidgetKeyFocus)
			return;

		//-------------------------------------------------------------------------------------//
		// новый вид рутового фокуса
		Widget* save_widget = nullptr;

		// спускаемся по новому виджету и устанавливаем рутовый фокус
		Widget* root_focus = _widget;
		while (root_focus != nullptr)
		{
			if (root_focus->mRootKeyActive)
			{
				save_widget = root_focus;
				break;
			}
			root_focus->mRootKeyActive = true;

			// в методе может пропасть наш виджет
			WidgetManager::getInstance().addWidgetToUnlink(root_focus);
			root_focus->onKeyChangeRootFocus(true);
			WidgetManager::getInstance().removeWidgetFromUnlink(root_focus);

			if (root_focus)
				root_focus = root_focus->getParent();
		}

		// спускаемся по старому виджету и сбрасываем фокус
		root_focus = mWidgetKeyFocus;
		while (root_focus != nullptr)
		{
			if (root_focus == save_widget)
			{
				break;
			}
			root_focus->mRootKeyActive = false;

			// в методе может пропасть наш виджет
			WidgetManager::getInstance().addWidgetToUnlink(root_focus);
			root_focus->onKeyChangeRootFocus(false);
			WidgetManager::getInstance().removeWidgetFromUnlink(root_focus);

			if (root_focus)
				root_focus = root_focus->getParent();
		}
		//-------------------------------------------------------------------------------------//

		// сбрасываем старый
		if (mWidgetKeyFocus)
		{
			mWidgetKeyFocus->onKeyLostFocus(_widget);
		}

		// устанавливаем новый
		if (_widget && _widget->isNeedKeyFocus())
		{
			_widget->onKeySetFocus(mWidgetKeyFocus);
		}

		mWidgetKeyFocus = _widget;
	}

	void InputManager::resetMouseFocusWidget()
	{
		// спускаемся по старому виджету и сбрасываем фокус
		Widget* root_focus = mWidgetMouseFocus;
		while (root_focus != nullptr)
		{
			root_focus->mRootMouseActive = false;

			// в методе может пропасть наш виджет
			WidgetManager::getInstance().addWidgetToUnlink(root_focus);
			root_focus->onMouseChangeRootFocus(false);
			WidgetManager::getInstance().removeWidgetFromUnlink(root_focus);

			if (root_focus)
				root_focus = root_focus->getParent();
		}

		mIsWidgetMouseCapture = false;
		if (nullptr != mWidgetMouseFocus)
		{
			mWidgetMouseFocus->onMouseLostFocus(nullptr);
			mWidgetMouseFocus = nullptr;
		}

	}

	// удаляем данный виджет из всех возможных мест
	void InputManager::_unlinkWidget(Widget* _widget)
	{
		if (nullptr == _widget) return;
		if (_widget == mWidgetMouseFocus)
		{
			mIsWidgetMouseCapture = false;
			mWidgetMouseFocus = nullptr;
		}
		if (_widget == mWidgetKeyFocus)
		{
			mWidgetKeyFocus = nullptr;
		}

		// ручками сбрасываем, чтобы не менять фокусы
		for (VectorWidgetPtr::iterator iter=mVectorModalRootWidget.begin(); iter!=mVectorModalRootWidget.end(); ++iter)
		{
			if ((*iter == _widget))
			{
				mVectorModalRootWidget.erase(iter);
				break;
			}
		}

	}

	void InputManager::addWidgetModal(Widget* _widget)
	{
		if (nullptr == _widget) return;
		MYGUI_ASSERT(nullptr == _widget->getParent(), "Modal widget must be root");

		resetMouseFocusWidget();
		removeWidgetModal(_widget);
		mVectorModalRootWidget.push_back(_widget);

		setKeyFocusWidget(_widget);
		LayerManager::getInstance().upLayerItem(_widget);
	}

	void InputManager::removeWidgetModal(Widget* _widget)
	{
		resetKeyFocusWidget(_widget);
		resetMouseFocusWidget();

		for (VectorWidgetPtr::iterator iter=mVectorModalRootWidget.begin(); iter!=mVectorModalRootWidget.end(); ++iter)
		{
			if ((*iter == _widget))
			{
				mVectorModalRootWidget.erase(iter);
				break;
			}
		}
		// если еще есть модальные то их фокусируем и поднимаем
		if (!mVectorModalRootWidget.empty())
		{
			setKeyFocusWidget(mVectorModalRootWidget.back());
			LayerManager::getInstance().upLayerItem(mVectorModalRootWidget.back());
		}
	}

	void InputManager::storeKey(KeyCode _key, Char _text)
	{
		mHoldKey = KeyCode::None;
		mHoldChar = 0;

		if ( !isFocusKey() ) return;
		if ( (_key == KeyCode::LeftShift) || (_key == KeyCode::RightShift)
			|| (_key == KeyCode::LeftControl) || (_key == KeyCode::RightControl)
			|| (_key == KeyCode::LeftAlt) || (_key == KeyCode::RightAlt)
			) return;

		mFirstPressKey = true;
		mHoldKey = _key;
		mHoldChar = _text;
		mTimerKey = 0.0f;
	}

	void InputManager::resetKey()
	{
		mHoldKey = KeyCode::None;
		mHoldChar = 0;
	}

	void InputManager::frameEntered(float _frame)
	{
		if ( mHoldKey == KeyCode::None)
			return;

		if ( !isFocusKey() )
		{
			mHoldKey = KeyCode::None;
			mHoldChar = 0;
			return;
		}

		mTimerKey += _frame;

		if (mFirstPressKey)
		{
			if (mTimerKey > INPUT_DELAY_FIRST_KEY)
			{
				mFirstPressKey = false;
				mTimerKey = 0.0f;
			}
		}
		else
		{
			if (mTimerKey > INPUT_INTERVAL_KEY)
			{
				while (mTimerKey > INPUT_INTERVAL_KEY) mTimerKey -= INPUT_INTERVAL_KEY;
				mWidgetKeyFocus->onKeyButtonPressed(mHoldKey, mHoldChar);
				// focus can be dropped in onKeyButtonPressed
				if ( isFocusKey() ) mWidgetKeyFocus->onKeyButtonReleased(mHoldKey);
			}
		}

	}

	void InputManager::resetKeyFocusWidget(Widget* _widget)
	{
		if (mWidgetKeyFocus == _widget)
			setKeyFocusWidget(nullptr);
	}

	IntPoint InputManager::getMousePositionByLayer()
	{
		if (mLayerMouseFocus != nullptr)
			return mLayerMouseFocus->getPosition(mMousePosition.left, mMousePosition.top);
		return mMousePosition;
	}

} // namespace MyGUI
