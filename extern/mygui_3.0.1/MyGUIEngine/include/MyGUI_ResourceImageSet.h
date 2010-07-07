/*!
	@file
	@author		Albert Semenov
	@date		09/2008
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
#ifndef __MYGUI_RESOURCE_IMAGE_SET_H__
#define __MYGUI_RESOURCE_IMAGE_SET_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Macros.h"
#include "MyGUI_XmlDocument.h"
#include "MyGUI_IResource.h"
#include "MyGUI_ImageInfo.h"
#include "MyGUI_Enumerator.h"
#include "MyGUI_ResourceManager.h"
#include "MyGUI_GenericFactory.h"

namespace MyGUI
{

	class ResourceImageSet;
	typedef ResourceImageSet * ResourceImageSetPtr;

	struct IndexImage
	{
		std::string name;
		float rate;
		std::vector<IntPoint> frames;
	};
	typedef std::vector<IndexImage> VectorIndexImage;

	struct GroupImage
	{
		std::string name;
		std::string texture;
		IntSize size;
		VectorIndexImage indexes;
	};
	typedef std::vector<GroupImage> VectorGroupImage;
	typedef Enumerator<VectorGroupImage> EnumeratorGroupImage;

	class MYGUI_EXPORT ResourceImageSet :
		public IResource
	{
		friend class GenericFactory<ResourceImageSet>;

		MYGUI_RTTI_DERIVED( ResourceImageSet )

	private:
		ResourceImageSet() { }
		virtual ~ResourceImageSet() { }

		virtual void deserialization(xml::ElementPtr _node, Version _version);

		//-------------------------------------------------------------//
	private:
		size_t getGroupIndex(const std::string& _name)
		{
			for (size_t index=0; index<mGroups.size(); ++index)
			{
				if (mGroups[index].name == _name) return index;
			}
			return ITEM_NONE;
		}

		size_t getGroupIndex(const IntSize& _size)
		{
			for (size_t index=0; index<mGroups.size(); ++index)
			{
				if (mGroups[index].size == _size) return index;
			}
			return ITEM_NONE;
		}

		size_t getImageIndex(GroupImage& _group, const std::string& _name)
		{
			VectorIndexImage& indexes = _group.indexes;
			for (size_t index=0; index<indexes.size(); ++index)
			{
				if (indexes[index].name == _name) return index;
			}
			return ITEM_NONE;
		}

		const IntSize& getGroupSize(size_t _index)
		{
			if (_index >= mGroups.size()) return mSizeEmpty;
			return mGroups[_index].size;
		}

		const IntSize& getGroupSize(const std::string& _group)
		{
			for (size_t index=0; index<mGroups.size(); ++index)
			{
				if (mGroups[index].name == _group) return mGroups[index].size;
			}
			return mSizeEmpty;
		}

	public:
		ImageIndexInfo getIndexInfo(const std::string& _group, const std::string& _index);
		ImageIndexInfo getIndexInfo(size_t _group, const std::string& _index);
		ImageIndexInfo getIndexInfo(const std::string& _group, size_t _index);
		ImageIndexInfo getIndexInfo(size_t _group, size_t _index);
		ImageIndexInfo getIndexInfo(const IntSize& _group, size_t _index);
		ImageIndexInfo getIndexInfo(const IntSize& _group, const std::string& _index);

		/** Get groups Enumerator */
		EnumeratorGroupImage getEnumerator() { return EnumeratorGroupImage(mGroups); }

	private:
		VectorGroupImage mGroups;

		static std::string mTextureEmpty;
		static IntSize mSizeEmpty;
		static std::vector<IntPoint> mFramesEmpty;

	};

} // namespace MyGUI

#endif // __MYGUI_RESOURCE_IMAGE_SET_H__
