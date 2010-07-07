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
#ifndef __MYGUI_TEXT_ITERATOR_H__
#define __MYGUI_TEXT_ITERATOR_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Colour.h"
#include "MyGUI_TextChangeHistory.h"
#include "MyGUI_IFont.h"

namespace MyGUI
{

	class MYGUI_EXPORT TextIterator
	{
	private:
		TextIterator();

	public:
		TextIterator(const UString& _text, VectorChangeInfo * _history = nullptr);

		bool moveNext();

		// возвращает цвет
		UString getTagColour(bool _clear = false);

		// возвращает цвет
		bool getTagColour(UString& _colour);

		// удаляет цвет
		void clearTagColour() { getTagColour(true); }

		bool setTagColour(const Colour& _colour);

		bool setTagColour(UString _colour);

		// сохраняет текущий итератор
		bool saveStartPoint();

		// возвращает строку от сохраненного итератора до текущего
		UString getFromStart();

		// удаляет от запомненной точки до текущей
		bool eraseFromStart();

		// возвращает текущую псевдо позицию
		size_t getPosition() const { return mPosition; }

		const UString& getText() const { return mText; }

		void insertText(const UString& _insert, bool _multiLine);

		void clearNewLine(UString& _text);

		//очищает весь текст
		void clearText() { clear(); }

		// возвращает размер строки
		size_t getSize() const;

		void setText(const UString& _text, bool _multiLine);

		void cutMaxLength(size_t _max);

		void cutMaxLengthFromBeginning(size_t _max);

		// возвращает текст без тегов
		static UString getOnlyText(const UString& _text);

		static UString getTextNewLine() { return L"\n"; }

		static UString getTextCharInfo(Char _char);

		// просто конвертируем цвет в строку
		static UString convertTagColour(const Colour& _colour);

		static UString toTagsString(const UString& _text);

	private:

		// возвращает цвет
		bool getTagColour(UString& _colour, UString::iterator& _iter);

		void insert(UString::iterator& _start, UString& _insert);

		UString::iterator erase(UString::iterator _start, UString::iterator _end);

		void clear();

	private:
		UString mText;
		UString::iterator mCurrent, mEnd, mSave;

		// позиция и размер
		size_t mPosition;
		mutable size_t mSize;
		bool mFirst;

		VectorChangeInfo * mHistory;

	};

} // namespace MyGUI

#endif // __MYGUI_TEXT_ITERATOR_H__
