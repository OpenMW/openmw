/*!
	@file
	@author		Albert Semenov
	@date		09/2009
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
#ifndef __MYGUI_EDIT_TEXT_H__
#define __MYGUI_EDIT_TEXT_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_XmlDocument.h"
#include "MyGUI_Types.h"
#include "MyGUI_ISubWidgetText.h"
#include "MyGUI_IFont.h"
#include "MyGUI_ResourceSkin.h"
#include "MyGUI_RenderFormat.h"
#include "MyGUI_TextView.h"

namespace MyGUI
{

	class RenderItem;

	class MYGUI_EXPORT EditText : public ISubWidgetText
	{
		MYGUI_RTTI_DERIVED( EditText )

	public:
		EditText();
		virtual ~EditText();

		virtual void setVisible(bool _value);

		// обновляет все данные связанные с тектом
		virtual void updateRawData();

		// метод для отрисовки себя
		virtual void doRender();

		void setCaption(const UString& _value);
		const UString& getCaption();

		void setTextColour(const Colour& _value);
		const Colour& getTextColour();

		void setAlpha(float _value);
		float getAlpha();

		virtual void setFontName(const std::string& _value);
		virtual const std::string& getFontName();

		virtual void setFontHeight(int _value);
		virtual int getFontHeight();

		virtual void createDrawItem(ITexture* _texture, ILayerNode * _node);
		virtual void destroyDrawItem();

		virtual void setTextAlign(Align _value);
		virtual Align getTextAlign();

		virtual size_t getTextSelectionStart();
		virtual size_t getTextSelectionEnd();
		virtual void setTextSelection(size_t _start, size_t _end);

		virtual bool getSelectBackground();
		virtual void setSelectBackground(bool _normal);

		virtual bool isVisibleCursor();
		virtual void setVisibleCursor(bool _value);

		/** Get invert selected text color property */
		virtual bool getInvertSelected() { return mInvertSelect; }
		/** Enable or disable inverting color of selected text\n
			Enabled (true) by default
		*/
		virtual void setInvertSelected(bool _value);

		virtual size_t getCursorPosition();
		virtual void setCursorPosition(size_t _index);

		virtual IntSize getTextSize();

		// устанавливает смещение текста в пикселях
		virtual void setViewOffset(const IntPoint& _point);
		virtual IntPoint getViewOffset();

		// возвращает положение курсора по произвольному положению
		virtual size_t getCursorPosition(const IntPoint& _point);

		// возвращает положение курсора в обсолютных координатах
		virtual IntCoord getCursorCoord(size_t _position);

		void setShiftText(bool _shift);

		void setWordWrap(bool _value);

		virtual void setStateData(IStateInfo * _data);

		void _updateView();
		void _correctView();

	/*internal:*/
		void _setAlign(const IntSize& _oldsize, bool _update);
		void _setAlign(const IntCoord& _oldcoord, bool _update);

	protected:
		bool mEmptyView;
		uint32 mCurrentColour;
		uint32 mInverseColour;
		uint32 mCurrentAlpha;
		IntCoord mCurrentCoord;

		UString mCaption;
		bool mTextOutDate;
		Align mTextAlign;

		Colour mColour;
		float mAlpha;
		VertexColourType mVertexFormat;

		IFont* mFont;
		ITexture* mTexture;
		int mFontHeight;

		bool mBackgroundNormal;
		size_t mStartSelect;
		size_t mEndSelect;
		size_t mCursorPosition;
		bool mVisibleCursor;
		bool mInvertSelect;

		IntPoint mViewOffset; // смещение текста

		ILayerNode* mNode;
		RenderItem* mRenderItem;
		size_t mCountVertex;
		bool mIsAddCursorWidth;

		bool mShiftText;
		bool mWordWrap;
		int mOldWidth;

		TextView mTextView;
	};

} // namespace MyGUI

#endif // __MYGUI_EDIT_TEXT_H__
