/*!
	@file
	@author		Albert Semenov
	@date		06/2008
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
#ifndef __MYGUI_RESOURCE_MANUAL_FONT_H__
#define __MYGUI_RESOURCE_MANUAL_FONT_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_ITexture.h"
#include "MyGUI_IFont.h"

namespace MyGUI
{

	class MYGUI_EXPORT ResourceManualFont :
		public IFont
	{
		MYGUI_RTTI_DERIVED( ResourceManualFont )

	private:
		typedef std::vector<RangeInfo> VectorRangeInfo;
		typedef std::vector<PairCodeCoord> VectorPairCodeCoord;

	public:
		ResourceManualFont();
		virtual ~ResourceManualFont();

		virtual void deserialization(xml::ElementPtr _node, Version _version);

		virtual GlyphInfo* getGlyphInfo(Char _id);

		virtual ITexture* getTextureFont() { return mTexture; }

		// дефолтная высота, указанная в настройках шрифта
		virtual int getDefaultHeight() { return mDefaultHeight; }

	private:
		void addGlyph(Char _index, const IntCoord& _coord);

		void initialise();

		void addGlyph(GlyphInfo * _info, Char _index, int _left, int _top, int _right, int _bottom, int _finalw, int _finalh, float _aspect, int _addHeight = 0);

		void addRange(VectorPairCodeCoord& _info, size_t _first, size_t _last, int _width, int _height, float _aspect);
		void checkTexture();

	private:
		std::string mSource;
		int mDefaultHeight;

		// отдельная информация о символах
		GlyphInfo mSpaceGlyphInfo;

		// символы созданные руками
		VectorPairCodeCoord mVectorPairCodeCoord;

		// вся информация о символах
		VectorRangeInfo mVectorRangeInfo;

		MyGUI::ITexture* mTexture;
	};

} // namespace MyGUI

#endif // __MYGUI_RESOURCE_MANUAL_FONT_H__
