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
#include "MyGUI_Precompiled.h"
#include "MyGUI_EditText.h"
#include "MyGUI_RenderItem.h"
#include "MyGUI_FontManager.h"
#include "MyGUI_RenderManager.h"
#include "MyGUI_LanguageManager.h"
#include "MyGUI_TextIterator.h"
#include "MyGUI_IRenderTarget.h"
#include "MyGUI_FontData.h"
#include "MyGUI_CommonStateInfo.h"

namespace MyGUI
{

	const size_t VERTEX_IN_QUAD = 6;
	const size_t SIMPLETEXT_COUNT_VERTEX = 32 * VERTEX_IN_QUAD;

	MYGUI_FORCEINLINE void DrawQuad(
		MyGUI::Vertex*& _buff,
		float v_left,
		float v_top,
		float v_rignt,
		float v_bottom,
		float v_z,
		MyGUI::uint32 _colour,
		float t_left,
		float t_top,
		float t_right,
		float t_bottom,
		size_t& _count)
	{
		_buff[0].x = v_left;
		_buff[0].y = v_top;
		_buff[0].z = v_z;
		_buff[0].colour = _colour;
		_buff[0].u = t_left;
		_buff[0].v = t_top;

		_buff[1].x = v_left;
		_buff[1].y = v_bottom;
		_buff[1].z = v_z;
		_buff[1].colour = _colour;
		_buff[1].u = t_left;
		_buff[1].v = t_bottom;

		_buff[2].x = v_rignt;
		_buff[2].y = v_top;
		_buff[2].z = v_z;
		_buff[2].colour = _colour;
		_buff[2].u = t_right;
		_buff[2].v = t_top;

		_buff[3].x = v_rignt;
		_buff[3].y = v_top;
		_buff[3].z = v_z;
		_buff[3].colour = _colour;
		_buff[3].u = t_right;
		_buff[3].v = t_top;

		_buff[4].x = v_left;
		_buff[4].y = v_bottom;
		_buff[4].z = v_z;
		_buff[4].colour = _colour;
		_buff[4].u = t_left;
		_buff[4].v = t_bottom;

		_buff[5].x = v_rignt;
		_buff[5].y = v_bottom;
		_buff[5].z = v_z;
		_buff[5].colour = _colour;
		_buff[5].u = t_right;
		_buff[5].v = t_bottom;

		_buff += VERTEX_IN_QUAD;
		_count += VERTEX_IN_QUAD;
	}

	EditText::EditText() :
		ISubWidgetText(),
		mEmptyView(false),
		mCurrentColour(0x00FFFFFF),
		mInverseColour(0x00000000),
		mCurrentAlpha(0xFF000000),
		mTextOutDate(false),
		mTextAlign(Align::Default),
		mColour(Colour::White),
		mAlpha(ALPHA_MAX),
		mFont(nullptr),
		mTexture(nullptr),
		mFontHeight(0),
		mBackgroundNormal(true),
		mStartSelect(0),
		mEndSelect(0),
		mCursorPosition(0),
		mVisibleCursor(false),
		mInvertSelect(true),
		mNode(nullptr),
		mRenderItem(nullptr),
		mCountVertex(SIMPLETEXT_COUNT_VERTEX),
		mIsAddCursorWidth(true),
		mShiftText(false),
		mWordWrap(false),
		mOldWidth(0)
	{
		mVertexFormat = RenderManager::getInstance().getVertexFormat();

		mCurrentColour = texture_utility::toColourARGB(mColour);
		texture_utility::convertColour(mCurrentColour, mVertexFormat);

		mCurrentColour = (mCurrentColour & 0x00FFFFFF) | mCurrentAlpha;
		mInverseColour = mCurrentColour ^ 0x00FFFFFF;
	}

	EditText::~EditText()
	{
	}

	void EditText::setVisible(bool _visible)
	{
		if (mVisible == _visible) return;
		mVisible = _visible;

		if (nullptr != mNode) mNode->outOfDate(mRenderItem);
	}

	void EditText::_correctView()
	{
		if (nullptr != mNode) mNode->outOfDate(mRenderItem);
	}

	void EditText::_setAlign(const IntCoord& _oldcoord, bool _update)
	{
		_setAlign(_oldcoord.size(), _update);
	}

	void EditText::_setAlign(const IntSize& _oldsize, bool _update)
	{

		if (mWordWrap)
		{
			// передается старая координата всегда
			int width = mCroppedParent->getWidth();
			if (mOldWidth != width)
			{
				mOldWidth = width;
				mTextOutDate = true;
			}
		}

		// необходимо разобраться
		bool need_update = true;//_update;

		// первоначальное выравнивание
		if (mAlign.isHStretch())
		{
			// растягиваем
			mCoord.width = mCoord.width + (mCroppedParent->getWidth() - _oldsize.width);
			need_update = true;
			mIsMargin = true; // при изменении размеров все пересчитывать
		}
		else if (mAlign.isRight())
		{
			// двигаем по правому краю
			mCoord.left = mCoord.left + (mCroppedParent->getWidth() - _oldsize.width);
			need_update = true;
		}
		else if (mAlign.isHCenter())
		{
			// выравнивание по горизонтали без растяжения
			mCoord.left = (mCroppedParent->getWidth() - mCoord.width) / 2;
			need_update = true;
		}

		if (mAlign.isVStretch())
		{
			// растягиваем
			mCoord.height = mCoord.height + (mCroppedParent->getHeight() - _oldsize.height);
			need_update = true;
			mIsMargin = true; // при изменении размеров все пересчитывать
		}
		else if (mAlign.isBottom())
		{
			// двигаем по нижнему краю
			mCoord.top = mCoord.top + (mCroppedParent->getHeight() - _oldsize.height);
			need_update = true;
		}
		else if (mAlign.isVCenter())
		{
			// выравнивание по вертикали без растяжения
			mCoord.top = (mCroppedParent->getHeight() - mCoord.height) / 2;
			need_update = true;
		}

		if (need_update)
		{
			mCurrentCoord = mCoord;
			_updateView();
		}

	}

	void EditText::_updateView()
	{
		bool margin = _checkMargin();

		mEmptyView = ((0 >= _getViewWidth()) || (0 >= _getViewHeight()));

		mCurrentCoord.left = mCoord.left + mMargin.left;
		mCurrentCoord.top = mCoord.top + mMargin.top;

		// вьюпорт стал битым
		if (margin)
		{
			// проверка на полный выход за границу
			if (_checkOutside())
			{
				// запоминаем текущее состояние
				mIsMargin = margin;

				// обновить перед выходом
				if (nullptr != mNode) mNode->outOfDate(mRenderItem);
				return;
			}
		}

		// мы обрезаны или были обрезаны
		if ((mIsMargin) || (margin))
		{
			mCurrentCoord.width = _getViewWidth();
			mCurrentCoord.height = _getViewHeight();
		}

		// запоминаем текущее состояние
		mIsMargin = margin;

		if (nullptr != mNode) mNode->outOfDate(mRenderItem);
	}

	void EditText::setCaption(const UString& _value)
	{
		mCaption = _value;
		mTextOutDate = true;

		// если вершин не хватит, делаем реалок, с учетом выделения * 2 и курсора
		size_t need = (mCaption.size() * 2 + 2) * VERTEX_IN_QUAD;
		if (mCountVertex < need)
		{
			mCountVertex = need + SIMPLETEXT_COUNT_VERTEX;
			if (nullptr != mRenderItem) mRenderItem->reallockDrawItem(this, mCountVertex);
		}
		if (nullptr != mNode) mNode->outOfDate(mRenderItem);
	}

	const UString& EditText::getCaption()
	{
		return mCaption;
	}

	void EditText::setTextColour(const Colour& _value)
	{
		if (mColour == _value) return;
		mColour = _value;
		mCurrentColour = texture_utility::toColourARGB(mColour);

		texture_utility::convertColour(mCurrentColour, mVertexFormat);

		mCurrentColour = (mCurrentColour & 0x00FFFFFF) | mCurrentAlpha;
		mInverseColour = mCurrentColour ^ 0x00FFFFFF;

		if (nullptr != mNode) mNode->outOfDate(mRenderItem);
	}

	const Colour& EditText::getTextColour()
	{
		return mColour;
	}

	void EditText::setAlpha(float _value)
	{
		if (mAlpha == _value) return;
		mAlpha = _value;
		mCurrentAlpha = ((uint8)(mAlpha*255) << 24);
		mCurrentColour = (mCurrentColour & 0x00FFFFFF) | mCurrentAlpha;
		mInverseColour = mCurrentColour ^ 0x00FFFFFF;

		if (nullptr != mNode) mNode->outOfDate(mRenderItem);
	}

	float EditText::getAlpha()
	{
		return mAlpha;
	}

	void EditText::setFontName(const std::string& _value)
	{
		mTexture = 0;
		mFont = FontManager::getInstance().getByName(_value);
		if (mFont != nullptr)
		{
			mTexture = mFont->getTextureFont();

			// если надо, устанавливаем дефолтный размер шрифта
			if (mFont->getDefaultHeight() != 0)
			{
				mFontHeight = mFont->getDefaultHeight();
			}
		}

		mTextOutDate = true;

		// если мы были приаттаченны, то удаляем себя
		if (nullptr != mRenderItem)
		{
			mRenderItem->removeDrawItem(this);
			mRenderItem = nullptr;
		}

		// если есть текстура, то приаттачиваемся
		if (nullptr != mTexture && nullptr != mNode)
		{
			mRenderItem = mNode->addToRenderItem(mTexture, this);
			mRenderItem->addDrawItem(this, mCountVertex);
		}

		if (nullptr != mNode) mNode->outOfDate(mRenderItem);
	}

	const std::string& EditText::getFontName()
	{
		return mFont->getResourceName();
	}

	void EditText::setFontHeight(int _value)
	{
		mFontHeight = _value;
		mTextOutDate = true;
		if (nullptr != mNode) mNode->outOfDate(mRenderItem);
	}

	int EditText::getFontHeight()
	{
		return mFontHeight;
	}

	void EditText::createDrawItem(ITexture* _texture, ILayerNode * _node)
	{
		mNode = _node;
		// если уже есть текстура, то атачимся, актуально для смены леера
		if (nullptr != mTexture)
		{
			MYGUI_ASSERT(!mRenderItem, "mRenderItem must be nullptr");

			mRenderItem = mNode->addToRenderItem(mTexture, this);
			mRenderItem->addDrawItem(this, mCountVertex);
		}
	}

	void EditText::destroyDrawItem()
	{
		if (nullptr != mRenderItem)
		{
			mRenderItem->removeDrawItem(this);
			mRenderItem = nullptr;
		}
		mNode = nullptr;
	}

	size_t EditText::getTextSelectionStart()
	{
		return mStartSelect;
	}

	size_t EditText::getTextSelectionEnd()
	{
		return mEndSelect;
	}

	void EditText::setTextSelection(size_t _start, size_t _end)
	{
		mStartSelect=_start;
		mEndSelect=_end;
		if (nullptr != mNode) mNode->outOfDate(mRenderItem);
	}

	bool EditText::getSelectBackground()
	{
		return mBackgroundNormal;
	}

	void EditText::setSelectBackground(bool _normal)
	{
		if (mBackgroundNormal == _normal) return;
		mBackgroundNormal = _normal;
		if (nullptr != mNode) mNode->outOfDate(mRenderItem);
	}

	bool EditText::isVisibleCursor()
	{
		return mVisibleCursor;
	}

	void EditText::setVisibleCursor(bool _value)
	{
		if (mVisibleCursor == _value) return;
		mVisibleCursor = _value;
		if (nullptr != mNode) mNode->outOfDate(mRenderItem);
	}

	size_t EditText::getCursorPosition()
	{
		return mCursorPosition;
	}

	void EditText::setCursorPosition(size_t _index)
	{
		if (mCursorPosition == _index) return;
		mCursorPosition = _index;
		if (nullptr != mNode) mNode->outOfDate(mRenderItem);
	}

	void EditText::setTextAlign(Align _value)
	{
		mTextAlign = _value;
		if (nullptr != mNode) mNode->outOfDate(mRenderItem);
	}

	Align EditText::getTextAlign()
	{
		return mTextAlign;
	}

	IntSize EditText::getTextSize()
	{
		// если нуно обновить, или изменились пропорции экрана
		if (mTextOutDate) updateRawData();

		IntSize size = mTextView.getViewSize();
		// плюс размер курсора
		if (mIsAddCursorWidth)
			size.width += 2;
		return size;
	}

	void EditText::setViewOffset(const IntPoint& _point)
	{
		mViewOffset = _point;
		if (nullptr != mNode) mNode->outOfDate(mRenderItem);
	}

	IntPoint EditText::getViewOffset()
	{
		return mViewOffset;
	}

	size_t EditText::getCursorPosition(const IntPoint& _point)
	{
		if (nullptr == mFont) return 0;
		if (mTextOutDate) updateRawData();

		IntPoint point = _point;
		point -= mCroppedParent->getAbsolutePosition();
		point += mViewOffset;
		point -= mCoord.point();

		return mTextView.getCursorPosition(point);
	}

	IntCoord EditText::getCursorCoord(size_t _position)
	{
		if (nullptr == mFont) return IntCoord();
		if (mTextOutDate) updateRawData();

		IntPoint point = mTextView.getCursorPoint(_position);
		point += mCroppedParent->getAbsolutePosition();
		point -= mViewOffset;
		point += mCoord.point();

		return IntCoord(point.left, point.top, 2, mFontHeight);
	}

	void EditText::setShiftText(bool _value)
	{
		if (mShiftText == _value) return;
		mShiftText = _value;
		if (nullptr != mNode) mNode->outOfDate(mRenderItem);
	}

	void EditText::setWordWrap(bool _value)
	{
		mWordWrap = _value;
		mTextOutDate = true;
		if (nullptr != mNode) mNode->outOfDate(mRenderItem);
	}

	void EditText::updateRawData()
	{
		if (nullptr == mFont) return;
		// сбрасывам флаги
		mTextOutDate = false;

		int width = -1;
		if (mWordWrap)
		{
			width = mCoord.width;
			// обрезать слова нужно по шарине, которую мы реально используем
			if (mIsAddCursorWidth)
				width -= 2;
		}

		mTextView.update(mCaption, mFont, mFontHeight, mTextAlign, mVertexFormat, width);
	}

	void EditText::setStateData(IStateInfo* _data)
	{
		EditTextStateInfo* data = _data->castType<EditTextStateInfo>();
		if (data->getColour() != Colour::Zero) setTextColour(data->getColour());
		setShiftText(data->getShift());
	}

	void EditText::doRender()
	{
		if (nullptr == mFont) return;
		if (!mVisible || mEmptyView) return;

		bool _update = mRenderItem->getCurrentUpdate();
		if (_update) mTextOutDate = true;

		if (mTextOutDate) updateRawData();

		Vertex* _vertex = mRenderItem->getCurrentVertextBuffer();

		const RenderTargetInfo& info = mRenderItem->getRenderTarget()->getInfo();

		// колличество отрисованных вершин
		size_t vertex_count = 0;

		// текущие цвета
		uint32 colour_current = mCurrentColour;
		uint32 colour = mCurrentColour;
		uint32 colour_inverse = mInverseColour;

		GlyphInfo* back_glyph = mFont->getGlyphInfo(mBackgroundNormal ? FontCodeType::Selected : FontCodeType::SelectedBack);

		float vertex_z = info.maximumDepth;

		const VectorLineInfo& data = mTextView.getData();

		int left = - mViewOffset.left + mCoord.left;
		int top = - mViewOffset.top + mCoord.top;
		int width = 0;
		const int height = mFontHeight;

		size_t index = 0;
		for (VectorLineInfo::const_iterator line=data.begin(); line!=data.end(); ++line)
		{
			left = line->offset - mViewOffset.left + mCoord.left;
			for (VectorCharInfo::const_iterator sim=line->simbols.begin(); sim!=line->simbols.end(); ++sim)
			{
				if (sim->isColour())
				{
					colour = sim->getColour() | (colour & 0xFF000000);
					colour_inverse = colour ^ 0x00FFFFFF;
					continue;
				}

				// смещение текстуры для фона
				bool select = !((index >= mEndSelect) || (index < mStartSelect));

				uint32 back_colour = 0;

				// выделение символа
				if (!select || !mInvertSelect)
				{
					colour_current = colour;
					back_colour = colour | 0x00FFFFFF;
				}
				else
				{
					colour_current = colour_inverse;
					back_colour = colour_inverse;
				}

				bool draw = true;

				// текущие размеры
				MyGUI::FloatRect texture_rect = sim->getUVRect();
				width = sim->getWidth();

				// конечные размеры
				int result_left = left;
				int result_top = top;
				int result_right = left + width;
				int result_bottom = top + height;

				float texture_width = texture_rect.right - texture_rect.left;
				float texture_height = texture_rect.bottom - texture_rect.top;

				// символ залазиет влево
				if (left < mCurrentCoord.left)
				{
					// символ вообще не виден
					if (left + width <= mCurrentCoord.left)
					{
						draw = false;
					}
					// символ обрезан
					else
					{
						result_left = mCurrentCoord.left;
						texture_rect.left += (texture_width * (float)(result_left - left) / (float)width);
					}
				}

				// символ залазиет вправо
				if (left + width > mCurrentCoord.right())
				{
					// символ вообще не виден
					if (left >= mCurrentCoord.right())
					{
						draw = false;
					}
					// символ обрезан
					else
					{
						result_right = mCurrentCoord.right();
						texture_rect.right -= (texture_width * (float)((left + width) - result_right) / (float)width);
					}
				}

				// символ залазиет вверх
				if (top < mCurrentCoord.top)
				{
					// символ вообще не виден
					if (top + height <= mCurrentCoord.top)
					{
						draw = false;
					}
					// символ обрезан
					else
					{
						result_top = mCurrentCoord.top;
						texture_rect.top += (texture_height * (float)(result_top - top) / (float)height);
					}
				}

				// символ залазиет вниз
				if (top + height > mCurrentCoord.bottom())
				{
					// символ вообще не виден
					if (top >= mCurrentCoord.bottom())
					{
						draw = false;
					}
					// символ обрезан
					else
					{
						result_bottom = mCurrentCoord.bottom();
						texture_rect.bottom -= (texture_height * (float)((top + height) - result_bottom) / (float)height);
					}
				}

				if (draw)
				{
					int pix_left = mCroppedParent->getAbsoluteLeft() - info.leftOffset + result_left;
					int pix_top = mCroppedParent->getAbsoluteTop() - info.topOffset + (mShiftText ? 1 : 0) + result_top;

					float real_left = ((info.pixScaleX * (float)(pix_left) + info.hOffset) * 2) - 1;
					float real_top = - (((info.pixScaleY * (float)(pix_top) + info.vOffset) * 2) - 1);
					float real_right = ((info.pixScaleX * (float)(pix_left + result_right - result_left) + info.hOffset) * 2) - 1;
					float real_bottom = - (((info.pixScaleY * (float)(pix_top + result_bottom - result_top) + info.vOffset) * 2) - 1);

					// если нужно рисуем выделение
					if (select)
					{
						const FloatRect& background_current = back_glyph->uvRect;
						DrawQuad(_vertex, real_left, real_top, real_right, real_bottom, vertex_z, back_colour,
							background_current.left, background_current.top, background_current.left, background_current.top, vertex_count);
					}

					DrawQuad(_vertex, real_left, real_top, real_right, real_bottom, vertex_z, colour_current,
						texture_rect.left, texture_rect.top, texture_rect.right, texture_rect.bottom, vertex_count);
				}

				left += width;
				index++;
			}

			top += height;
			index++;
		}

		if (mVisibleCursor)
		{
			MyGUI::IntPoint point = mTextView.getCursorPoint(mCursorPosition);
			point -= mViewOffset;
			point += mCoord.point();

			bool draw = true;

			GlyphInfo* cursor_glyph = mFont->getGlyphInfo(FontCodeType::Cursor);
			MyGUI::FloatRect texture_rect = cursor_glyph->uvRect;
			left = point.left;
			top = point.top;
			width = 2;//cursor_glyph->width;

			// конечные размеры
			int result_left = left;
			int result_top = top;
			int result_width = width;
			int result_height = height;

			// символ залазиет влево
			if (left < mCurrentCoord.left)
			{
				// символ вообще не виден
				if (left + width <= mCurrentCoord.left)
				{
					draw = false;
				}
				// символ обрезан
				else
				{
					result_left = mCurrentCoord.left;
					result_width = width - (mCurrentCoord.left - left);

					float texture_width = texture_rect.right - texture_rect.left;
					texture_rect.left = texture_rect.right - (texture_width * (float)result_width / (float)width);
				}
			}

			// символ залазиет вправо
			if (left + width > mCurrentCoord.right())
			{
				// символ вообще не виден
				if (left >= mCurrentCoord.right())
				{
					draw = false;
				}
				// символ обрезан
				else
				{
					result_width = mCurrentCoord.right() - left;

					float texture_width = texture_rect.right - texture_rect.left;
					texture_rect.right = texture_rect.left + (texture_width * (float)result_width / (float)width);
				}
			}

			// символ залазиет вверх
			if (top < mCurrentCoord.top)
			{
				// символ вообще не виден
				if (top + height <= mCurrentCoord.top)
				{
					draw = false;
				}
				// символ обрезан
				else
				{
					result_top = mCurrentCoord.top;
					result_height = height - (mCurrentCoord.top - top);

					float texture_height = texture_rect.bottom - texture_rect.top;
					texture_rect.top = texture_rect.bottom - (texture_height * (float)result_height / (float)height);
				}
			}

			// символ залазиет вниз
			if (top + height > mCurrentCoord.bottom())
			{
				// символ вообще не виден
				if (top >= mCurrentCoord.bottom())
				{
					draw = false;
				}
				// символ обрезан
				else
				{
					result_height = mCurrentCoord.bottom() - top;

					float texture_height = texture_rect.bottom - texture_rect.top;
					texture_rect.bottom = texture_rect.top + (texture_height * (float)result_height / (float)height);
				}
			}

			if (draw)
			{
				int pix_left = mCroppedParent->getAbsoluteLeft() - info.leftOffset + result_left;
				int pix_top = mCroppedParent->getAbsoluteTop() - info.topOffset + (mShiftText ? 1 : 0) + result_top;

				float real_left = ((info.pixScaleX * (float)(pix_left) + info.hOffset) * 2) - 1;
				float real_top = - (((info.pixScaleY * (float)(pix_top) + info.vOffset) * 2) - 1);
				float real_right = ((info.pixScaleX * (float)(pix_left + result_width) + info.hOffset) * 2) - 1;
				float real_bottom = - (((info.pixScaleY * (float)(pix_top + result_height) + info.vOffset) * 2) - 1);

				DrawQuad(_vertex, real_left, real_top, real_right, real_bottom, vertex_z, colour_current | 0x00FFFFFF,
					texture_rect.left, texture_rect.top, texture_rect.right, texture_rect.bottom, vertex_count);
			}
		}

		// колличество реально отрисованных вершин
		mRenderItem->setLastVertexCount(vertex_count);
	}

	void EditText::setInvertSelected(bool _value)
	{
		if (mInvertSelect == _value) return;
		mInvertSelect = _value;
		if (nullptr != mNode) mNode->outOfDate(mRenderItem);
	}

} // namespace MyGUI
