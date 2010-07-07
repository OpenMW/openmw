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
#include "MyGUI_Message.h"
#include "MyGUI_ResourceSkin.h"
#include "MyGUI_WidgetManager.h"
#include "MyGUI_LayerManager.h"
#include "MyGUI_InputManager.h"
#include "MyGUI_ResourceManager.h"
#include "MyGUI_Gui.h"
#include "MyGUI_ControllerManager.h"
#include "MyGUI_StaticImage.h"
#include "MyGUI_LanguageManager.h"

namespace MyGUI
{

	const float MESSAGE_ALPHA_MAX = 0.5f;
	const float MESSAGE_ALPHA_MIN = 0.0f;
	const float MESSAGE_SPEED_COEF = 3.0f;

	Message::Message() :
		mWidgetText(nullptr),
		mInfoOk(MessageBoxStyle::None),
		mInfoCancel(MessageBoxStyle::None),
		mSmoothShow(false),
		mWidgetFade(nullptr),
		mIcon(nullptr),
		mLeftOffset1(0),
		mLeftOffset2(0)
	{
	}

	void Message::_initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name)
	{
		Base::_initialise(_style, _coord, _align, _info, _parent, _croppedParent, _creator, _name);

		initialiseWidgetSkin(_info);
	}

	Message::~Message()
	{
		shutdownWidgetSkin();
	}

	void Message::baseChangeWidgetSkin(ResourceSkin* _info)
	{
		shutdownWidgetSkin();
		Base::baseChangeWidgetSkin(_info);
		initialiseWidgetSkin(_info);
	}

	void Message::initialiseWidgetSkin(ResourceSkin* _info)
	{
		// парсим виджет для текста
		for (VectorWidgetPtr::iterator iter=mWidgetChildSkin.begin(); iter!=mWidgetChildSkin.end(); ++iter)
		{
			if (*(*iter)->_getInternalData<std::string>() == "Text")
			{
				MYGUI_DEBUG_ASSERT( ! mWidgetText, "widget already assigned");
				mWidgetText = (*iter);
				mOffsetText.set(mCoord.width - mWidgetText->getWidth(), mCoord.height - mWidgetText->getHeight());
				mLeftOffset2 = mLeftOffset1 = mWidgetText->getLeft();
			}
			else if (*(*iter)->_getInternalData<std::string>() == "Icon")
			{
				MYGUI_DEBUG_ASSERT( ! mIcon, "widget already assigned");
				mIcon = (*iter)->castType<StaticImage>();
			}
		}
		//MYGUI_ASSERT(nullptr != mWidgetText, "Child Text not found in skin (MessageBox must have widget for text)");

		if (mIcon != nullptr)
		{
			mLeftOffset2 = mIcon->getRight() + 3;
		}

		// парсим свойства
		const MapString& properties = _info->getProperties();
		if (!properties.empty())
		{
			MapString::const_iterator iter = properties.find("ButtonSkin");
			if (iter != properties.end()) mButtonSkin = iter->second;
			iter = properties.find("ButtonType");
			if (iter != properties.end()) mButtonType = iter->second;
			iter = properties.find("ButtonSize");
			if (iter != properties.end()) mButtonSize = IntSize::parse(iter->second);
			iter = properties.find("ButtonOffset");
			if (iter != properties.end()) mButtonOffset = IntSize::parse(iter->second);
			iter = properties.find("DefaultLayer");
			if (iter != properties.end()) mDefaultLayer = iter->second;
			iter = properties.find("FadeSkin");
			if (iter != properties.end()) mFadeSkin = iter->second;
			iter = properties.find("FadeLayer");
			if (iter != properties.end()) mFadeLayer = iter->second;
		}

	}

	void Message::shutdownWidgetSkin()
	{
		mWidgetText = nullptr;
		mIcon = nullptr;
	}

	void Message::setMessageText(const UString& _message)
	{
		if (mWidgetText != nullptr)
			mWidgetText->setCaption(_message);
		updateSize();
	}

	MessageBoxStyle Message::addButtonName(const UString& _name)
	{
		//FIXME
		if (mVectorButton.size() >= MessageBoxStyle::_CountUserButtons)
		{
			MYGUI_LOG(Warning, "Too many buttons in message box, ignored");
			return MessageBoxStyle::None;
		}
		// бит, номер кнопки + смещение до Button1
		MessageBoxStyle info = MessageBoxStyle(MessageBoxStyle::Enum(MYGUI_FLAG(mVectorButton.size() + MessageBoxStyle::_IndexUserButton1)));

		// запоминаем кнопки для отмены и подтверждения
		if (mVectorButton.empty()) mInfoOk = info;
		mInfoCancel = info;

		Widget* button = createWidgetT(mButtonType, mButtonSkin, IntCoord(), Align::Left | Align::Bottom);
		button->eventMouseButtonClick = newDelegate(this, &Message::notifyButtonClick);
		button->setCaption(_name);
		button->_setInternalData(info);
		mVectorButton.push_back(button);

		updateSize();
		return info;
	}

	void Message::setMessageIcon(MessageBoxStyle _icon)
	{
		if (nullptr == mIcon) return;
		if (mIcon->getItemResource() != nullptr)
		{
			mIcon->setItemName( getIconName(_icon.getIconIndex()) );
		}
		else
		{
			mIcon->setImageIndex(_icon.getIconIndex());
		}

		updateSize();
	}

	void Message::setMessageButton(MessageBoxStyle _info)
	{
		clearButton();

		std::vector<MessageBoxStyle> buttons = _info.getButtons();

		for (size_t index=0; index<buttons.size(); ++index)
		{
			// корректируем ее номер
			MessageBoxStyle info = buttons[index];

			// если бит есть то ставим кнопку
			addButtonName(getButtonName(info));

			// внутри адд сбрасывается
			mVectorButton.back()->_setInternalData(info);

			// первая кнопка
			if (mVectorButton.size() == 1) mInfoOk = info;
			// последняя кнопка
			mInfoCancel = info;
		}

		updateSize();
	}

	void Message::setMessageStyle(MessageBoxStyle _style)
	{
		setMessageButton(_style);
		setMessageIcon(_style);
	}

	void Message::notifyButtonClick(MyGUI::Widget* _sender)
	{
		_destroyMessage(*_sender->_getInternalData<MessageBoxStyle>());
	}

	void Message::clearButton()
	{
		for (VectorWidgetPtr::iterator iter=mVectorButton.begin(); iter!=mVectorButton.end(); ++iter)
		{
			WidgetManager::getInstance().destroyWidget(*iter);
		}
		mVectorButton.clear();
	}

	void Message::onKeyButtonPressed(KeyCode _key, Char _char)
	{
		Base::onKeyButtonPressed(_key, _char);
		if ((_key == KeyCode::Return) || (_key == KeyCode::NumpadEnter)) _destroyMessage(mInfoOk);
		else if (_key == KeyCode::Escape) _destroyMessage(mInfoCancel);
	}

	void Message::_destroyMessage(MessageBoxStyle _result)
	{
		eventMessageBoxResult(this, _result);
		if (nullptr != mWidgetFade)
		{
			if (mSmoothShow)
			{
				ControllerFadeAlpha* controller = createControllerFadeAlpha(MESSAGE_ALPHA_MIN, MESSAGE_SPEED_COEF, false);
				controller->eventPostAction = newDelegate(action::actionWidgetDestroy);
				ControllerManager::getInstance().addItem(mWidgetFade, controller);
			}
			else
			{
				WidgetManager::getInstance().destroyWidget(mWidgetFade);
			}
		}
		if (mSmoothShow) destroySmooth();
		else WidgetManager::getInstance().destroyWidget(this);
	}

	void Message::setSmoothShow(bool _smooth)
	{
		mSmoothShow = _smooth;
		if (mSmoothShow)
		{
			setAlpha(ALPHA_MIN);
			setVisible(true);
			setVisibleSmooth(true);
		}
	}

	void Message::setWindowFade(bool _fade)
	{
		return; //пока пропустим

		if (_fade)
		{
			if (nullptr == mWidgetFade)
			{
				Gui& gui = Gui::getInstance();
				mWidgetFade = gui.createWidgetT(Widget::getClassTypeName(), mFadeSkin, IntCoord(0, 0, gui.getViewSize().width, gui.getViewSize().height), Align::Stretch, mFadeLayer);
				if (mSmoothShow)
				{
					mWidgetFade->setVisible(false);

					ControllerFadeAlpha* controller = createControllerFadeAlpha(MESSAGE_ALPHA_MAX, MESSAGE_SPEED_COEF, false);
					ControllerManager::getInstance().addItem(mWidgetFade, controller);
				}
				else
				{
					mWidgetFade->setAlpha(MESSAGE_ALPHA_MAX);
				}
			}
		}
		else
		{
			if (nullptr != mWidgetFade)
			{
				WidgetManager::getInstance().destroyWidget(mWidgetFade);
				mWidgetFade = nullptr;
			}
		}
	}

	const char * Message::getIconName(size_t _index)
	{
		static const size_t CountIcons = 4;
		static const char * IconNames[CountIcons + 1] = { "Info", "Quest", "Error", "Warning", "" };
		if (_index >= CountIcons) return IconNames[CountIcons];
		return IconNames[_index];
	}

	MyGUI::Message* Message::createMessageBox(
		const std::string& _skin,
		const UString& _caption,
		const UString& _message,
		MessageBoxStyle _style,
		const std::string& _layer,
		bool _modal,
		const std::string& _button1,
		const std::string& _button2,
		const std::string& _button3,
		const std::string& _button4)
	{
		Message* mess = Gui::getInstance().createWidget<Message>(_skin, IntCoord(), Align::Default, _layer);

		mess->setCaption(_caption);
		mess->setMessageText(_message);

		mess->setSmoothShow(true);
		if (_modal) mess->setWindowFade(true);

		mess->setMessageStyle(_style);

		if (!_button1.empty())
		{
			mess->addButtonName(_button1);
			if (!_button2.empty())
			{
				mess->addButtonName(_button2);
				if (!_button3.empty())
				{
					mess->addButtonName(_button3);
				}
			}
		}

		if (_layer.empty()) LayerManager::getInstance().attachToLayerNode(mess->getDefaultLayer(), mess);
		if (_modal) InputManager::getInstance().addWidgetModal(mess);

		return mess;
	}

	void Message::updateSize()
	{
		ISubWidgetText* text = nullptr;
		if (mWidgetText != nullptr)
			text = mWidgetText->getSubWidgetText();
		IntSize size = text == nullptr ? IntSize() : text->getTextSize();
		// минимум высота иконки
		if ((nullptr != mIcon) && (mIcon->getImageIndex() != ITEM_NONE))
		{
			if (size.height < mIcon->getHeight()) size.height = mIcon->getHeight();
			size.width += mIcon->getSize().width;
		}
		size += mOffsetText;
		size.width += 3;

		int width = ((int)mVectorButton.size() * mButtonSize.width) + (((int)mVectorButton.size()+1) * mButtonOffset.width);
		if (size.width < width) size.width = width;

		int offset = (size.width - width)/2;
		offset += mButtonOffset.width;

		const IntSize& view = Gui::getInstance().getViewSize();
		setCoord((view.width-size.width)/2, (view.height-size.height)/2, size.width, size.height);

		if (nullptr != mIcon)
		{
			if (mWidgetText != nullptr)
			{
				if (mIcon->getImageIndex() != ITEM_NONE) mWidgetText->setCoord(mLeftOffset2, mWidgetText->getTop(), mWidgetText->getWidth(), mWidgetText->getHeight());
				else mWidgetText->setCoord(mLeftOffset1, mWidgetText->getTop(), mWidgetText->getWidth(), mWidgetText->getHeight());
			}
		}

		for (VectorWidgetPtr::iterator iter=mVectorButton.begin(); iter!=mVectorButton.end(); ++iter)
		{
			(*iter)->setCoord(offset, mCoord.height - mButtonOffset.height, mButtonSize.width, mButtonSize.height);
			offset += mButtonOffset.width + mButtonSize.width;
		}
	}

	ControllerFadeAlpha* Message::createControllerFadeAlpha(float _alpha, float _coef, bool _enable)
	{
		ControllerItem* item = ControllerManager::getInstance().createItem(ControllerFadeAlpha::getClassTypeName());
		ControllerFadeAlpha* controller = item->castType<ControllerFadeAlpha>();

		controller->setAlpha(_alpha);
		controller->setCoef(_coef);
		controller->setEnabled(_enable);

		return controller;
	}

	void Message::setMessageModal(bool _value)
	{
		if (_value) InputManager::getInstance().addWidgetModal(this);
		else InputManager::getInstance().removeWidgetModal(this);
	}

	UString Message::getButtonName(MessageBoxStyle _style)
	{
		size_t index = _style.getButtonIndex();
		const char* tag = getButtonTag(index);
		UString result = LanguageManager::getInstance().replaceTags(utility::toString("#{", tag, "}"));
		if (result == tag) return getButtonName(index);
		return result;
	}

	const char * Message::getButtonName(size_t _index)
	{
		static const size_t Count = 9;
		static const char * Names[Count + 1] = { "Ok", "Yes", "No", "Abort", "Retry", "Ignore", "Cancel", "Try", "Continue", "" };
		if (_index >= Count) return Names[Count];
		return Names[_index];
	}

	const char * Message::getButtonTag(size_t _index)
	{
		static const size_t Count = 9;
		static const char * Names[Count + 1] = { "MyGUI_MessageBox_Ok", "MyGUI_MessageBox_Yes", "MyGUI_MessageBox_No", "MyGUI_MessageBox_Abort", "MyGUI_MessageBox_Retry", "MyGUI_MessageBox_Ignore", "MyGUI_MessageBox_Cancel", "MyGUI_MessageBox_Try", "MyGUI_MessageBox_Continue", "" };
		if (_index >= Count) return Names[Count];
		return Names[_index];
	}

	void Message::endMessage(MessageBoxStyle _result)
	{
		_destroyMessage(_result);
	}

	void Message::endMessage()
	{
		_destroyMessage(mInfoCancel);
	}

	void Message::setProperty(const std::string& _key, const std::string& _value)
	{
		if (_key == "Message_Caption") setCaption(_value);
		else if (_key == "Message_Message") setMessageText(_value);
		else if (_key == "Message_Modal") setMessageModal(utility::parseValue<bool>(_value));
		else if (_key == "Message_Button") setMessageButton(utility::parseValue<MessageBoxStyle>(_value));
		else if (_key == "Message_AddButton") addButtonName(_value);
		else if (_key == "Message_SmoothShow") setSmoothShow(utility::parseValue<bool>(_value));
		else if (_key == "Message_Fade") setWindowFade(utility::parseValue<bool>(_value));
		else
		{
			Base::setProperty(_key, _value);
			return;
		}
		eventChangeProperty(this, _key, _value);
	}

} // namespace MyGUI
