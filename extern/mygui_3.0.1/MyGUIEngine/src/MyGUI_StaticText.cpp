/*!
	@file
	@author		Albert Semenov
	@date		12/2007
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
#include "MyGUI_StaticText.h"

namespace MyGUI
{

	StaticText::StaticText()
	{
	}

	void StaticText::_initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name)
	{
		Base::_initialise(_style, _coord, _align, _info, _parent, _croppedParent, _creator, _name);

		initialiseWidgetSkin(_info);
	}

	StaticText::~StaticText()
	{
		shutdownWidgetSkin();
	}

	void StaticText::baseChangeWidgetSkin(ResourceSkin* _info)
	{
		shutdownWidgetSkin();
		Base::baseChangeWidgetSkin(_info);
		initialiseWidgetSkin(_info);
	}

	void StaticText::initialiseWidgetSkin(ResourceSkin* _info)
	{
		// парсим свойства
		const MapString& properties = _info->getProperties();
		if (!properties.empty())
		{
			MapString::const_iterator iter = properties.end();
			if ((iter = properties.find("FontName")) != properties.end()) setFontName(iter->second);
			if ((iter = properties.find("FontHeight")) != properties.end()) setFontHeight(utility::parseInt(iter->second));
			if ((iter = properties.find("TextAlign")) != properties.end()) setTextAlign(Align::parse(iter->second));
			if ((iter = properties.find("TextColour")) != properties.end()) setTextColour(Colour::parse(iter->second));
		}
	}

	void StaticText::shutdownWidgetSkin()
	{
	}

	IntCoord StaticText::getTextRegion()
	{
		return (nullptr == mText) ? IntCoord() : mText->getCoord();
	}

	IntSize StaticText::getTextSize()
	{
		return (nullptr == mText) ? IntSize() : mText->getTextSize();
	}

	void StaticText::setTextAlign(Align _align)
	{
		if (mText != nullptr) mText->setTextAlign(_align);
	}

	Align StaticText::getTextAlign()
	{
		if (mText != nullptr) return mText->getTextAlign();
		return Align::Default;
	}

	void StaticText::setTextColour(const Colour& _colour)
	{
		if (nullptr != mText) mText->setTextColour(_colour);
	}

	const Colour& StaticText::getTextColour()
	{
		return (nullptr == mText) ? Colour::Zero : mText->getTextColour();
	}

	void StaticText::setFontName(const std::string& _font)
	{
		if (nullptr != mText) mText->setFontName(_font);
	}

	const std::string& StaticText::getFontName()
	{
		if (nullptr == mText)
		{
			static std::string empty;
			return empty;
		}
		return mText->getFontName();
	}

	void StaticText::setFontHeight(int _height)
	{
		if (nullptr != mText) mText->setFontHeight(_height);
	}

	int StaticText::getFontHeight()
	{
		return (nullptr == mText) ? 0 : mText->getFontHeight();
	}

	void StaticText::setProperty(const std::string& _key, const std::string& _value)
	{
		if (_key == "Text_TextColour") setTextColour(utility::parseValue<Colour>(_value));
		else if (_key == "Text_TextAlign") setTextAlign(utility::parseValue<Align>(_value));
		else if (_key == "Text_FontName") setFontName(_value);
		else if (_key == "Text_FontHeight") setFontHeight(utility::parseValue<int>(_value));
		else
		{
			Base::setProperty(_key, _value);
			return;
		}
		eventChangeProperty(this, _key, _value);
	}

} // namespace MyGUI
