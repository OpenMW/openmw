/*!
	@file
	@author		Albert Semenov
	@date		06/2009
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
#include "MyGUI_ResourceManualFont.h"
#include "MyGUI_SkinManager.h"
#include "MyGUI_RenderManager.h"
#include "MyGUI_TextureUtility.h"

namespace MyGUI
{

	ResourceManualFont::ResourceManualFont() :
		mDefaultHeight(0),
		mTexture(nullptr)
	{
	}

	ResourceManualFont::~ResourceManualFont()
	{
	}

	GlyphInfo * ResourceManualFont::getGlyphInfo(Char _id)
	{
		for (VectorRangeInfo::iterator iter=mVectorRangeInfo.begin(); iter!=mVectorRangeInfo.end(); ++iter)
		{
			GlyphInfo * info = iter->getInfo(_id);
			if (info == nullptr) continue;
			return info;
		}
		// при ошибках возвращаем пробел
		return &mSpaceGlyphInfo;
	}

	void ResourceManualFont::checkTexture()
	{
		if (mTexture == nullptr)
		{
			RenderManager& render = RenderManager::getInstance();
			mTexture = render.getTexture(mSource);
			if (mTexture == nullptr)
			{
				mTexture = render.createTexture(mSource);
				mTexture->loadFromFile(mSource);
			}
		}
	}

	void ResourceManualFont::addGlyph(GlyphInfo * _info, Char _index, int _left, int _top, int _right, int _bottom, int _finalw, int _finalh, float _aspect, int _addHeight)
	{
		_info->codePoint = _index;
		_info->uvRect.left = (float)_left / (float)_finalw;  // u1
		_info->uvRect.top = (float)(_top + _addHeight) / (float)_finalh;  // v1
		_info->uvRect.right = (float)( _right ) / (float)_finalw; // u2
		_info->uvRect.bottom = ( _bottom + _addHeight ) / (float)_finalh; // v2
		_info->width = _right - _left;
	}

	void ResourceManualFont::addGlyph(Char _code, const IntCoord& _coord)
	{
		mVectorPairCodeCoord.push_back(PairCodeCoord(_code, _coord));
	}

	void ResourceManualFont::initialise()
	{
		if (mVectorPairCodeCoord.empty()) return;

		std::sort(mVectorPairCodeCoord.begin(), mVectorPairCodeCoord.end());

		const IntSize& size = texture_utility::getTextureSize(mSource);
		float aspect = (float)size.width / (float)size.height;

		Char code = mVectorPairCodeCoord.front().code;
		size_t count = mVectorPairCodeCoord.size();
		size_t first = 0;

		for (size_t pos=1; pos<count; ++pos)
		{
			// диапазон оборвался
			if (code + 1 != mVectorPairCodeCoord[pos].code)
			{
				addRange(mVectorPairCodeCoord, first, pos-1, size.width, size.height, aspect);
				code = mVectorPairCodeCoord[pos].code;
				first = pos;
			}
			else
			{
				code ++;
			}
		}

		addRange(mVectorPairCodeCoord, first, count-1, size.width, size.height, aspect);

		// уничтожаем буфер
		VectorPairCodeCoord tmp;
		std::swap(tmp, mVectorPairCodeCoord);

		checkTexture();
	}

	void ResourceManualFont::addRange(VectorPairCodeCoord& _info, size_t _first, size_t _last, int _width, int _height, float _aspect)
	{
		RangeInfo range = RangeInfo(_info[_first].code, _info[_last].code);

		for (size_t pos=_first; pos<=_last; ++pos)
		{
			GlyphInfo * info = range.getInfo(_info[pos].code);
			const IntCoord& coord = _info[pos].coord;
			addGlyph(info, _info[pos].code, coord.left, coord.top, coord.right(), coord.bottom(), _width, _height, _aspect);

			if (_info[pos].code == FontCodeType::Space)
				mSpaceGlyphInfo = *info;
		}

		mVectorRangeInfo.push_back(range);
	}

	void ResourceManualFont::deserialization(xml::ElementPtr _node, Version _version)
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
				else if (key == "DefaultHeight") mDefaultHeight = utility::parseInt(value);
			}
			else if (node->getName() == "Codes")
			{
				xml::ElementEnumerator range = node->getElementEnumerator();
				while (range.next("Code"))
				{
					std::string range_value;
					std::vector<std::string> parse_range;
					// описане глифов
					if (range->findAttribute("index", range_value))
					{
						Char id = 0;
						if (range_value == "cursor")
							id = FontCodeType::Cursor;
						else if (range_value == "selected")
							id = FontCodeType::Selected;
						else if (range_value == "selected_back")
							id = FontCodeType::SelectedBack;
						else
							id = utility::parseUInt(range_value);

						addGlyph(id, utility::parseValue<IntCoord>(range->findAttribute("coord")));
					}
				}
			}
		}

		// инициализируем
		initialise();
	}

} // namespace MyGUI
