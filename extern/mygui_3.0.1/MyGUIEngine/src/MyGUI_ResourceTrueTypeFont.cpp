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
#include "MyGUI_ResourceTrueTypeFont.h"
#include "MyGUI_DataManager.h"
#include "MyGUI_RenderManager.h"

#ifdef MYGUI_USE_FREETYPE
	#include <ft2build.h>
	#include FT_FREETYPE_H
	#include FT_GLYPH_H
#endif // MYGUI_USE_FREETYPE

namespace MyGUI
{

	const unsigned char FONT_MASK_SELECT = 0x88;
	const unsigned char FONT_MASK_SELECT_DEACTIVE = 0x60;
	const unsigned char FONT_MASK_SPACE = 0x00;
	const unsigned char FONT_MASK_CHAR = 0xFF;
	const size_t MIN_FONT_TEXTURE_WIDTH = 256;

	ResourceTrueTypeFont::ResourceTrueTypeFont() :
		mTtfSize(0),
		mTtfResolution(0),
		mAntialiasColour(false),
		mDistance(0),
		mSpaceWidth(0),
		mTabWidth(0),
		mCursorWidth(2),
		mSelectionWidth(2),
		mOffsetHeight(0),
		mHeightPix(0),
		mTexture(nullptr)
	{
	}

	ResourceTrueTypeFont::~ResourceTrueTypeFont()
	{
		if (mTexture != nullptr)
		{
			RenderManager::getInstance().destroyTexture(mTexture);
			mTexture = nullptr;
		}
	}

	GlyphInfo* ResourceTrueTypeFont::getGlyphInfo(Char _id)
	{
		for (VectorRangeInfo::iterator iter=mVectorRangeInfo.begin(); iter!=mVectorRangeInfo.end(); ++iter)
		{
			GlyphInfo* info = iter->getInfo(_id);
			if (info == nullptr) continue;
			return info;
		}
		// при ошибках возвращаем пробел
		return &mSpaceGlyphInfo;
	}

	void ResourceTrueTypeFont::addGlyph(GlyphInfo * _info, Char _index, int _left, int _top, int _right, int _bottom, int _finalw, int _finalh, float _aspect, int _addHeight)
	{
		_info->codePoint = _index;
		_info->uvRect.left = (float)_left / (float)_finalw;  // u1
		_info->uvRect.top = (float)(_top + _addHeight) / (float)_finalh;  // v1
		_info->uvRect.right = (float)( _right ) / (float)_finalw; // u2
		_info->uvRect.bottom = ( _bottom + _addHeight ) / (float)_finalh; // v2
		_info->width = _right - _left;
	}

	uint8* ResourceTrueTypeFont::writeData(uint8* _pDest, unsigned char _luminance, unsigned char _alpha, bool _rgba)
	{
		if (_rgba)
		{
			*_pDest++ = _luminance; // luminance
			*_pDest++ = _luminance; // luminance
			*_pDest++ = _luminance; // luminance
			*_pDest++ = _alpha; // alpha
		}
		else
		{
			*_pDest++ = _luminance; // luminance
			*_pDest++ = _alpha; // alpha
		}
		return _pDest;
	}

	void ResourceTrueTypeFont::initialise()
	{

#ifndef MYGUI_USE_FREETYPE

		MYGUI_LOG(Error, "ResourceTrueTypeFont '" << getResourceName() << "' - Ttf font disabled. Define MYGUI_USE_FREETYE if you need ttf fonts.");

#else // MYGUI_USE_FREETYPE

		mTexture = RenderManager::getInstance().createTexture(MyGUI::utility::toString((size_t)this, "_TrueTypeFont"));

		// ManualResourceLoader implementation - load the texture
		FT_Library ftLibrary;
		// Init freetype
		if ( FT_Init_FreeType( &ftLibrary ) ) MYGUI_EXCEPT("Could not init FreeType library!");

		// Load font
		FT_Face face;

		//FIXME научить работать без шрифтов
		IDataStream* datastream = DataManager::getInstance().getData(mSource);
		MYGUI_ASSERT(datastream, "Could not open font face!");

		size_t datasize = datastream->size();
		uint8* data = new uint8[datasize];
		datastream->read(data, datasize);
		delete datastream;

		if ( FT_New_Memory_Face( ftLibrary, data, (FT_Long)datasize, 0, &face ) )
			MYGUI_EXCEPT("Could not open font face!");

		// Convert our point size to freetype 26.6 fixed point format
		FT_F26Dot6 ftSize = (FT_F26Dot6)(mTtfSize * (1 << 6));
		if ( FT_Set_Char_Size( face, ftSize, 0, mTtfResolution, mTtfResolution ) )
			MYGUI_EXCEPT("Could not set char size!");

		int max_height = 0, max_bear = 0;

		int spec_len = mCursorWidth + mSelectionWidth + mSelectionWidth + mSpaceWidth + mTabWidth + (mDistance * 5);
		int len = mDistance + spec_len;
		int height = 0; // здесь используется как колличество строк

		size_t finalWidth = MIN_FONT_TEXTURE_WIDTH;
		// trying to guess necessary width for texture
		while (mTtfSize*mTtfResolution > finalWidth*6) finalWidth *= 2;

		for (VectorRangeInfo::iterator iter=mVectorRangeInfo.begin(); iter!=mVectorRangeInfo.end(); ++iter)
		{
			for (Char index=iter->first; index<=iter->last; ++index)
			{

				// символ рисовать ненужно
				if (checkHidePointCode(index)) continue;

				if (FT_Load_Char( face, index, FT_LOAD_RENDER )) continue;
				if (nullptr == face->glyph->bitmap.buffer) continue;
				FT_Int advance = (face->glyph->advance.x >> 6 ) + ( face->glyph->metrics.horiBearingX >> 6 );

				if ( ( 2 * ( face->glyph->bitmap.rows << 6 ) - face->glyph->metrics.horiBearingY ) > max_height )
					max_height = ( 2 * ( face->glyph->bitmap.rows << 6 ) - face->glyph->metrics.horiBearingY );

				if ( face->glyph->metrics.horiBearingY > max_bear )
					max_bear = face->glyph->metrics.horiBearingY;

				len += (advance + mDistance);
				if ( int(finalWidth - 1) < (len + advance + mDistance) ) { height ++; len = mDistance;}

			}
		}

		max_height >>= 6;
		max_bear >>= 6;

		size_t finalHeight = (height+1) * (max_height + mDistance) + mDistance;

		//make it more squared
		while (finalHeight > finalWidth)
		{
			finalHeight /= 2;
			finalWidth *= 2;
		}

		// вычисляем ближайшую кратную 2
		size_t needHeight = 1;
		while (needHeight < finalHeight) needHeight <<= 1;
		finalHeight = needHeight;

		float textureAspect = (float)finalWidth / (float)finalHeight;

		// if L8A8 (2 bytes per pixel) not supported use 4 bytes per pixel R8G8B8A8
		// where R == G == B == L
		bool rgbaMode = ! MyGUI::RenderManager::getInstance().isFormatSupported(PixelFormat::L8A8, TextureUsage::Static | TextureUsage::Write);

		const size_t pixel_bytes = rgbaMode ? 4 : 2;
		size_t data_width = finalWidth * pixel_bytes;
		size_t data_size = finalWidth * finalHeight * pixel_bytes;

		MYGUI_LOG(Info, "ResourceTrueTypeFont '" << getResourceName() << "' using texture size " << finalWidth << " x " << finalHeight);
		MYGUI_LOG(Info, "ResourceTrueTypeFont '" << getResourceName() << "' using real height " << max_height << " pixels");
		mHeightPix = max_height;

		uint8* imageData = new uint8[data_size];

		uint8* dest = imageData;
		// Reset content (White, transparent)
		for (size_t i = 0; i < data_size; i += pixel_bytes)
		{
			dest = writeData(dest, 0xFF, 0x00, rgbaMode);
		}

		// текущее положение в текстуре
		len = mDistance;
		height = mDistance; // здесь используется как текущее положение высоты
		FT_Int advance = 0;

		//------------------------------------------------------------------
		// создаем символ пробела
		//------------------------------------------------------------------
		advance = mSpaceWidth;

		// перевод на новую строку
		if ( int(finalWidth - 1) < (len + advance + mDistance) ) { height += max_height + mDistance; len = mDistance; }

		for (int j = 0; j < max_height; j++ )
		{
			int row = j + (int)height;
			uint8* pDest = &imageData[(row * data_width) + len * pixel_bytes];
			for (int k = 0; k < advance; k++ )
			{
				pDest = writeData(pDest, FONT_MASK_CHAR, FONT_MASK_SPACE, rgbaMode);
			}
		}

		addGlyph(&mSpaceGlyphInfo, FontCodeType::Space, len, height, len + advance, height + max_height, finalWidth, finalHeight, textureAspect, mOffsetHeight);
		len += (advance + mDistance);

		//------------------------------------------------------------------
		// создаем табуляцию
		//------------------------------------------------------------------
		advance = mTabWidth;

		// перевод на новую строку
		if ( int(finalWidth - 1) < (len + advance + mDistance) ) { height += max_height + mDistance; len = mDistance; }

		for (int j = 0; j < max_height; j++ )
		{
			int row = j + (int)height;
			uint8* pDest = &imageData[(row * data_width) + len * pixel_bytes];
			for (int k = 0; k < advance; k++ )
			{
				pDest = writeData(pDest, FONT_MASK_CHAR, FONT_MASK_SPACE, rgbaMode);
			}
		}

		addGlyph(&mTabGlyphInfo, FontCodeType::Tab, len, height, len + advance, height + max_height, finalWidth, finalHeight, textureAspect, mOffsetHeight);
		len += (advance + mDistance);

		//------------------------------------------------------------------
		// создаем выделение
		//------------------------------------------------------------------
		advance = mSelectionWidth;
		for (int j = 0; j < max_height; j++ )
		{
			int row = j + (int)height;
			uint8* pDest = &imageData[(row * data_width) + len * pixel_bytes];
			for (int k = 0; k < advance; k++ )
			{
				pDest = writeData(pDest, FONT_MASK_CHAR, FONT_MASK_SELECT, rgbaMode);
			}
		}

		// перевод на новую строку
		if ( int(finalWidth - 1) < (len + advance + mDistance) ) { height += max_height + mDistance; len = mDistance; }

		addGlyph(&mSelectGlyphInfo, FontCodeType::Selected, len, height, len + advance, height + max_height, finalWidth, finalHeight, textureAspect, mOffsetHeight);
		len += (advance + mDistance);

		//------------------------------------------------------------------
		// создаем неактивное выделение
		//------------------------------------------------------------------
		advance = mSelectionWidth;

		// перевод на новую строку
		if ( int(finalWidth - 1) < (len + advance + mDistance) ) { height += max_height + mDistance; len = mDistance; }

		for (int j = 0; j < max_height; j++ )
		{
			int row = j + (int)height;
			uint8* pDest = &imageData[(row * data_width) + len * pixel_bytes];
			for (int k = 0; k < advance; k++ )
			{
				pDest = writeData(pDest, FONT_MASK_CHAR, FONT_MASK_SELECT_DEACTIVE, rgbaMode);
			}
		}

		addGlyph(&mSelectDeactiveGlyphInfo, FontCodeType::SelectedBack, len, height, len + advance, height + max_height, finalWidth, finalHeight, textureAspect, mOffsetHeight);
		len += (advance + mDistance);

		//------------------------------------------------------------------
		// создаем курсор
		//------------------------------------------------------------------
		advance = mCursorWidth;

		// перевод на новую строку
		if ( int(finalWidth - 1) < (len + advance + mDistance) ) { height += max_height + mDistance; len = mDistance; }

		for (int j = 0; j < max_height; j++ )
		{
			int row = j + (int)height;
			uint8* pDest = &imageData[(row * data_width) + len * pixel_bytes];
			for (int k = 0; k < advance; k++ )
			{
				pDest = writeData(pDest, (k&1) ? 0 : 0xFF, FONT_MASK_CHAR, rgbaMode);
			}
		}

		addGlyph(&mCursorGlyphInfo, FontCodeType::Cursor, len, height, len + advance, height + max_height, finalWidth, finalHeight, textureAspect, mOffsetHeight);
		len += (advance + mDistance);

		//------------------------------------------------------------------
		// создаем все остальные символы
		//------------------------------------------------------------------
		FT_Error ftResult;
		for (VectorRangeInfo::iterator iter=mVectorRangeInfo.begin(); iter!=mVectorRangeInfo.end(); ++iter)
		{
			size_t pos = 0;
			for (Char index=iter->first; index<=iter->last; ++index, ++pos)
			{
				// сомвол рисовать не нада
				if (checkHidePointCode(index)) continue;

				GlyphInfo& info = iter->range.at(pos);

				ftResult = FT_Load_Char( face, index, FT_LOAD_RENDER );
				if (ftResult)
				{
					// problem loading this glyph, continue
					MYGUI_LOG(Warning, "cannot load character " << index << " in font " << getResourceName());
					continue;
				}

				FT_Int glyph_advance = (face->glyph->advance.x >> 6 );
				unsigned char* buffer = face->glyph->bitmap.buffer;

				if (nullptr == buffer)
				{
					// Yuck, FT didn't detect this but generated a nullptr pointer!
					MYGUI_LOG(Warning, "Freetype returned nullptr for character " << index << " in font " << getResourceName());
					continue;
				}

				int y_bearnig = max_bear - ( face->glyph->metrics.horiBearingY >> 6 );

				// перевод на новую строку
				if ( int(finalWidth - 1) < (len + face->glyph->bitmap.width + mDistance) ) { height += max_height + mDistance; len = mDistance; }

				for (int j = 0; j < face->glyph->bitmap.rows; j++ )
				{
					int row = j + (int)height + y_bearnig;
					uint8* pDest = &imageData[(row * data_width) + (len + ( face->glyph->metrics.horiBearingX >> 6 )) * pixel_bytes];
					for (int k = 0; k < face->glyph->bitmap.width; k++ )
					{
						if (mAntialiasColour) pDest = writeData(pDest, *buffer, *buffer, rgbaMode);
						else pDest = writeData(pDest, FONT_MASK_CHAR, *buffer, rgbaMode);
						buffer++;
					}
				}

				addGlyph(&info, index, len, height, len + glyph_advance, height + max_height, finalWidth, finalHeight, textureAspect, mOffsetHeight);
				len += (glyph_advance + mDistance);

			}
		}

		// Добавляем спец символы в основной список
		// пробел можно не добавлять, его вернет по ошибке
		RangeInfo info(FontCodeType::Selected, FontCodeType::Tab);
		info.setInfo(FontCodeType::Selected, &mSelectGlyphInfo);
		info.setInfo(FontCodeType::SelectedBack, &mSelectDeactiveGlyphInfo);
		info.setInfo(FontCodeType::Cursor, &mCursorGlyphInfo);
		info.setInfo(FontCodeType::Tab, &mTabGlyphInfo);

		mVectorRangeInfo.push_back(info);


		mTexture->createManual(finalWidth, finalHeight, TextureUsage::Static | TextureUsage::Write, rgbaMode ? PixelFormat::R8G8B8A8 : PixelFormat::L8A8);

		void* buffer_ptr = mTexture->lock(TextureUsage::Write);
		memcpy(buffer_ptr, imageData, data_size);
		mTexture->unlock();

		delete [] imageData;
		delete [] data;

		FT_Done_FreeType(ftLibrary);

#endif // MYGUI_USE_FREETYPE

	}

	void ResourceTrueTypeFont::addCodePointRange(Char _first, Char _second)
	{
		mVectorRangeInfo.push_back(RangeInfo(_first, _second));
	}

	void ResourceTrueTypeFont::addHideCodePointRange(Char _first, Char _second)
	{
		mVectorHideCodePoint.push_back(PairCodePoint(_first, _second));
	}

	// проверяет, входит ли символ в зоны ненужных символов
	bool ResourceTrueTypeFont::checkHidePointCode(Char _id)
	{
		for (VectorPairCodePoint::iterator iter=mVectorHideCodePoint.begin(); iter!=mVectorHideCodePoint.end(); ++iter)
		{
			if (iter->isExist(_id)) return true;
		}
		return false;
	}

	void ResourceTrueTypeFont::clearCodePointRanges()
	{
		mVectorRangeInfo.clear();
		mVectorHideCodePoint.clear();
	}

	void ResourceTrueTypeFont::deserialization(xml::ElementPtr _node, Version _version)
	{
		Base::deserialization(_node, _version);

		xml::ElementEnumerator node = _node->getElementEnumerator();
		while (node.next())
		{
			if (node->getName() == "Property")
			{
				const std::string& key = node->findAttribute("key");
				const std::string& value = node->findAttribute("value");
				if (key == "Source") mSource = value;
				else if (key == "Size") mTtfSize = utility::parseFloat(value);
				else if (key == "Resolution") mTtfResolution = utility::parseUInt(value);
				else if (key == "Antialias") mAntialiasColour = utility::parseBool(value);
				else if (key == "SpaceWidth") mSpaceWidth = utility::parseInt(value);
				else if (key == "TabWidth") mTabWidth = utility::parseInt(value);
				//else if (key == "CursorWidth") mCursorWidth = utility::parseInt(value);
				else if (key == "Distance") mDistance = utility::parseInt(value);
				else if (key == "OffsetHeight") mOffsetHeight = utility::parseInt(value);
			}
			else if (node->getName() == "Codes")
			{
				xml::ElementEnumerator range = node->getElementEnumerator();
				while (range.next("Code"))
				{
					std::string range_value;
					std::vector<std::string> parse_range;
					// диапазон включений
					if (range->findAttribute("range", range_value))
					{
						parse_range = utility::split(range_value);
						if (!parse_range.empty())
						{
							int first = utility::parseInt(parse_range[0]);
							int last = parse_range.size() > 1 ? utility::parseInt(parse_range[1]) : first;
							addCodePointRange(first, last);
						}
					}
					// диапазон исключений
					else if (range->findAttribute("hide", range_value))
					{
						parse_range = utility::split(range_value);
						if (!parse_range.empty())
						{
							int first = utility::parseInt(parse_range[0]);
							int last = parse_range.size() > 1 ? utility::parseInt(parse_range[1]) : first;
							addHideCodePointRange(first, last);
						}
					}
				}
			}
		}

		// инициализируем
		initialise();
	}

} // namespace MyGUI
