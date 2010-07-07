/*!
	@file
	@author		Albert Semenov
	@date		10/2008
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
#ifndef __MYGUI_BIINDEX_BASE_H__
#define __MYGUI_BIINDEX_BASE_H__

#include "MyGUI_Prerequest.h"

namespace MyGUI
{

	class BiIndexBase
	{
	public:
		virtual ~BiIndexBase() { }
	protected:

		size_t getIndexCount() { return mIndexFace.size(); }

		size_t insertItemAt(size_t _index)
		{
			#if MYGUI_DEBUG_MODE == 1
				MYGUI_ASSERT_RANGE_INSERT(_index, mIndexFace.size(), "BiIndexBase::insertItemAt");
				checkIndexes();
			#endif

			if (_index == MyGUI::ITEM_NONE) _index = mIndexFace.size();

			size_t index;

			if (_index == mIndexFace.size())
			{
				// для вставки айтема
				index = mIndexFace.size();

				mIndexFace.push_back(_index);
				mIndexBack.push_back(_index);
			}
			else
			{
				// для вставки айтема
				index = mIndexFace[_index];

				size_t count = mIndexFace.size();
				for (size_t pos=0; pos<count; ++pos)
				{
					if (mIndexFace[pos] >= index) mIndexFace[pos]++;
				}
				mIndexFace.insert(mIndexFace.begin() + _index, index);

				count ++;
				mIndexBack.push_back(0);
				for (size_t pos=0; pos<count; ++pos)
				{
					mIndexBack[mIndexFace[pos]] = pos;
				}
			}

			#if MYGUI_DEBUG_MODE == 1
				checkIndexes();
			#endif

			return index;
		}

		size_t removeItemAt(size_t _index)
		{
			#if MYGUI_DEBUG_MODE == 1
				MYGUI_ASSERT_RANGE(_index, mIndexFace.size(), "BiIndexBase::removeItemAt");
				checkIndexes();
			#endif

			// для удаления айтема
			size_t index = mIndexFace[_index];

			mIndexFace.erase(mIndexFace.begin() + _index);
			mIndexBack.pop_back();

			size_t count = mIndexFace.size();
			for (size_t pos=0; pos<count; ++pos)
			{
				if (mIndexFace[pos] > index) mIndexFace[pos]--;
				mIndexBack[mIndexFace[pos]] = pos;
			}

			#if MYGUI_DEBUG_MODE == 1
				checkIndexes();
			#endif

			return index;
		}

		void removeAllItems()
		{
			mIndexFace.clear();
			mIndexBack.clear();
		}

		// на входе индексы пользователя, на выходе реальные индексы
		size_t convertToBack(size_t _index) const
		{
			#if MYGUI_DEBUG_MODE == 1
				MYGUI_ASSERT_RANGE_AND_NONE(_index, mIndexFace.size(), "BiIndexBase::convertToBack");
			#endif
			return _index == ITEM_NONE ? ITEM_NONE : mIndexFace[_index];
		}

		// на входе индексы реальные, на выходе, то что видит пользователь
		size_t convertToFace(size_t _index) const
		{
			#if MYGUI_DEBUG_MODE == 1
				MYGUI_ASSERT_RANGE_AND_NONE(_index, mIndexFace.size(), "BiIndexBase::convertToFace");
			#endif
			return _index == ITEM_NONE ? ITEM_NONE : mIndexBack[_index];
		}

		// меняет местами два индекса, индексы со стороны пользователя
		void swapItemsFaceAt(size_t _index1, size_t _index2)
		{
			#if MYGUI_DEBUG_MODE == 1
				MYGUI_ASSERT_RANGE(_index1, mIndexFace.size(), "BiIndexBase::swapItemsFaceAt");
				MYGUI_ASSERT_RANGE(_index2, mIndexFace.size(), "BiIndexBase::swapItemsFaceAt");
			#endif

			std::swap(mIndexFace[_index1], mIndexFace[_index2]);
			std::swap(mIndexBack[mIndexFace[_index1]], mIndexBack[mIndexFace[_index2]]);
		}

		// меняет местами два индекса, индексы со сторонны данных
		void swapItemsBackAt(size_t _index1, size_t _index2)
		{
			#if MYGUI_DEBUG_MODE == 1
				MYGUI_ASSERT_RANGE(_index1, mIndexFace.size(), "BiIndexBase::swapItemsBackAt");
				MYGUI_ASSERT_RANGE(_index2, mIndexFace.size(), "BiIndexBase::swapItemsBackAt");
			#endif

			std::swap(mIndexBack[_index1], mIndexBack[_index2]);
			std::swap(mIndexFace[mIndexBack[_index1]], mIndexFace[mIndexBack[_index2]]);
		}

		#if MYGUI_DEBUG_MODE == 1

		void checkIndexes()
		{
			assert(mIndexFace.size() == mIndexBack.size());

			// проверяем на уникальность каждого индекса в маппинге
			std::vector<bool> vec;
			size_t count = mIndexFace.size();

			vec.reserve(count);
			for (size_t pos=0; pos<count; ++pos) vec.push_back(false);

			for (size_t pos=0; pos<count; ++pos)
			{
				// максимум
				size_t index = mIndexBack[pos];
				if (index >= count) throw new std::exception();

				// максимум
				index = mIndexFace[pos];
				if (index >= count) throw new std::exception();

				if (vec[index]) throw new std::exception();
				vec[index] = true;
			}

			for (size_t pos=0; pos<count; ++pos)
			{
				if (!vec[pos]) throw new std::exception();
			}

			// проверяем на взаимоссылаемость индексов
			for (size_t pos=0; pos<count; ++pos)
			{
				size_t index = mIndexFace[pos];
				if (mIndexBack[index] != pos) throw new std::exception();
			}
		}

		#endif

	private:
		typedef std::vector<size_t> VectorSizeT;

		// маппинг с индексов, которые видны наружу
		// на индексы которые реально используются данными
		VectorSizeT mIndexFace;

		// маппинг с индексов, которые используют данные
		// на индексы которые виндны наружу
		VectorSizeT mIndexBack;
	};

} // namespace MyGUI

#endif // __MYGUI_BIINDEX_BASE_H__
