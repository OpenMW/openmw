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
#ifndef __MYGUI_MESSAGE_H__
#define __MYGUI_MESSAGE_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Window.h"
#include "MyGUI_ResourceImageSet.h"
#include "MyGUI_EventPair.h"
#include "MyGUI_MessageStyle.h"
#include "MyGUI_ControllerFadeAlpha.h"

namespace MyGUI
{

	typedef delegates::CDelegate2<Message*, MessageBoxStyle> EventHandle_MessagePtrMessageStyle;

	class MYGUI_EXPORT Message :
		public Window
	{
		MYGUI_RTTI_DERIVED( Message )

	public:
		Message();

		/** Set message text*/
		void setMessageText(const UString& _value);

		/** Create button with specific name*/
		MessageBoxStyle addButtonName(const UString& _name);

		/** Set smooth message showing*/
		void setSmoothShow(bool _value);

		/** Get name of default layer for Message*/
		const std::string& getDefaultLayer() { return mDefaultLayer; }

		/** Set message icon*/
		void setMessageIcon(MessageBoxStyle _value);
		/** Set fade under message*/
		void setWindowFade(bool _value);

		void endMessage(MessageBoxStyle _result);
		void endMessage();

		/** Create button using MessageBoxStyle*/
		void setMessageButton(MessageBoxStyle _value);

		/** Set message style (button and icon)*/
		void setMessageStyle(MessageBoxStyle _value);

		void setMessageModal(bool _value);

		/** Static method for creating message with one command
			@param
				_modal if true all other GUI elements will be blocked untill message is closed
			@param
				_style any combination of flags from ViewValueInfo
			@param
				_button1 ... _button4 specific buttons names
		*/
		static MyGUI::Message* createMessageBox(
			const std::string& _skin,
			const UString& _caption,
			const UString& _message,
			MessageBoxStyle _style = MessageBoxStyle::Ok | MessageBoxStyle::IconDefault,
			const std::string& _layer = "",
			bool _modal = true,
			const std::string& _button1 = "",
			const std::string& _button2 = "",
			const std::string& _button3 = "",
			const std::string& _button4 = "");


		/** @copydoc Widget::setProperty(const std::string& _key, const std::string& _value) */
		virtual void setProperty(const std::string& _key, const std::string& _value);

	/*event:*/
		/** Event : button on message window pressed.\n
			signature : void method(MyGUI::Message* _sender, MyGUI::MessageBoxStyle _result)\n
			@param _sender widget that called this event
			@param _result - id of pressed button
		*/
		EventHandle_MessagePtrMessageStyle eventMessageBoxResult;

	/*internal:*/
		virtual void _initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name);

	protected:
		virtual ~Message();

		void baseChangeWidgetSkin(ResourceSkin* _info);

		void updateSize();
		void notifyButtonClick(MyGUI::Widget* _sender);
		void clearButton();

		void onKeyButtonPressed(KeyCode _key, Char _char);
		void _destroyMessage(MessageBoxStyle _result);

		UString getButtonName(MessageBoxStyle _style);
		const char * getIconName(size_t _index);
		const char * getButtonName(size_t _index);
		const char * getButtonTag(size_t _index);

	private:
		void initialiseWidgetSkin(ResourceSkin* _info);
		void shutdownWidgetSkin();

		ControllerFadeAlpha* createControllerFadeAlpha(float _alpha, float _coef, bool _enable);

	private:
		IntSize mOffsetText;
		Widget* mWidgetText;

		std::string mButtonSkin, mButtonType;
		IntSize mButtonSize, mButtonOffset;

		VectorWidgetPtr mVectorButton;
		MessageBoxStyle mInfoOk;
		MessageBoxStyle mInfoCancel;
		bool mSmoothShow;

		std::string mDefaultLayer, mDefaultCaption;
		std::string mFadeSkin, mFadeLayer;
		Widget* mWidgetFade;
		StaticImage* mIcon;
		int mLeftOffset1;
		int mLeftOffset2;

	};

} // namespace MyGUI

#endif // __MYGUI_MESSAGE_H__
