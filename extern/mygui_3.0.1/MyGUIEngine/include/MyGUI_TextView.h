/*!
	@file
	@author		Albert Semenov
	@date		09/2009
	@module
*/
#ifndef __MYGUI_TEXT_VIEW_H__
#define __MYGUI_TEXT_VIEW_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_TextureUtility.h"

namespace MyGUI
{

	class CharInfo
	{
	public:
		CharInfo() : width(0) { }
		CharInfo(const FloatRect& _rect, int _width) : rect(_rect), width(_width) { }
		CharInfo(uint32 _colour) : rect(-1, 0, 0, 0), width((int)_colour) { }

		bool isColour() const { return rect.left == -1; }
		int getWidth() const { return width; }
		const FloatRect& getUVRect() const { return rect; }
		uint32 getColour() const { return (uint32)width; }

	private:
		FloatRect rect;
		int width;
	};

	typedef std::vector<CharInfo> VectorCharInfo;

	struct LineInfo
	{
		LineInfo() : width(0), offset(0), count(0) { }
		void clear() { width = 0; count = 0; simbols.clear(); offset = 0; }
		int width;
		int offset;
		size_t count;
		VectorCharInfo simbols;
	};

	typedef std::vector<LineInfo> VectorLineInfo;

	class RollBackPoint
	{
	public:
		RollBackPoint() :
			position(0),
			count(0),
			lenght(0),
			rollback(false)
		{ }

		void set(
			size_t _position,
			UString::const_iterator& _space_point,
			size_t _count,
			int _length
		)
		{
			position = _position;
			space_point = _space_point;
			count = _count;
			lenght = _length;
			rollback = true;
		}

		void clear() { rollback = false; }
		bool empty() const { return !rollback; }
		int getLenght() const { MYGUI_DEBUG_ASSERT(rollback, "rollback point not valid"); return lenght; }
		size_t getCount() const { MYGUI_DEBUG_ASSERT(rollback, "rollback point not valid"); return count; }
		size_t getPosition() const { MYGUI_DEBUG_ASSERT(rollback, "rollback point not valid"); return position; }
		UString::const_iterator getTextIter() { MYGUI_DEBUG_ASSERT(rollback, "rollback point not valid"); return space_point; }

	private:
		size_t position;
		UString::const_iterator space_point;
		size_t count;
		int lenght;
		bool rollback;
	};

	class TextView
	{
	public:
		TextView() :
			mLength(0),
			mFontHeight(0)
		{
		}

		void update(const UString& _text, IFont* _font, int _height, Align _align, VertexColourType _format, int _maxheight = -1)
		{
			mFontHeight = _height;

			// массив дл€ быстрой конвертации цветов
			static const char convert_colour[64] = { 0,  1,  2,  3,  4,  5,  6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
													 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
													 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
													 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0
			                                       };

			mViewSize.clear();

			RollBackPoint roll_back;
			IntSize result;
			int width = 0;
			size_t count = 0;
			mLength = 0;
			mLineInfo.clear();
			LineInfo line_info;
			int font_height = _font->getDefaultHeight();

			UString::const_iterator end = _text.end();
			UString::const_iterator index = _text.begin();

			/*if (index == end)
				return;*/

			result.height += _height;

			for (; index!=end; ++index)
			{
				Char character = *index;

				// нова€ строка
				if (character == FontCodeType::CR
					|| character == FontCodeType::NEL
					|| character == FontCodeType::LF)
				{
					if (character == FontCodeType::CR)
					{
						UString::const_iterator peeki = index;
						peeki ++;
						if ((peeki != end) && (*peeki == FontCodeType::LF))
							index = peeki; // skip both as one newline
					}

					line_info.width = width;
					line_info.count = count;
					mLength += line_info.count + 1;

					result.height += _height;
					if (result.width < width)
						result.width = width;
					width = 0;
					count = 0;

					mLineInfo.push_back(line_info);
					line_info.clear();

					// отмен€ем откат
					roll_back.clear();

					continue;
				}
				// тег
				else if (character == L'#')
				{
					// берем следующий символ
					++ index;
					if (index == end) { --index; continue; } // это защита

					character = *index;
					// если два подр€д, то рисуем один шарп, если нет то мен€ем цвет
					if (character != L'#')
					{
						// парсим первый символ
						uint32 colour = convert_colour[(character-48) & 0x3F];

						// и еще п€ть символов после шарпа
						for (char i=0; i<5; i++)
						{
							++ index;
							if (index == end) { --index; continue; } // это защита
							colour <<= 4;
							colour += convert_colour[ ((*index) - 48) & 0x3F ];
						}

						// если нужно, то мен€ем красный и синий компоненты
						texture_utility::convertColour(colour, _format);

						line_info.simbols.push_back( CharInfo(colour) );

						continue;
					}
				}

				GlyphInfo* info = _font->getGlyphInfo(character);
				if (FontCodeType::Space == character)
				{
					roll_back.set(line_info.simbols.size(), index, count, width);
				}
				else if (FontCodeType::Tab == character)
				{
					roll_back.set(line_info.simbols.size(), index, count, width);
				}

				int char_width = info->width;
				if (font_height != _height)
				{
					char_width = char_width * _height / font_height;
					if (!char_width) char_width = 1;
				}

				// перенос слов
				if (_maxheight != -1
					&& (width + char_width) > _maxheight
					&& !roll_back.empty())
				{
					// откатываем до последнего пробела
					width = roll_back.getLenght();
					count = roll_back.getCount();
					index = roll_back.getTextIter();
					line_info.simbols.erase(line_info.simbols.begin() + roll_back.getPosition(), line_info.simbols.end());

					// запоминаем место отката, как полную строку
					line_info.width = width;
					line_info.count = count;
					mLength += line_info.count + 1;

					result.height += _height;
					if (result.width < width)
						result.width = width;
					width = 0;
					count = 0;

					mLineInfo.push_back(line_info);
					line_info.clear();

					// отмен€ем откат
					roll_back.clear();

					continue;
				}

				line_info.simbols.push_back(CharInfo(info->uvRect, char_width));
				width += char_width;
				count ++;
			}

			line_info.width = width;
			line_info.count = count;
			mLength += line_info.count;

			mLineInfo.push_back(line_info);

			if (result.width < width)
				result.width = width;

			// теперь выравниванием строки
			for (VectorLineInfo::iterator line=mLineInfo.begin(); line!=mLineInfo.end(); ++line)
			{
				if (_align.isRight())
					line->offset = result.width - line->width;
				else if (_align.isHCenter())
					line->offset = (result.width - line->width) / 2;
			}

			mViewSize = result;
		}

		size_t getCursorPosition(const IntPoint& _value)
		{
			const int height = mFontHeight;
			size_t result = 0;
			int top = 0;

			for (VectorLineInfo::const_iterator line=mLineInfo.begin(); line!=mLineInfo.end(); ++line)
			{
				// это последн€€ строка
				bool lastline = !(line + 1 != mLineInfo.end());

				// наша строчка
				if (top + height > _value.top || lastline)
				{
					top += height;
					int left = line->offset;
					int count = 0;

					// ищем символ
					for (VectorCharInfo::const_iterator sim=line->simbols.begin(); sim!=line->simbols.end(); ++sim)
					{
						if (sim->isColour())
							continue;

						if ((left + (sim->getWidth() / 2)) > _value.left)
						{
							break;
						}
						left += sim->getWidth();
						count ++;
					}

					result += count;
					break;
				}

				if (!lastline)
				{
					top += height;
					result += line->count + 1;
				}
			}

			return result;
		}

		IntPoint getCursorPoint(size_t _position)
		{
			if (_position >= mLength + 1) _position = mLength;

			size_t position = 0;
			int top = 0;
			int left = 0;
			for (VectorLineInfo::const_iterator line=mLineInfo.begin(); line!=mLineInfo.end(); ++line)
			{
				left = line->offset;
				if (position + line->count >= _position)
				{
					for (VectorCharInfo::const_iterator sim=line->simbols.begin(); sim!=line->simbols.end(); ++sim)
					{
						if (sim->isColour())
							continue;

						if (position == _position)
							break;

						position ++;
						left += sim->getWidth();
					}
					break;
				}
				position += line->count + 1;
				top += mFontHeight;
			}

			return IntPoint(left, top);
		}

		const IntSize& getViewSize() const { return mViewSize; }
		size_t getTextLength() const { return mLength; }
		const VectorLineInfo& getData() { return mLineInfo; }

	private:
		IntSize mViewSize;
		size_t mLength;
		VectorLineInfo mLineInfo;
		int mFontHeight;
	};


} // namespace MyGUI

#endif // __MYGUI_TEXT_VIEW_H__
