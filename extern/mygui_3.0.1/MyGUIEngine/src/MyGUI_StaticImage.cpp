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
#include "MyGUI_StaticImage.h"
#include "MyGUI_CoordConverter.h"
#include "MyGUI_ResourceManager.h"
#include "MyGUI_ResourceSkin.h"
#include "MyGUI_RotatingSkin.h"
#include "MyGUI_Gui.h"
#include "MyGUI_TextureUtility.h"

namespace MyGUI
{

	const size_t IMAGE_MAX_INDEX = 256;

	StaticImage::StaticImage() :
		mIndexSelect(ITEM_NONE),
		mFrameAdvise(false),
		mCurrentTime(0),
		mCurrentFrame(0),
		mResource(nullptr)
	{
	}

	void StaticImage::_initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name)
	{
		Base::_initialise(_style, _coord, _align, _info, _parent, _croppedParent, _creator, _name);

		initialiseWidgetSkin(_info);
	}

	StaticImage::~StaticImage()
	{
		shutdownWidgetSkin();
	}

	void StaticImage::baseChangeWidgetSkin(ResourceSkin* _info)
	{
		shutdownWidgetSkin();
		Base::baseChangeWidgetSkin(_info);
		initialiseWidgetSkin(_info);
	}

	void StaticImage::initialiseWidgetSkin(ResourceSkin* _info)
	{
		// парсим свойства
		const MapString& properties = _info->getProperties();
		if ( ! properties.empty() )
		{
			MapString::const_iterator iter = properties.end();
			if ((iter = properties.find("ImageTexture")) != properties.end()) setImageTexture(iter->second);
			if ((iter = properties.find("ImageRect")) != properties.end()) setImageRect(IntRect::parse(iter->second));
			if ((iter = properties.find("ImageCoord")) != properties.end()) setImageCoord(IntCoord::parse(iter->second));
			if ((iter = properties.find("ImageTile")) != properties.end()) setImageTile(IntSize::parse(iter->second));
			if ((iter = properties.find("ImageIndex")) != properties.end()) setImageIndex(utility::parseInt(iter->second));
			if ((iter = properties.find("ImageResource")) != properties.end()) setItemResource(iter->second);
			if ((iter = properties.find("ImageGroup")) != properties.end()) setItemGroup(iter->second);
			if ((iter = properties.find("ImageName")) != properties.end()) setItemName(iter->second);
		}
	}

	void StaticImage::shutdownWidgetSkin()
	{
		frameAdvise(false);
	}

	void StaticImage::setImageInfo(const std::string& _texture, const IntCoord& _coord, const IntSize& _tile)
	{
		mCurrentTextureName = _texture;
		mSizeTexture = texture_utility::getTextureSize(mCurrentTextureName);

		mSizeTile = _tile;
		mRectImage.left = _coord.left;
		mRectImage.top = _coord.top;
		mRectImage.right = _coord.left + _coord.width;
		mRectImage.bottom = _coord.top + _coord.height;

		recalcIndexes();
		updateSelectIndex(mIndexSelect);
	}

	void StaticImage::setImageTile(const IntSize& _tile)
	{
		mSizeTile = _tile;

		// если размер еще не установлен, то ставим тот что у тайла
		if (mRectImage.empty()) mRectImage.set(0, 0, _tile.width, _tile.height);
		//если индекса еще нет, то ставим 0
		if (mIndexSelect == ITEM_NONE) mIndexSelect = 0;

		recalcIndexes();
		updateSelectIndex(mIndexSelect);
	}

	void StaticImage::setImageCoord(const IntCoord& _coord)
	{
		mRectImage.left = _coord.left;
		mRectImage.top = _coord.top;
		mRectImage.right = _coord.left + _coord.width;
		mRectImage.bottom = _coord.top + _coord.height;

		// если тайл еще не установлен, то ставим тот что у координат
		if (mSizeTile.empty()) mSizeTile = _coord.size();
		//если индекса еще нет, то ставим 0
		if (mIndexSelect == ITEM_NONE) mIndexSelect = 0;

		recalcIndexes();
		updateSelectIndex(mIndexSelect);
	}

	void StaticImage::setImageRect(const IntRect& _rect)
	{
		mRectImage= _rect;

		// если тайл еще не установлен, то ставим тот что у координат
		if (mSizeTile.empty()) mSizeTile.set(_rect.width(), _rect.height());
		//если индекса еще нет, то ставим 0
		if (mIndexSelect == ITEM_NONE) mIndexSelect = 0;

		recalcIndexes();
		updateSelectIndex(mIndexSelect);
	}

	void StaticImage::setImageTexture(const std::string& _texture)
	{
		mCurrentTextureName = _texture;
		mSizeTexture = texture_utility::getTextureSize(mCurrentTextureName);

		// если первый раз, то ставим во всю текстуру
		if (mItems.empty())
		{
			_setUVSet(FloatRect(0, 0, 1, 1));
			_setTextureName(mCurrentTextureName);
		}
		else
		{
			recalcIndexes();
			updateSelectIndex(mIndexSelect);
		}
	}

	void StaticImage::recalcIndexes()
	{
		mItems.clear();

		if ((mRectImage.right <= mRectImage.left) || (mRectImage.bottom <= mRectImage.top)) return;
		if ((mSizeTile.width <= 0) || (mSizeTile.height <= 0)) return;

		size_t count_h = (size_t)(mRectImage.width() / mSizeTile.width);
		size_t count_v = (size_t)(mRectImage.height() / mSizeTile.height);

		if ((count_h * count_v) > IMAGE_MAX_INDEX)
		{
			MYGUI_LOG(Warning, "Tile count very mach, rect : " << mRectImage.print() << " tile : " << mSizeTile.print() << " texture : " << mTextureName << " indexes : " << (count_h * count_v) << " max : " << IMAGE_MAX_INDEX);
			return;
		}

		int pos_h = mRectImage.left;
		int pos_v = mRectImage.top;

		for (size_t v=0; v<count_v; ++v)
		{
			for (size_t h=0; h<count_h; ++h)
			{
				addItem(IntCoord(pos_h, pos_v, mSizeTile.width, mSizeTile.height));
				pos_h += mSizeTile.width;
			}
			pos_v += mSizeTile.height;
			pos_h = mRectImage.left;
		}
	}

	void StaticImage::updateSelectIndex(size_t _index)
	{
		mIndexSelect = _index;

		if ((_index == ITEM_NONE) || (_index >= mItems.size()))
		{
			_setTextureName("");
			return;
		}
		else
		{
			_setTextureName(mCurrentTextureName);
		}

		VectorImages::iterator iter = mItems.begin() + _index;

		if (iter->images.size() < 2)
		{
			frameAdvise(false);
		}
		else
		{
			if ( ! mFrameAdvise)
			{
				mCurrentTime = 0;
				mCurrentFrame = 0;
			}
			frameAdvise(true);
		}

		if ( ! iter->images.empty())
		{
			_setUVSet(iter->images.front());
		}
	}

	void StaticImage::deleteItem(size_t _index)
	{
		MYGUI_ASSERT_RANGE(_index, mItems.size(), "StaticImage::deleteItem");

		mItems.erase(mItems.begin() + _index);

		if (mIndexSelect != ITEM_NONE)
		{
			if (mItems.empty()) updateSelectIndex(ITEM_NONE);
			else if ((_index < mIndexSelect) || (mIndexSelect == mItems.size())) updateSelectIndex(mIndexSelect--);
		}
	}

	void StaticImage::deleteAllItems()
	{
		updateSelectIndex(ITEM_NONE);
		mItems.clear();
	}

	void StaticImage::insertItem(size_t _index, const IntCoord& _item)
	{
		MYGUI_ASSERT_RANGE_INSERT(_index, mItems.size(), "StaticImage::insertItem");
		if (_index == ITEM_NONE) _index = mItems.size();

		VectorImages::iterator iter = mItems.insert(mItems.begin() + _index, ImageItem());

		iter->images.push_back(CoordConverter::convertTextureCoord(_item, mSizeTexture));

		if ((mIndexSelect != ITEM_NONE) && (_index <= mIndexSelect)) updateSelectIndex(mIndexSelect++);
	}

	void StaticImage::setItem(size_t _index, const IntCoord& _item)
	{
		MYGUI_ASSERT_RANGE(_index, mItems.size(), "StaticImage::setItem");

		VectorImages::iterator iter = mItems.begin() + _index;
		iter->images.clear();
		iter->images.push_back(CoordConverter::convertTextureCoord(_item, mSizeTexture));

		if (_index == mIndexSelect) updateSelectIndex(mIndexSelect);
	}

	void StaticImage::frameEntered(float _frame)
	{
		if (mIndexSelect == ITEM_NONE) return;

		if (mItems.empty()) return;
		VectorImages::iterator iter = mItems.begin() + mIndexSelect;
		if ((iter->images.size() < 2) || (iter->frame_rate == 0)) return;

		mCurrentTime += _frame;

		while (mCurrentTime >= iter->frame_rate)
		{
			mCurrentTime -= iter->frame_rate;
			mCurrentFrame ++;
			if (mCurrentFrame >= (iter->images.size())) mCurrentFrame = 0;
		}

		_setUVSet(iter->images[mCurrentFrame]);
	}

	void StaticImage::deleteAllItemFrames(size_t _index)
	{
		MYGUI_ASSERT_RANGE(_index, mItems.size(), "StaticImage::clearItemFrame");
		VectorImages::iterator iter = mItems.begin() + _index;
		iter->images.clear();
	}

	void StaticImage::addItemFrame(size_t _index, const IntCoord& _item)
	{
		MYGUI_ASSERT_RANGE(_index, mItems.size(), "StaticImage::addItemFrame");
		VectorImages::iterator iter = mItems.begin() + _index;
		iter->images.push_back(CoordConverter::convertTextureCoord(_item, mSizeTexture));
	}

	void StaticImage::setItemFrameRate(size_t _index, float _rate)
	{
		MYGUI_ASSERT_RANGE(_index, mItems.size(), "StaticImage::setItemFrameRate");
		VectorImages::iterator iter = mItems.begin() + _index;
		iter->frame_rate = _rate;
	}

	float StaticImage::getItemFrameRate(size_t _index)
	{
		MYGUI_ASSERT_RANGE(_index, mItems.size(), "StaticImage::getItemFrameRate");
		VectorImages::iterator iter = mItems.begin() + _index;
		return iter->frame_rate;
	}

	void StaticImage::addItemFrameDublicate(size_t _index, size_t _indexSourceFrame)
	{
		MYGUI_ASSERT_RANGE(_index, mItems.size(), "StaticImage::addItemFrameDublicate");

		VectorImages::iterator iter = mItems.begin() + _index;
		MYGUI_ASSERT_RANGE(_indexSourceFrame, iter->images.size(), "StaticImage::addItemFrameDublicate");
		iter->images.push_back(iter->images[_indexSourceFrame]);
	}

	void StaticImage::insertItemFrame(size_t _index, size_t _indexFrame, const IntCoord& _item)
	{
		MYGUI_ASSERT_RANGE(_index, mItems.size(), "StaticImage::insertItemFrame");

		VectorImages::iterator iter = mItems.begin() + _index;
		MYGUI_ASSERT_RANGE_INSERT(_indexFrame, iter->images.size(), "StaticImage::insertItemFrame");
		if (_indexFrame == ITEM_NONE) _indexFrame = iter->images.size() - 1;

		iter->images.insert(iter->images.begin() + _indexFrame,
			CoordConverter::convertTextureCoord(_item, mSizeTexture));
	}

	void StaticImage::insertItemFrameDublicate(size_t _index, size_t _indexFrame, size_t _indexSourceFrame)
	{
		MYGUI_ASSERT_RANGE(_index, mItems.size(), "StaticImage::insertItemFrameDublicate");

		VectorImages::iterator iter = mItems.begin() + _index;
		MYGUI_ASSERT_RANGE_INSERT(_indexFrame, iter->images.size(), "StaticImage::insertItemFrameDublicate");
		if (_indexFrame == ITEM_NONE) _indexFrame = iter->images.size() - 1;

		MYGUI_ASSERT_RANGE(_indexSourceFrame, iter->images.size(), "StaticImage::insertItemFrameDublicate");

		iter->images.insert(iter->images.begin() + _indexFrame, iter->images[_indexSourceFrame]);
	}

	void StaticImage::setItemFrame(size_t _index, size_t _indexFrame, const IntCoord& _item)
	{
		MYGUI_ASSERT_RANGE(_index, mItems.size(), "StaticImage::setItemFrame");

		VectorImages::iterator iter = mItems.begin() + _index;
		MYGUI_ASSERT_RANGE(_indexFrame, iter->images.size(), "StaticImage::setItemFrame");

		iter->images[_indexFrame] = CoordConverter::convertTextureCoord(_item, mSizeTexture);
	}

	void StaticImage::deleteItemFrame(size_t _index, size_t _indexFrame)
	{
		MYGUI_ASSERT_RANGE(_index, mItems.size(), "StaticImage::deleteItemFrame");

		VectorImages::iterator iter = mItems.begin() + _index;
		MYGUI_ASSERT_RANGE_INSERT(_indexFrame, iter->images.size(), "StaticImage::deleteItemFrame");
		if (_indexFrame == ITEM_NONE) _indexFrame = iter->images.size() - 1;

		iter->images.erase(iter->images.begin() + _indexFrame);
	}

	void StaticImage::setItemResourceInfo(const ImageIndexInfo& _info)
	{
		mCurrentTextureName = _info.texture;
		mSizeTexture = texture_utility::getTextureSize(mCurrentTextureName);

		mItems.clear();

		if (_info.frames.size() != 0)
		{
			std::vector<IntPoint>::const_iterator iter = _info.frames.begin();

			addItem(IntCoord(*iter, _info.size));
			setItemFrameRate(0, _info.rate);

			for (++iter; iter!=_info.frames.end(); ++iter)
			{
				addItemFrame(0, MyGUI::IntCoord(*iter, _info.size));
			}

		}

		mIndexSelect = 0;
		updateSelectIndex(mIndexSelect);
	}

	bool StaticImage::setItemResource(const Guid& _id)
	{
		IResourcePtr resource = _id.empty() ? nullptr : ResourceManager::getInstance().getByID(_id, false);
		setItemResourcePtr(resource ? resource->castType<ResourceImageSet>() : nullptr);
		return resource != nullptr;
	}

	bool StaticImage::setItemResource(const std::string& _name)
	{
		IResourcePtr resource = ResourceManager::getInstance().getByName(_name, false);
		setItemResourcePtr(resource ? resource->castType<ResourceImageSet>() : nullptr);
		return resource != nullptr;
	}

	void StaticImage::setItemResourcePtr(ResourceImageSetPtr _resource)
	{
		if (mResource == _resource)
			return;

		// если первый раз то устанавливаем дефолтное
		if (mResource == nullptr && _resource != nullptr)
		{
			if (mItemGroup.empty())
			{
				EnumeratorGroupImage iter_group = _resource->getEnumerator();
				while (iter_group.next())
				{
					mItemGroup = iter_group.current().name;
					if (mItemName.empty() && !iter_group.current().indexes.empty())
					{
						mItemName = iter_group.current().indexes[0].name;
					}
					break;
				}
			}
			else if (mItemName.empty())
			{
				EnumeratorGroupImage iter_group = _resource->getEnumerator();
				while (iter_group.next())
				{
					if (mItemGroup == iter_group.current().name)
					{
						if (!iter_group.current().indexes.empty())
						{
							mItemName = iter_group.current().indexes[0].name;
							break;
						}
					}
				}
			}
		}

		mResource = _resource;
		if (!mResource || mItemGroup.empty() || mItemName.empty()) updateSelectIndex(ITEM_NONE);
		else setItemResourceInfo(mResource->getIndexInfo(mItemGroup, mItemName));
	}

	void StaticImage::setItemGroup(const std::string& _group)
	{
		if (mItemGroup == _group)
			return;

		mItemGroup = _group;
		if (!mResource || mItemGroup.empty() || mItemName.empty()) updateSelectIndex(ITEM_NONE);
		else setItemResourceInfo(mResource->getIndexInfo(mItemGroup, mItemName));
	}

	void StaticImage::setItemName(const std::string& _name)
	{
		if (mItemName == _name)
			return;

		mItemName = _name;
		if (!mResource || mItemGroup.empty() || mItemName.empty()) updateSelectIndex(ITEM_NONE);
		else setItemResourceInfo(mResource->getIndexInfo(mItemGroup, mItemName));
	}

	void StaticImage::setItemResourceInfo(ResourceImageSetPtr _resource, const std::string& _group, const std::string& _name)
	{
		mResource = _resource;
		mItemGroup = _group;
		mItemName = _name;
		if (!mResource || mItemGroup.empty() || mItemName.empty()) updateSelectIndex(ITEM_NONE);
		else setItemResourceInfo(mResource->getIndexInfo(mItemGroup, mItemName));
	}

	void StaticImage::frameAdvise(bool _advise)
	{
		if ( _advise )
		{
			if ( ! mFrameAdvise )
			{
				MyGUI::Gui::getInstance().eventFrameStart += MyGUI::newDelegate( this, &StaticImage::frameEntered );
				mFrameAdvise = true;
			}
		}
		else
		{
			if ( mFrameAdvise )
			{
				MyGUI::Gui::getInstance().eventFrameStart -= MyGUI::newDelegate( this, &StaticImage::frameEntered );
				mFrameAdvise = false;
			}
		}
	}

	void StaticImage::setImageIndex(size_t _index)
	{
		setItemSelect(_index);
	}

	size_t StaticImage::getImageIndex()
	{
		return getItemSelect();
	}

	void StaticImage::setItemSelect(size_t _index)
	{
		if (mIndexSelect != _index) updateSelectIndex(_index);
	}

	void StaticImage::setProperty(const std::string& _key, const std::string& _value)
	{
		if (_key == "Image_Texture") setImageTexture(_value);
		else if (_key == "Image_Coord") setImageCoord(utility::parseValue<IntCoord>(_value));
		else if (_key == "Image_Tile") setImageTile(utility::parseValue<IntSize>(_value));
		else if (_key == "Image_Index") setItemSelect(utility::parseValue<size_t>(_value));
		else if (_key == "Image_Resource") setItemResource(_value);
		else if (_key == "Image_Group") setItemGroup(_value);
		else if (_key == "Image_Name") setItemName(_value);
		else
		{
			Base::setProperty(_key, _value);
			return;
		}
		eventChangeProperty(this, _key, _value);
	}

} // namespace MyGUI
