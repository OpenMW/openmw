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
#include "MyGUI_Precompiled.h"
#include "MyGUI_ResourceImageSet.h"
#include "MyGUI_ResourceManager.h"
#include "MyGUI_LanguageManager.h"

namespace MyGUI
{

	std::string ResourceImageSet::mTextureEmpty;
	IntSize ResourceImageSet::mSizeEmpty;
	std::vector<IntPoint> ResourceImageSet::mFramesEmpty;

	void ResourceImageSet::deserialization(xml::ElementPtr _node, Version _version)
	{
		Base::deserialization(_node, _version);

		// берем детей и крутимся, основной цикл
		xml::ElementEnumerator group_node = _node->getElementEnumerator();
		while (group_node.next("Group"))
		{
			GroupImage group;
			group.name = group_node->findAttribute("name");

			group.texture = group_node->findAttribute("texture");
			// поддержка замены тегов
			if (_version >= Version(1, 1))
			{
				group.texture = LanguageManager::getInstance().replaceTags(group.texture);
			}

			group.size = IntSize::parse(group_node->findAttribute("size"));

			xml::ElementEnumerator index_node = group_node->getElementEnumerator();
			while (index_node.next("Index"))
			{
				IndexImage index;
				index.name = index_node->findAttribute("name");
				index.rate = utility::parseFloat(index_node->findAttribute("rate"));

				xml::ElementEnumerator frame_node = index_node->getElementEnumerator();
				while (frame_node.next("Frame"))
				{
					size_t count = utility::parseSizeT(frame_node->findAttribute("count"));
					const IntPoint& point = IntPoint::parse(frame_node->findAttribute("point"));
					if ((count < 1) || (count > 256)) count = 1;
					while (count > 0)
					{
						index.frames.push_back(point);
						-- count;
					}
				}

				group.indexes.push_back(index);
			}

			mGroups.push_back(group);
		}
	}

	ImageIndexInfo ResourceImageSet::getIndexInfo(const std::string& _group, const std::string& _index)
	{
		size_t index_group = getGroupIndex(_group);
		if (index_group != ITEM_NONE)
		{
			GroupImage& group = mGroups[index_group];
			size_t index_image = getImageIndex(group, _index);
			if (index_image != ITEM_NONE)
			{
				IndexImage& index = group.indexes[index_image];
				return ImageIndexInfo(group.texture, group.size, index.rate, index.frames);
			}
		}
		return ImageIndexInfo(mTextureEmpty, mSizeEmpty, 0, mFramesEmpty);
	}

	ImageIndexInfo ResourceImageSet::getIndexInfo(size_t _group, const std::string& _index)
	{
		if (_group < mGroups.size())
		{
			GroupImage& group = mGroups[_group];
			size_t index_image = getImageIndex(group, _index);
			if (index_image != ITEM_NONE)
			{
				IndexImage& index = group.indexes[index_image];
				return ImageIndexInfo(group.texture, group.size, index.rate, index.frames);
			}
		}
		return ImageIndexInfo(mTextureEmpty, mSizeEmpty, 0, mFramesEmpty);
	}

	ImageIndexInfo ResourceImageSet::getIndexInfo(const std::string& _group, size_t _index)
	{
		size_t index_group = getGroupIndex(_group);
		if (index_group != ITEM_NONE)
		{
			GroupImage& group = mGroups[index_group];
			if (_index < group.indexes.size())
			{
				IndexImage& index = group.indexes[_index];
				return ImageIndexInfo(group.texture, group.size, index.rate, index.frames);
			}
		}
		return ImageIndexInfo(mTextureEmpty, mSizeEmpty, 0, mFramesEmpty);
	}

	ImageIndexInfo ResourceImageSet::getIndexInfo(size_t _group, size_t _index)
	{
		if (_group < mGroups.size())
		{
			GroupImage& group = mGroups[_group];
			if (_index < group.indexes.size())
			{
				IndexImage& index = group.indexes[_index];
				return ImageIndexInfo(group.texture, group.size, index.rate, index.frames);
			}
		}
		return ImageIndexInfo(mTextureEmpty, mSizeEmpty, 0, mFramesEmpty);
	}

	ImageIndexInfo ResourceImageSet::getIndexInfo(const IntSize& _group, size_t _index)
	{
		size_t index_group = getGroupIndex(_group);
		if (index_group != ITEM_NONE)
		{
			GroupImage& group = mGroups[index_group];
			if (_index < group.indexes.size())
			{
				IndexImage& index = group.indexes[_index];
				return ImageIndexInfo(group.texture, group.size, index.rate, index.frames);
			}
		}
		return ImageIndexInfo(mTextureEmpty, mSizeEmpty, 0, mFramesEmpty);
	}

	ImageIndexInfo ResourceImageSet::getIndexInfo(const IntSize& _group, const std::string& _index)
	{
		size_t index_group = getGroupIndex(_group);
		if (index_group != ITEM_NONE)
		{
			GroupImage& group = mGroups[index_group];
			size_t index_image = getImageIndex(group, _index);
			if (index_image != ITEM_NONE)
			{
				IndexImage& index = group.indexes[index_image];
				return ImageIndexInfo(group.texture, group.size, index.rate, index.frames);
			}
		}
		return ImageIndexInfo(mTextureEmpty, mSizeEmpty, 0, mFramesEmpty);
	}

} // namespace MyGUI
