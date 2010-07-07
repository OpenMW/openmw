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
#include "MyGUI_Gui.h"
#include "MyGUI_Edit.h"
#include "MyGUI_ResourceSkin.h"
#include "MyGUI_SkinManager.h"
#include "MyGUI_InputManager.h"
#include "MyGUI_ClipboardManager.h"
#include "MyGUI_PointerManager.h"
#include "MyGUI_ISubWidgetText.h"
#include "MyGUI_VScroll.h"
#include "MyGUI_HScroll.h"

namespace MyGUI
{

	const float EDIT_CURSOR_TIMER  = 0.7f;
	const float EDIT_ACTION_MOUSE_TIMER  = 0.05f;
	const int EDIT_CURSOR_MAX_POSITION = 100000;
	const int EDIT_CURSOR_MIN_POSITION = -100000;
	const size_t EDIT_MAX_UNDO = 128;
	const size_t EDIT_DEFAULT_MAX_TEXT_LENGTH = 2048;
	const float EDIT_OFFSET_HORZ_CURSOR = 10.0f; // дополнительное смещение для курсора
	const int EDIT_ACTION_MOUSE_ZONE = 1500; // область для восприятия мыши за пределом эдита
	const std::string EDIT_CLIPBOARD_TYPE_TEXT = "Text";
	const int EDIT_MOUSE_WHEEL = 50; // область для восприятия мыши за пределом эдита

	Edit::Edit() :
		mIsPressed(false),
		mIsFocus(false),
		mCursorActive(false),
		mCursorTimer(0),
		mActionMouseTimer(0),
		mCursorPosition(0),
		mTextLength(0),
		mStartSelect(ITEM_NONE),
		mEndSelect(0),
		mMouseLeftPressed(false),
		mModeReadOnly(false),
		mModePassword(false),
		mModeMultiline(false),
		mModeStatic(false),
		mModeWordWrap(false),
		mTabPrinting(false),
		mCharPassword('*'),
		mOverflowToTheLeft(false),
		mMaxTextLength(EDIT_DEFAULT_MAX_TEXT_LENGTH)
	{
		mChangeContentByResize = true;
	}

	void Edit::_initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name)
	{
		Base::_initialise(_style, _coord, _align, _info, _parent, _croppedParent, _creator, _name);

		initialiseWidgetSkin(_info);
	}

	Edit::~Edit()
	{
		shutdownWidgetSkin();
	}

	void Edit::baseChangeWidgetSkin(ResourceSkin* _info)
	{
		shutdownWidgetSkin();
		Base::baseChangeWidgetSkin(_info);
		initialiseWidgetSkin(_info);
	}

	void Edit::initialiseWidgetSkin(ResourceSkin* _info)
	{
		mOriginalPointer = mPointer;

		// нам нужен фокус клавы
		mNeedKeyFocus = true;

		for (VectorWidgetPtr::iterator iter=mWidgetChildSkin.begin(); iter!=mWidgetChildSkin.end(); ++iter)
		{
			if (*(*iter)->_getInternalData<std::string>() == "Client")
			{
				MYGUI_DEBUG_ASSERT( ! mWidgetClient, "widget already assigned");
				mWidgetClient = (*iter);
				mWidgetClient->eventMouseSetFocus = newDelegate(this, &Edit::notifyMouseSetFocus);
				mWidgetClient->eventMouseLostFocus = newDelegate(this, &Edit::notifyMouseLostFocus);
				mWidgetClient->eventMouseButtonPressed = newDelegate(this, &Edit::notifyMousePressed);
				mWidgetClient->eventMouseButtonReleased = newDelegate(this, &Edit::notifyMouseReleased);
				mWidgetClient->eventMouseDrag = newDelegate(this, &Edit::notifyMouseDrag);
				mWidgetClient->eventMouseButtonDoubleClick = newDelegate(this, &Edit::notifyMouseButtonDoubleClick);
				mWidgetClient->eventMouseWheel = newDelegate(this, &Edit::notifyMouseWheel);
				mClient = mWidgetClient;
			}
			else if (*(*iter)->_getInternalData<std::string>() == "VScroll")
			{
				MYGUI_DEBUG_ASSERT( ! mVScroll, "widget already assigned");
				mVScroll = (*iter)->castType<VScroll>();
				mVScroll->eventScrollChangePosition = newDelegate(this, &Edit::notifyScrollChangePosition);
			}
			else if (*(*iter)->_getInternalData<std::string>() == "HScroll")
			{
				MYGUI_DEBUG_ASSERT( ! mHScroll, "widget already assigned");
				mHScroll = (*iter)->castType<HScroll>();
				mHScroll->eventScrollChangePosition = newDelegate(this, &Edit::notifyScrollChangePosition);
			}
		}

		//MYGUI_ASSERT(nullptr != mWidgetClient, "Child Widget Client not found in skin (Edit must have Client)");

		if (mWidgetClient != nullptr)
		{
			ISubWidgetText* text = mWidgetClient->getSubWidgetText();
			if (text) mText = text;
		}

		//MYGUI_ASSERT(nullptr != mText, "TextEdit not found in skin (Edit or Client must have TextEdit)");

		// парсим свойства
		const MapString& properties = _info->getProperties();
		if (!properties.empty())
		{
			MapString::const_iterator iter = properties.end();
			if ((iter = properties.find("WordWrap")) != properties.end()) setEditWordWrap(utility::parseValue<bool>(iter->second));
			else if ((iter = properties.find("InvertSelected")) != properties.end()) setInvertSelected(utility::parseValue<bool>(iter->second));
		}

		updateScrollSize();

		// первоначальная инициализация курсора
		if (mText != nullptr)
			mText->setCursorPosition(mCursorPosition);
		updateSelectText();
	}

	void Edit::shutdownWidgetSkin()
	{
		mWidgetClient = nullptr;
		mVScroll= nullptr;
		mHScroll = nullptr;
	}

	void Edit::notifyMouseSetFocus(Widget* _sender, Widget* _old)
	{
		if ( (_old == mWidgetClient) || (mIsFocus) ) return;
		mIsFocus = true;
		updateEditState();
	}

	void Edit::notifyMouseLostFocus(Widget* _sender, Widget* _new)
	{
		if ( (_new == mWidgetClient) || (!mIsFocus) ) return;
		mIsFocus = false;
		updateEditState();
	}

	void Edit::notifyMousePressed(Widget* _sender, int _left, int _top, MouseButton _id)
	{
		if (mText == nullptr)
			return;

		// в статике все недоступно
		if (mModeStatic)
			return;

		IntPoint point = InputManager::getInstance().getLastLeftPressed();
		mCursorPosition = mText->getCursorPosition(point);
		mText->setCursorPosition(mCursorPosition);
		mText->setVisibleCursor(true);
		mCursorTimer = 0;
		updateSelectText();

		if (_id == MouseButton::Left) mMouseLeftPressed = true;
	}

	void Edit::notifyMouseReleased(Widget* _sender, int _left, int _top, MouseButton _id)
	{
		// сбрасываем всегда
		mMouseLeftPressed = false;
	}

	void Edit::notifyMouseDrag(Widget* _sender, int _left, int _top)
	{
		if (mText == nullptr)
			return;

		// в статике все недоступно
		if (mModeStatic) return;

		// останавливаем курсор
		mText->setVisibleCursor(true);

		// сбрасываем все таймеры
		mCursorTimer = 0;
		mActionMouseTimer = 0;

		size_t Old = mCursorPosition;
		IntPoint point(_left, _top);
		mCursorPosition = mText->getCursorPosition(point);
		if (Old == mCursorPosition) return;

		mText->setCursorPosition(mCursorPosition);

		// если не было выделения
		if (mStartSelect == ITEM_NONE) mStartSelect = Old;

		// меняем выделение
		mEndSelect = (size_t)mCursorPosition;
		if (mStartSelect > mEndSelect) mText->setTextSelection(mEndSelect, mStartSelect);
		else mText->setTextSelection(mStartSelect, mEndSelect);

	}

	void Edit::notifyMouseButtonDoubleClick(Widget* _sender)
	{
		if (mText == nullptr)
			return;

		// в статике все недоступно
		if (mModeStatic)
			return;

		const IntPoint& lastPressed = InputManager::getInstance().getLastLeftPressed();

		size_t cursorPosition = mText->getCursorPosition(lastPressed);
		mStartSelect = cursorPosition;
		mEndSelect = cursorPosition;

		UString text = this->getOnlyText();
		UString::reverse_iterator iterBack = text.rend() - cursorPosition;
		UString::iterator iterForw = text.begin() + cursorPosition;

		while (iterBack != text.rend())
		{
			if (((*iterBack)<265) && (ispunct(*iterBack) || isspace(*iterBack))) break;
			iterBack++;
			mStartSelect--;
		}
		while (iterForw != text.end())
		{
			if (((*iterForw)<265) && (ispunct(*iterForw) || isspace(*iterForw))) break;
			iterForw++;
			mEndSelect++;
		}

		mText->setCursorPosition(mEndSelect);
		mText->setTextSelection(mStartSelect, mEndSelect);
	}

	void Edit::onMouseDrag(int _left, int _top)
	{
		notifyMouseDrag(nullptr, _left, _top);

		Base::onMouseDrag(_left, _top);
	}

	void Edit::onKeySetFocus(Widget* _old)
	{
		if (!mIsPressed)
		{
			mIsPressed = true;
			updateEditState();

			if (!mModeStatic)
			{
				if (mText != nullptr)
				{
					mCursorActive = true;
					Gui::getInstance().eventFrameStart += newDelegate(this, &Edit::frameEntered);
					mText->setVisibleCursor(true);
					mText->setSelectBackground(true);
					mCursorTimer = 0;
				}
			}
		}

		Base::onKeySetFocus(_old);
	}

	void Edit::onKeyLostFocus(Widget* _new)
	{
		if (mIsPressed)
		{
			mIsPressed = false;
			updateEditState();

			if (mText != nullptr)
			{
				mCursorActive = false;
				Gui::getInstance().eventFrameStart -= newDelegate(this, &Edit::frameEntered);
				mText->setVisibleCursor(false);
				mText->setSelectBackground(false);
			}
		}

		Base::onKeyLostFocus(_new);
	}

	void Edit::onKeyButtonPressed(KeyCode _key, Char _char)
	{
		if (mText == nullptr || mWidgetClient == nullptr)
		{
			Base::onKeyButtonPressed(_key, _char);
			return;
		}

		// в статическом режиме ничего не доступно
		if (mModeStatic)
		{
			Base::onKeyButtonPressed(_key, _char);
			return;
		}

		InputManager& input = InputManager::getInstance();

		mText->setVisibleCursor(true);
		mCursorTimer = 0.0f;

		if (_key == KeyCode::Escape)
		{
			InputManager::getInstance().setKeyFocusWidget(nullptr);
		}
		else if (_key == KeyCode::Backspace)
		{
			// если нуно то удаляем выделенный текст
			if (!mModeReadOnly)
			{
				if (!deleteTextSelect(true))
				{
					// прыгаем на одну назад и удаляем
					if (mCursorPosition != 0)
					{
						mCursorPosition--;
						eraseText(mCursorPosition, 1, true);
					}
				}
				// отсылаем событие о изменении
				eventEditTextChange(this);
			}

		}
		else if (_key == KeyCode::Delete)
		{
			if (input.isShiftPressed()) commandCut();
			else if (!mModeReadOnly)
			{
				// если нуно то удаляем выделенный текст
				if (!deleteTextSelect(true))
				{
					if (mCursorPosition != mTextLength)
					{
						eraseText(mCursorPosition, 1, true);
					}
				}
				// отсылаем событие о изменении
				eventEditTextChange(this);
			}

		}
		else if (_key == KeyCode::Insert)
		{
			if (input.isShiftPressed()) commandPast();
			else if (input.isControlPressed()) commandCopy();

		}
		else if ((_key == KeyCode::Return) || (_key == KeyCode::NumpadEnter))
		{
			// работаем только в режиме редактирования
			if (!mModeReadOnly)
			{
				if ((mModeMultiline) && (!input.isControlPressed()))
				{
					// попытка объединения двух комманд
					size_t size = mVectorUndoChangeInfo.size();
					// непосредственно операции
					deleteTextSelect(true);
					insertText(TextIterator::getTextNewLine(), mCursorPosition, true);
					// проверяем на возможность объединения
					if ((size+2) == mVectorUndoChangeInfo.size()) commandMerge();
					// отсылаем событие о изменении
					eventEditTextChange(this);
				}
				// при сингл лайн и и мульти+сонтрол шлем эвент
				else
				{
					eventEditSelectAccept(this);
				}
			}

		}
		else if (_key == KeyCode::ArrowRight)
		{
			if ((mCursorPosition) < mTextLength)
			{
				mCursorPosition ++;
				mText->setCursorPosition(mCursorPosition);
				updateSelectText();
			}
			// сбрасываем выделение
			else if (isTextSelection() && !input.isShiftPressed())
			{
				resetSelect();
			}

		}
		else if (_key == KeyCode::ArrowLeft)
		{
			if (mCursorPosition != 0)
			{
				mCursorPosition --;
				mText->setCursorPosition(mCursorPosition);
				updateSelectText();
			}
			// сбрасываем выделение
			else if (isTextSelection() && !input.isShiftPressed())
			{
				resetSelect();
			}

		}
		else if (_key == KeyCode::ArrowUp)
		{
			IntPoint point = mText->getCursorPoint(mCursorPosition);
			point.top -= mText->getFontHeight();
			size_t old = mCursorPosition;
			mCursorPosition = mText->getCursorPosition(point);
			// самая верхняя строчка
			if ( old == mCursorPosition )
			{
				if (mCursorPosition != 0)
				{
					mCursorPosition = 0;
					mText->setCursorPosition(mCursorPosition);
					updateSelectText();
				}
				// сбрасываем выделение
				else if (isTextSelection() && !input.isShiftPressed())
				{
					resetSelect();
				}
			}
			else
			{
				mText->setCursorPosition(mCursorPosition);
				updateSelectText();
			}

		}
		else if (_key == KeyCode::ArrowDown)
		{
			IntPoint point = mText->getCursorPoint(mCursorPosition);
			point.top += mText->getFontHeight();
			size_t old = mCursorPosition;
			mCursorPosition = mText->getCursorPosition(point);
			// самая нижняя строчка
			if ( old == mCursorPosition )
			{
				if (mCursorPosition != mTextLength)
				{
					mCursorPosition = mTextLength;
					mText->setCursorPosition(mCursorPosition);
					updateSelectText();
				}
				// сбрасываем выделение
				else if (isTextSelection() && !input.isShiftPressed())
				{
					resetSelect();
				}
			}
			else
			{
				mText->setCursorPosition(mCursorPosition);
				updateSelectText();
			}

		}
		else if (_key == KeyCode::Home)
		{
			// в начало строки
			if ( !input.isControlPressed())
			{
				IntPoint point = mText->getCursorPoint(mCursorPosition);
				point.left = EDIT_CURSOR_MIN_POSITION;
				size_t old = mCursorPosition;
				mCursorPosition = mText->getCursorPosition(point);
				if ( old != mCursorPosition )
				{
					mText->setCursorPosition(mCursorPosition);
					updateSelectText();
				}
				else if (isTextSelection() && !input.isShiftPressed())
				{
					resetSelect();
				}
			}
			// в начало всего текста
			else
			{
				if (0 != mCursorPosition)
				{
					mCursorPosition = 0;
					mText->setCursorPosition(mCursorPosition);
					updateSelectText();
				}
				else if (isTextSelection() && !input.isShiftPressed())
				{
					resetSelect();
				}
			}

		}
		else if (_key == KeyCode::End)
		{
			// в конец строки
			if ( !  input.isControlPressed())
			{
				IntPoint point = mText->getCursorPoint(mCursorPosition);
				point.left = EDIT_CURSOR_MAX_POSITION;
				size_t old = mCursorPosition;
				mCursorPosition = mText->getCursorPosition(point);
				if ( old != mCursorPosition )
				{
					mText->setCursorPosition(mCursorPosition);
					updateSelectText();
				}
				else if (isTextSelection() && !input.isShiftPressed())
				{
					resetSelect();
				}
			}
			// в самый конец
			else
			{
				if (mTextLength != mCursorPosition)
				{
					mCursorPosition = mTextLength;
					mText->setCursorPosition(mCursorPosition);
					updateSelectText();
				}
				else if (isTextSelection() && !input.isShiftPressed())
				{
					resetSelect();
				}
			}

		}
		else if (_key == KeyCode::PageUp)
		{
			// на размер окна, но не меньше одной строки
			IntPoint point = mText->getCursorPoint(mCursorPosition);
			point.top -= (mWidgetClient->getHeight() > mText->getFontHeight()) ? mWidgetClient->getHeight() : mText->getFontHeight();
			size_t old = mCursorPosition;
			mCursorPosition = mText->getCursorPosition(point);
			// самая верхняя строчка
			if ( old == mCursorPosition )
			{
				if (mCursorPosition != 0)
				{
					mCursorPosition = 0;
					mText->setCursorPosition(mCursorPosition);
					updateSelectText();
				}
				// сбрасываем выделение
				else if (isTextSelection() && !input.isShiftPressed())
				{
					resetSelect();
				}
			}
			else
			{
				mText->setCursorPosition(mCursorPosition);
				updateSelectText();
			}

		}
		else if (_key == KeyCode::PageDown)
		{
			// на размер окна, но не меньше одной строки
			IntPoint point = mText->getCursorPoint(mCursorPosition);
			point.top += (mWidgetClient->getHeight() > mText->getFontHeight()) ? mWidgetClient->getHeight() : mText->getFontHeight();
			size_t old = mCursorPosition;
			mCursorPosition = mText->getCursorPosition(point);
			// самая нижняя строчка
			if ( old == mCursorPosition )
			{
				if (mCursorPosition != mTextLength)
				{
					mCursorPosition = mTextLength;
					mText->setCursorPosition(mCursorPosition);
					updateSelectText();
				}
				// сбрасываем выделение
				else if (isTextSelection() && !input.isShiftPressed())
				{
					resetSelect();
				}
			}
			else
			{
				mText->setCursorPosition(mCursorPosition);
				updateSelectText();
			}

		}
		else if ((_key == KeyCode::LeftShift) || (_key == KeyCode::RightShift))
		{
			// для правильно выделения
			if (mStartSelect == ITEM_NONE)
			{
				mStartSelect = mEndSelect = mCursorPosition;
			}
		}
		else if (_char != 0)
		{

			// если не нажат контрл, то обрабатываем как текст
			if (!input.isControlPressed())
			{
				if (!mModeReadOnly)
				{
					// таб только если нужно
					if (_char != '\t' || mTabPrinting)
					{
						// попытка объединения двух комманд
						size_t size = mVectorUndoChangeInfo.size();
						// непосредственно операции
						deleteTextSelect(true);
						insertText(TextIterator::getTextCharInfo(_char), mCursorPosition, true);
						// проверяем на возможность объединения
						if ((size+2) == mVectorUndoChangeInfo.size()) commandMerge();
						// отсылаем событие о изменении
						eventEditTextChange(this);
					}
				}
			}
			else if (_key == KeyCode::C)
			{
				commandCopy();

			}
			else if (_key == KeyCode::X)
			{
				commandCut();

			}
			else if (_key == KeyCode::V)
			{
				commandPast();

			}
			else if (_key == KeyCode::A)
			{
				// выделяем весь текст
				setTextSelection(0, mTextLength);

			}
			else if (_key == KeyCode::Z)
			{
				// отмена
				commandUndo();

			}
			else if (_key == KeyCode::Y)
			{
				// повтор
				commandRedo();

			}
		}

		Base::onKeyButtonPressed(_key, _char);
	}

	void Edit::frameEntered(float _frame)
	{
		if (mText == nullptr)
			return;

		// в статике все недоступно
		if (mModeStatic)
			return;

		if (mCursorActive)
		{
			mCursorTimer += _frame;

			if (mCursorTimer > EDIT_CURSOR_TIMER)
			{
				mText->setVisibleCursor(!mText->isVisibleCursor());
				while (mCursorTimer > EDIT_CURSOR_TIMER) mCursorTimer -= EDIT_CURSOR_TIMER;
			}
		}

		// сдвигаем курсор по положению мыши
		if (mMouseLeftPressed)
		{
			mActionMouseTimer += _frame;

			if (mActionMouseTimer > EDIT_ACTION_MOUSE_TIMER)
			{

				IntPoint mouse = InputManager::getInstance().getMousePositionByLayer();
				const IntRect& view = mWidgetClient->getAbsoluteRect();
				mouse.left -= view.left;
				mouse.top -= view.top;
				IntPoint point;

				bool action = false;

				// вверх на одну строчку
				if ( (mouse.top < 0) && (mouse.top > -EDIT_ACTION_MOUSE_ZONE) )
				{
					if ( (mouse.left > 0) && (mouse.left <= mWidgetClient->getWidth()) )
					{
						point = mText->getCursorPoint(mCursorPosition);
						point.top -= mText->getFontHeight();
						action = true;
					}
				}
				// вниз на одну строчку
				else if ( (mouse.top > mWidgetClient->getHeight()) && (mouse.top < (mWidgetClient->getHeight() + EDIT_ACTION_MOUSE_ZONE)) )
				{
					if ( (mouse.left > 0) && (mouse.left <= mWidgetClient->getWidth()) )
					{
						point = mText->getCursorPoint(mCursorPosition);
						point.top += mText->getFontHeight();
						action = true;
					}
				}

				// влево на небольшое расстояние
				if ( (mouse.left < 0) && (mouse.left > -EDIT_ACTION_MOUSE_ZONE) )
				{
					point = mText->getCursorPoint(mCursorPosition);
					point.left -= (int)EDIT_OFFSET_HORZ_CURSOR;
					action = true;
				}
				// вправо на небольшое расстояние
				else if ( (mouse.left > mWidgetClient->getWidth()) && (mouse.left < (mWidgetClient->getWidth() + EDIT_ACTION_MOUSE_ZONE)) )
				{
					point = mText->getCursorPoint(mCursorPosition);
					point.left += (int)EDIT_OFFSET_HORZ_CURSOR;
					action = true;
				}

				if (action)
				{
					size_t old = mCursorPosition;
					mCursorPosition = mText->getCursorPosition(point);

					if ( old != mCursorPosition )
					{

						mText->setCursorPosition(mCursorPosition);

						mEndSelect = (size_t)mCursorPosition;
						if (mStartSelect > mEndSelect) mText->setTextSelection(mEndSelect, mStartSelect);
						else mText->setTextSelection(mStartSelect, mEndSelect);

						// пытаемся показать курсор
						updateViewWithCursor();
					}

				}
				// если в зону не попадает то сбрасываем
				else mActionMouseTimer = 0;

				while (mActionMouseTimer > EDIT_ACTION_MOUSE_TIMER) mActionMouseTimer -= EDIT_ACTION_MOUSE_TIMER;
			}

		} // if (mMouseLeftPressed)
	}

	void Edit::setTextCursor(size_t _index)
	{
		// сбрасываем выделение
		resetSelect();

		// новая позиция
		if (_index > mTextLength) _index = mTextLength;
		if (mCursorPosition == _index) return;
		mCursorPosition = _index;

		// обновляем по позиции
		if (mText != nullptr)
			mText->setCursorPosition(mCursorPosition);
		updateSelectText();
	}

	void Edit::setTextSelection(size_t _start, size_t _end)
	{
		if (_start > mTextLength) _start = mTextLength;
		if (_end > mTextLength) _end = mTextLength;

		mStartSelect = _start;
		mEndSelect = _end;

		if (mText != nullptr)
		{
			if (mStartSelect > mEndSelect) mText->setTextSelection(mEndSelect, mStartSelect);
			else mText->setTextSelection(mStartSelect, mEndSelect);
		}

		if (mCursorPosition == mEndSelect) return;
		// курсор на конец выделения
		mCursorPosition = mEndSelect;

		// обновляем по позиции
		if (mText != nullptr)
			mText->setCursorPosition(mCursorPosition);
	}

	bool Edit::deleteTextSelect(bool _history)
	{
		if ( ! isTextSelection()) return false;

		// начало и конец выделения
		size_t start = getTextSelectionStart();
		size_t end =  getTextSelectionEnd();

		eraseText(start, end - start, _history);

		return true;
	}

	void Edit::resetSelect()
	{
		if (mStartSelect != ITEM_NONE)
		{
			mStartSelect = ITEM_NONE;
			if (mText != nullptr)
				mText->setTextSelection(0, 0);
		}
	}

	void Edit::commandPosition(size_t _undo, size_t _redo, size_t _length, VectorChangeInfo * _info)
	{
		if (_info != nullptr) _info->push_back(TextCommandInfo(_undo, _redo, _length));
	}

	void Edit::commandMerge()
	{
		if (mVectorUndoChangeInfo.size() < 2) return; // на всякий
		// сохраняем последние набор отмен
		VectorChangeInfo info = mVectorUndoChangeInfo.back();
		mVectorUndoChangeInfo.pop_back();

		// объединяем последовательности
		for (VectorChangeInfo::iterator iter=info.begin(); iter!=info.end(); ++iter)
		{
			mVectorUndoChangeInfo.back().push_back((*iter));
		}
	}

	bool Edit::commandUndo()
	{
		if (mVectorUndoChangeInfo.empty()) return false;

		// сбрасываем выделение
		resetSelect();

		// сохраняем последние набор отмен
		VectorChangeInfo info = mVectorUndoChangeInfo.back();
		// перекидываем последний набор отмен
		mVectorUndoChangeInfo.pop_back();
		mVectorRedoChangeInfo.push_back(info);

		// берем текст для издевательств
		UString text = getRealString();

		// восстанавливаем последовательность
		for (VectorChangeInfo::reverse_iterator iter=info.rbegin(); iter!=info.rend(); iter++)
		{

			if ((*iter).type == TextCommandInfo::COMMAND_INSERT) text.erase((*iter).start, (*iter).text.size());
			else if ((*iter).type == TextCommandInfo::COMMAND_ERASE) text.insert((*iter).start, (*iter).text);
			else
			{
				mCursorPosition = (*iter).undo;
				mTextLength = (*iter).length;
			}
		}

		// возвращаем текст
		setRealString(text);

		// обновляем по позиции
		if (mText != nullptr)
			mText->setCursorPosition(mCursorPosition);
		updateSelectText();

		// отсылаем событие о изменении
		eventEditTextChange(this);

		return true;
	}

	bool Edit::commandRedo()
	{
		if (mVectorRedoChangeInfo.empty()) return false;

		// сбрасываем выделение
		resetSelect();

		// сохраняем последние набор отмен
		VectorChangeInfo info = mVectorRedoChangeInfo.back();
		// перекидываем последний набор отмен
		mVectorRedoChangeInfo.pop_back();
		mVectorUndoChangeInfo.push_back(info);

		// берем текст для издевательств
		UString text = getRealString();

		// восстанавливаем последовательность
		for (VectorChangeInfo::iterator iter=info.begin(); iter!=info.end(); ++iter)
		{

			if ((*iter).type == TextCommandInfo::COMMAND_INSERT) text.insert((*iter).start, (*iter).text);
			else if ((*iter).type == TextCommandInfo::COMMAND_ERASE) text.erase((*iter).start, (*iter).text.size());
			else
			{
				mCursorPosition = (*iter).redo;
				mTextLength = (*iter).length;
			}

		}

		// возвращаем текст
		setRealString(text);

		// обновляем по позиции
		if (mText != nullptr)
			mText->setCursorPosition(mCursorPosition);
		updateSelectText();

		// отсылаем событие о изменении
		eventEditTextChange(this);

		return true;
	}

	void Edit::saveInHistory(VectorChangeInfo * _info)
	{
		if (_info == nullptr) return;
		// если нет информации об изменении
		if ( _info->empty() ) return;
		if ( (_info->size() == 1) && (_info->back().type == TextCommandInfo::COMMAND_POSITION)) return;

		mVectorUndoChangeInfo.push_back(*_info);
		// проверяем на максимальный размер
		if (mVectorUndoChangeInfo.size() > EDIT_MAX_UNDO)
			mVectorUndoChangeInfo.pop_front();
	}

	// возвращает текст
	UString Edit::getTextInterval(size_t _start, size_t _count)
	{
		// подстраховка
		if (_start > mTextLength) _start = mTextLength;
		// конец диапазона
		size_t end = _start + _count;

		// итератор нашей строки
		TextIterator iterator(getRealString());

		// дефолтный цвет
		UString colour = mText == nullptr ? "" : TextIterator::convertTagColour(mText->getTextColour());

		// нужно ли вставлять цвет
		bool need_colour = true;

		// цикл прохода по строке
		while (iterator.moveNext())
		{
			// текущаяя позиция
			size_t pos = iterator.getPosition();

			// еще рано
			if (pos < _start)
			{
				// берем цвет из позиции и запоминаем
				iterator.getTagColour(colour);

				continue;
			}

			// проверяем на надобность начального тега
			else if (pos == _start)
			{
				need_colour = ! iterator.getTagColour(colour);
				// сохраняем место откуда начинается
				iterator.saveStartPoint();

			}

			// а теперь просто до конца диапазона
			else if (pos == end) break;

		}

		// возвращаем строку
		if (need_colour) return colour + iterator.getFromStart();
		return iterator.getFromStart();
	}

	// выделяет цветом диапазон
	void Edit::_setTextColour(size_t _start, size_t _count, const Colour& _colour, bool _history)
	{
		// при изменениях сразу сбрасываем повтор
		commandResetRedo();

		// история изменений
		VectorChangeInfo * history = nullptr;
		if (_history) history = new VectorChangeInfo();

		// конец диапазона
		size_t end = _start + _count;

		// итератор нашей строки
		TextIterator iterator(getRealString(), history);

		// дефолтный цвет
		UString colour = mText == nullptr ? "" : TextIterator::convertTagColour(mText->getTextColour());

		// цикл прохода по строке
		while (iterator.moveNext())
		{

			// текущаяя позиция
			size_t pos = iterator.getPosition();

			// берем цвет из позиции и запоминаем
			iterator.getTagColour(colour);

			// еще рано
			if (pos < _start) continue;

			// ставим начальный тег
			else if (pos == _start)
				iterator.setTagColour(_colour);

			// внутри диапазона очищаем все
			else if (pos < end)
				iterator.clearTagColour();

			// на конец ставим последний найденный или дефолтный
			else if (pos == end)
			{
				iterator.setTagColour(colour);
				// и выходим из цикла
				break;
			}

		}

		// сохраняем позицию для восстановления курсора
		commandPosition(_start, _start+_count, mTextLength, history);

		// запоминаем в историю
		if (_history)
		{
			saveInHistory(history);
			delete history;
		}
		// сбрасываем историю
		else commandResetHistory();

		// и возвращаем строку на место
		setRealString(iterator.getText());

	}

	void Edit::setTextSelectColour(const Colour& _colour, bool _history)
	{
		// нужно выделение
		if ( !isTextSelection()) return;
		// начало и конец выделения
		size_t start = getTextSelectionStart();
		size_t end =  getTextSelectionEnd();
		_setTextColour(start, end-start, _colour, _history);
	}

	UString Edit::getTextSelection()
	{
		if ( !isTextSelection()) return "";
		size_t start = getTextSelectionStart();
		size_t end =  getTextSelectionEnd();
		return getTextInterval(start, end-start);
	}

	void Edit::setEditPassword(bool _password)
	{
		if (mModePassword == _password) return;
		mModePassword = _password;
		if (mModePassword)
		{
			if (mText != nullptr)
			{
				mPasswordText = mText->getCaption();
				mText->setCaption(UString(mTextLength, '*'));
			}
		}
		else
		{
			if (mText != nullptr)
			{
				mText->setCaption(mPasswordText);
				mPasswordText.clear();
			}
		}
		// обновляем по размерам
		updateView();
		// сбрасываем историю
		commandResetHistory();
	}

	void Edit::setText(const UString& _caption, bool _history)
	{
		// сбрасываем выделение
		resetSelect();

		// история изменений
		VectorChangeInfo * history = nullptr;
		if (_history) history = new VectorChangeInfo();

		// итератор нашей строки
		TextIterator iterator(getRealString(), history);

		// вставляем текст
		iterator.setText(_caption, mModeMultiline || mModeWordWrap);

		if (mOverflowToTheLeft)
		{
			iterator.cutMaxLengthFromBeginning(mMaxTextLength);
		}
		else
		{
			// обрезаем по максимальной длинне
			iterator.cutMaxLength(mMaxTextLength);
		}

		// запоминаем размер строки
		size_t old = mTextLength;
		// новая позиция и положение на конец вставки
		mCursorPosition = mTextLength = iterator.getSize();

		// сохраняем позицию для восстановления курсора
		commandPosition(0, mTextLength, old, history);

		// запоминаем в историю
		if (_history)
		{
			saveInHistory(history);
			delete history;
		}
		// сбрасываем историю
		else commandResetHistory();

		// и возвращаем строку на место
		setRealString(iterator.getText());

		// обновляем по позиции
		if (mText != nullptr)
			mText->setCursorPosition(mCursorPosition);
		updateSelectText();
	}

	void Edit::insertText(const UString& _text, size_t _start, bool _history)
	{
		// сбрасываем выделение
		resetSelect();

		// если строка пустая, или размер максимален
		if (_text.empty()) return;

		if ((mOverflowToTheLeft == false) && (mTextLength == mMaxTextLength)) return;

		// история изменений
		VectorChangeInfo * history = nullptr;
		if (_history) history = new VectorChangeInfo();

		// итератор нашей строки
		TextIterator iterator(getRealString(), history);

		// дефолтный цвет
		UString colour = mText == nullptr ? "" : TextIterator::convertTagColour(mText->getTextColour());
		// нужен ли тег текста
		// потом переделать через TextIterator чтобы отвязать понятие тег от эдита
		bool need_colour = ( (_text.size() > 6) && (_text[0] == L'#') && (_text[1] != L'#') );

		// цикл прохода по строке
		while (iterator.moveNext())
		{

			// текущаяя позиция
			size_t pos = iterator.getPosition();

			// текущий цвет
			if (need_colour) iterator.getTagColour(colour);

			// если дошли то выходим
			if (pos == _start) break;

		}

		// если нужен цвет то вставляем
		if (need_colour) iterator.setTagColour(colour);

		// а теперь вставляем строку
		iterator.insertText(_text, mModeMultiline || mModeWordWrap);

		if (mOverflowToTheLeft)
		{
			iterator.cutMaxLengthFromBeginning(mMaxTextLength);
		}
		else
		{
			// обрезаем по максимальной длинне
			iterator.cutMaxLength(mMaxTextLength);
		}

		// запоминаем размер строки
		size_t old = mTextLength;
		// новая позиция и положение на конец вставки
		mTextLength = iterator.getSize();
		mCursorPosition += mTextLength - old;

		// сохраняем позицию для восстановления курсора
		commandPosition(_start, _start + mTextLength - old, old, history);

		// запоминаем в историю
		if (_history)
		{
			saveInHistory(history);
			delete history;
		}
		// сбрасываем историю
		else commandResetHistory();

		// и возвращаем строку на место
		setRealString(iterator.getText());

		// обновляем по позиции
		if (mText != nullptr)
			mText->setCursorPosition(mCursorPosition);
		updateSelectText();
	}

	void Edit::eraseText(size_t _start, size_t _count, bool _history)
	{
		// чета маловато
		if (_count == 0) return;

		// сбрасываем выделение
		resetSelect();

		// история изменений
		VectorChangeInfo * history = nullptr;
		if (_history) history = new VectorChangeInfo();

		// итератор нашей строки
		TextIterator iterator(getRealString(), history);

		// дефолтный цвет
		UString colour;
		// конец диапазона
		size_t end = _start + _count;
		bool need_colour = false;

		// цикл прохода по строке
		while (iterator.moveNext())
		{

			// текущаяя позиция
			size_t pos = iterator.getPosition();

			// еще рано
			if (pos < _start)
			{
				// берем цвет из позиции и запоминаем
				iterator.getTagColour(colour);
				continue;
			}

			// сохраняем место откуда начинается
			else if (pos == _start)
			{
				// если до диапазона был цвет, то нужно закрыть тег
				if ( ! colour.empty())
				{
					need_colour = true;
					colour.clear();
				}
				// берем цвет из позиции и запоминаем
				iterator.getTagColour(colour);
				iterator.saveStartPoint();
			}

			// внутри диапазона
			else if (pos < end)
			{
				// берем цвет из позиции и запоминаем
				iterator.getTagColour(colour);
			}

			// окончание диапазона
			else if (pos == end)
			{
				// нужно ставить тег или нет
				if ( ! colour.empty()) need_colour = true;
				if ( iterator.getTagColour(colour)) need_colour = false;

				break;
			}

		}

		// удаляем диапазон
		iterator.eraseFromStart();
		// и вставляем последний цвет
		if (need_colour) iterator.setTagColour(colour);

		// сохраняем позицию для восстановления курсора
		commandPosition(_start + _count, _start, mTextLength, history);

		// на месте удаленного
		mCursorPosition = _start;
		mTextLength -= _count;

		// запоминаем в историю
		if (_history)
		{
			saveInHistory(history);
			delete history;
		}
		// сбрасываем историю
		else commandResetHistory();

		// и возвращаем строку на место
		setRealString(iterator.getText());

		// обновляем по позиции
		if (mText != nullptr)
			mText->setCursorPosition(mCursorPosition);
		updateSelectText();
	}

	void Edit::commandCut()
	{
		// вырезаем в буфер обмена
		if ( isTextSelection() && (!mModePassword) )
		{
			ClipboardManager::getInstance().setClipboardData(EDIT_CLIPBOARD_TYPE_TEXT, getTextSelection());
			if (!mModeReadOnly)
			{
				deleteTextSelect(true);
				// отсылаем событие о изменении
				eventEditTextChange(this);
			}
		}
		else ClipboardManager::getInstance().clearClipboardData(EDIT_CLIPBOARD_TYPE_TEXT);
	}

	void Edit::commandCopy()
	{
		// копируем в буфер обмена
		if ( isTextSelection() && (!mModePassword) ) ClipboardManager::getInstance().setClipboardData(EDIT_CLIPBOARD_TYPE_TEXT, getTextSelection());
		else ClipboardManager::getInstance().clearClipboardData(EDIT_CLIPBOARD_TYPE_TEXT);
	}

	void Edit::commandPast()
	{
		// копируем из буфера обмена
		std::string clipboard = ClipboardManager::getInstance().getClipboardData(EDIT_CLIPBOARD_TYPE_TEXT);
		if ( (!mModeReadOnly) && ( !clipboard.empty()) )
		{
			// попытка объединения двух комманд
			size_t size = mVectorUndoChangeInfo.size();
			// непосредственно операции
			deleteTextSelect(true);
			insertText(clipboard, mCursorPosition, true);
			// проверяем на возможность объединения
			if ((size+2) == mVectorUndoChangeInfo.size()) commandMerge();
			// отсылаем событие о изменении
			eventEditTextChange(this);
		}
	}

	const UString& Edit::getRealString()
	{
		if (mModePassword) return mPasswordText;
		else if (mText == nullptr) return mPasswordText;

		return mText->getCaption();
	}

	void Edit::setRealString(const UString& _caption)
	{
		if (mModePassword)
		{
			mPasswordText = _caption;
			if (mText != nullptr)
				mText->setCaption(UString(mTextLength, mCharPassword));
		}
		else
		{
			if (mText != nullptr)
				mText->setCaption(_caption);
		}
	}

	void Edit::setPasswordChar(Char _char)
	{
		mCharPassword = _char;
		if (mModePassword)
		{
			if (mText != nullptr)
				mText->setCaption(UString(mTextLength, mCharPassword));
		}
	}

	void Edit::updateEditState()
	{
		if (!mEnabled) setState("disabled");
		else if (mIsPressed)
		{
			if (mIsFocus) setState("pushed");
			else setState("normal_checked");
		}
		else if (mIsFocus) setState("highlighted");
		else setState("normal");
	}

	void Edit::setPosition(const IntPoint& _point)
	{
		Base::setPosition(_point);
	}

	void Edit::eraseView()
	{
		// если перенос, то сбрасываем размер текста
		if (mModeWordWrap)
		{
			if (mText != nullptr)
				mText->setWordWrap(true);
		}

		updateView();
	}

	void Edit::setSize(const IntSize& _size)
	{
		Base::setSize(_size);

		eraseView();
	}

	void Edit::setCoord(const IntCoord& _coord)
	{
		Base::setCoord(_coord);

		eraseView();
	}

	void Edit::setCaption(const UString& _value)
	{
		setText(_value, false);
	}

	const UString& Edit::getCaption()
	{
		return getRealString();
	}

	void Edit::updateSelectText()
	{
		if (!mModeStatic)
		{

			InputManager& input = InputManager::getInstance();
			if ( (input.isShiftPressed()) && (mStartSelect != ITEM_NONE) )
			{
				// меняем выделение
				mEndSelect = (size_t)mCursorPosition;
				if (mText != nullptr)
				{
					if (mStartSelect > mEndSelect) mText->setTextSelection(mEndSelect, mStartSelect);
					else mText->setTextSelection(mStartSelect, mEndSelect);
				}

			}
			else if (mStartSelect != ITEM_NONE)
			{
				// сбрасываем шифт
				mStartSelect = ITEM_NONE;
				if (mText != nullptr)
					mText->setTextSelection(0, 0);
			}
		}

		// пытаемся показать курсор
		updateViewWithCursor();
	}

	void Edit::setTextAlign(Align _align)
	{
		Base::setTextAlign(_align);

		// так как мы сами рулим смещениями
		updateView();
	}

	void Edit::notifyScrollChangePosition(VScroll* _sender, size_t _position)
	{
		if (mText == nullptr)
			return;

		if (_sender == mVScroll)
		{
			IntPoint point = mText->getViewOffset();
			point.top = _position;
			mText->setViewOffset(point);
		}
		else if (_sender == mHScroll)
		{
			IntPoint point = mText->getViewOffset();
			point.left = _position;
			mText->setViewOffset(point);
		}
	}

	void Edit::notifyMouseWheel(Widget* _sender, int _rel)
	{
		if (mText == nullptr)
			return;

		if (mVRange != 0)
		{
			IntPoint point = mText->getViewOffset();
			int offset = point.top;
			if (_rel < 0) offset += EDIT_MOUSE_WHEEL;
			else  offset -= EDIT_MOUSE_WHEEL;

			if (offset < 0) offset = 0;
			else if (offset > (int)mVRange) offset = mVRange;

			if (offset != point.top)
			{
				point.top = offset;
				if (mVScroll != nullptr)
					mVScroll->setScrollPosition(offset);
				mText->setViewOffset(point);
			}
		}
		else if (mHRange != 0)
		{
			IntPoint point = mText->getViewOffset();
			int offset = point.left;
			if (_rel < 0) offset += EDIT_MOUSE_WHEEL;
			else  offset -= EDIT_MOUSE_WHEEL;

			if (offset < 0) offset = 0;
			else if (offset > (int)mHRange) offset = mHRange;

			if (offset != point.left)
			{
				point.left = offset;
				if (mHScroll != nullptr)
					mHScroll->setScrollPosition(offset);
				mText->setViewOffset(point);
			}
		}
	}

	void Edit::setEditWordWrap(bool _value)
	{
		mModeWordWrap = _value;
		if (mText != nullptr)
			mText->setWordWrap(mModeWordWrap);

		eraseView();
	}

	void Edit::setFontName(const std::string& _value)
	{
		Base::setFontName(_value);

		eraseView();
	}

	void Edit::setFontHeight(int _value)
	{
		Base::setFontHeight(_value);

		eraseView();
	}

	void Edit::updateView()
	{
		updateScrollSize();
		updateScrollPosition();
	}

	void Edit::updateViewWithCursor()
	{
		updateScrollSize();
		updateCursorPosition();
		updateScrollPosition();
	}

	void Edit::updateCursorPosition()
	{
		if (mText == nullptr || mWidgetClient == nullptr)
			return;

		// размер контекста текста
		IntSize textSize = mText->getTextSize();

		// текущее смещение контекста текста
		IntPoint point = mText->getViewOffset();
		// расчетное смещение
		IntPoint offset = point;

		// абсолютные координаты курсора
		IntRect cursor = mText->getCursorRect(mCursorPosition);
		cursor.right ++;

		// абсолютные координаты вью
		const IntRect& view = mWidgetClient->getAbsoluteRect();

		// проверяем и показываем курсор
		if (!view.inside(cursor))
		{
			// горизонтальное смещение
			// FIXME проверить, помоему просто >
			if (textSize.width >= view.width())
			{
				if (cursor.left < view.left)
				{
					offset.left = point.left - (view.left - cursor.left);
					// добавляем смещение, только если курсор не перепрыгнет
					if ((float(view.width()) - EDIT_OFFSET_HORZ_CURSOR) > EDIT_OFFSET_HORZ_CURSOR) offset.left -= int(EDIT_OFFSET_HORZ_CURSOR);
				}
				else if (cursor.right > view.right)
				{
					offset.left = point.left + (cursor.right - view.right);
					// добавляем смещение, только если курсор не перепрыгнет
					if ((float(view.width()) - EDIT_OFFSET_HORZ_CURSOR) > EDIT_OFFSET_HORZ_CURSOR) offset.left += int(EDIT_OFFSET_HORZ_CURSOR);
				}
			}

			// вертикальное смещение
			// FIXME проверить, помоему просто >
			if (textSize.height >= view.height())
			{
				if (cursor.top < view.top)
				{
					offset.top = point.top - (view.top - cursor.top);
				}
				else if (cursor.bottom > view.bottom)
				{
					offset.top = point.top + (cursor.bottom - view.bottom);
				}
			}

		}

		if (offset != point)
		{
			mText->setViewOffset(offset);
			// обновить скролы
			if (mVScroll != nullptr)
				mVScroll->setScrollPosition(offset.top);
			if (mHScroll != nullptr)
				mHScroll->setScrollPosition(offset.left);
		}
	}

	void Edit::setContentPosition(const IntPoint& _point)
	{
		if (mText != nullptr)
			mText->setViewOffset(_point);
	}

	IntSize Edit::getViewSize() const
	{
		return mWidgetClient == nullptr ? getSize() : mWidgetClient->getSize();
	}

	IntSize Edit::getContentSize()
	{
		return mText == nullptr ? IntSize() : mText->getTextSize();
	}

	size_t Edit::getVScrollPage()
	{
		return (size_t)getFontHeight();
	}

	size_t Edit::getHScrollPage()
	{
		return (size_t)getFontHeight();
	}

	IntPoint Edit::getContentPosition()
	{
		return mText == nullptr ? IntPoint() : mText->getViewOffset();
	}

	Align Edit::getContentAlign()
	{
		return mText == nullptr ? Align::Default : mText->getTextAlign();
	}

	void Edit::setTextIntervalColour(size_t _start, size_t _count, const Colour& _colour)
	{
		_setTextColour(_start, _count, _colour, false);
	}

	size_t Edit::getTextSelectionStart()
	{
		return (mStartSelect == ITEM_NONE) ? ITEM_NONE : (mStartSelect > mEndSelect ? mEndSelect : mStartSelect);
	}

	size_t Edit::getTextSelectionEnd()
	{
		return (mStartSelect == ITEM_NONE) ? ITEM_NONE : (mStartSelect > mEndSelect ? mStartSelect : mEndSelect);
	}

	bool Edit::isTextSelection()
	{
		return ( (mStartSelect != ITEM_NONE) && (mStartSelect != mEndSelect) );
	}

	void Edit::deleteTextSelection()
	{
		deleteTextSelect(false);
	}

	void Edit::setTextSelectionColour(const Colour& _colour)
	{
		setTextSelectColour(_colour, false);
	}

	size_t Edit::getTextSelectionLength()
	{
		return mEndSelect - mStartSelect;
	}

	void Edit::setOnlyText(const UString& _text)
	{
		setText(TextIterator::toTagsString(_text), false);
	}

	UString Edit::getOnlyText()
	{
		return TextIterator::getOnlyText(getRealString());
	}

	void Edit::insertText(const UString& _text, size_t _index)
	{
		insertText(_text, _index, false);
	}

	void Edit::addText(const UString& _text)
	{
		insertText(_text, ITEM_NONE, false);
	}

	void Edit::eraseText(size_t _start, size_t _count)
	{
		eraseText(_start, _count, false);
	}

	void Edit::setEditReadOnly(bool _read)
	{
		mModeReadOnly = _read;
		// сбрасываем историю
		commandResetHistory();
	}

	void Edit::setEditMultiLine(bool _multi)
	{
		mModeMultiline = _multi;
		// на всякий, для уберания переносов
		if (!mModeMultiline)
		{
			setText(getRealString(), false);
		}
		// обновляем по размерам
		else updateView();
		// сбрасываем историю
		commandResetHistory();
	}

	void Edit::setEditStatic(bool _static)
	{
		mModeStatic = _static;
		resetSelect();

		if (mWidgetClient != nullptr)
		{
			if (mModeStatic) mWidgetClient->setPointer("");
			else mWidgetClient->setPointer(mOriginalPointer);
		}
	}

	void Edit::setPasswordChar(const UString& _char)
	{
		if (!_char.empty()) setPasswordChar(_char[0]);
	}

	void Edit::setVisibleVScroll(bool _value)
	{
		mVisibleVScroll = _value;
		updateView();
	}

	void Edit::setVisibleHScroll(bool _value)
	{
		mVisibleHScroll = _value;
		updateView();
	}

	void Edit::setProperty(const std::string& _key, const std::string& _value)
	{
		if (_key == "Edit_CursorPosition") setTextCursor(utility::parseValue<size_t>(_value));
		else if (_key == "Edit_TextSelect") setTextSelection(utility::parseValue< types::TSize<size_t> >(_value).width, utility::parseValue< types::TSize<size_t> >(_value).height);
		else if (_key == "Edit_ReadOnly") setEditReadOnly(utility::parseValue<bool>(_value));
		else if (_key == "Edit_Password") setEditPassword(utility::parseValue<bool>(_value));
		else if (_key == "Edit_MultiLine") setEditMultiLine(utility::parseValue<bool>(_value));
		else if (_key == "Edit_PasswordChar") setPasswordChar(_value);
		else if (_key == "Edit_MaxTextLength") setMaxTextLength(utility::parseValue<size_t>(_value));
		else if (_key == "Edit_OverflowToTheLeft") setOverflowToTheLeft(utility::parseValue<bool>(_value));
		else if (_key == "Edit_Static") setEditStatic(utility::parseValue<bool>(_value));
		else if (_key == "Edit_VisibleVScroll") setVisibleVScroll(utility::parseValue<bool>(_value));
		else if (_key == "Edit_VisibleHScroll") setVisibleHScroll(utility::parseValue<bool>(_value));
		else if (_key == "Edit_WordWrap") setEditWordWrap(utility::parseValue<bool>(_value));
		else if (_key == "Edit_TabPrinting") setTabPrinting(utility::parseValue<bool>(_value));
		else if (_key == "Edit_InvertSelected") setInvertSelected(utility::parseValue<bool>(_value));

#ifndef MYGUI_DONT_USE_OBSOLETE
		else if (_key == "Edit_ShowVScroll")
		{
			MYGUI_LOG(Warning, "Edit_ShowVScroll is obsolete, use Edit_VisibleVScroll");
			setVisibleVScroll(utility::parseValue<bool>(_value));
		}
		else if (_key == "Edit_ShowHScroll")
		{
			MYGUI_LOG(Warning, "Edit_ShowHScroll is obsolete, use Edit_VisibleHScroll");
			setVisibleHScroll(utility::parseValue<bool>(_value));
		}
#endif // MYGUI_DONT_USE_OBSOLETE

		else
		{
			Base::setProperty(_key, _value);
			return;
		}
		eventChangeProperty(this, _key, _value);
	}

	size_t Edit::getVScrollRange()
	{
		return mVRange + 1;
	}

	size_t Edit::getVScrollPosition()
	{
		return mText == nullptr ? 0 : mText->getViewOffset().top;
	}

	void Edit::setVScrollPosition(size_t _index)
	{
		if (mText == nullptr)
			return;

		if (_index > mVRange)
			_index = mVRange;

		IntPoint point = mText->getViewOffset();
		point.top = _index;

		mText->setViewOffset(point);
		// обновить скролы
		if (mVScroll != nullptr)
			mVScroll->setScrollPosition(point.top);
	}

	size_t Edit::getHScrollRange()
	{
		return mHRange + 1;
	}

	size_t Edit::getHScrollPosition()
	{
		return mText == nullptr ? 0 : mText->getViewOffset().left;
	}

	void Edit::setHScrollPosition(size_t _index)
	{
		if (mText == nullptr)
			return;

		if (_index > mHRange)
			_index = mHRange;

		IntPoint point = mText->getViewOffset();
		point.left = _index;

		mText->setViewOffset(point);
		// обновить скролы
		if (mHScroll != nullptr)
			mHScroll->setScrollPosition(point.left);
	}

	bool Edit::getInvertSelected()
	{
		return mText == nullptr ? false : mText->getInvertSelected();
	}

	void Edit::setInvertSelected(bool _value)
	{
		if (mText != nullptr)
			mText->setInvertSelected(_value);
	}

} // namespace MyGUI
