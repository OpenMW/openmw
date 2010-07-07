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
#ifndef __MYGUI_STATIC_IMAGE_H__
#define __MYGUI_STATIC_IMAGE_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Widget.h"
#include "MyGUI_ResourceImageSet.h"
#include "MyGUI_ImageInfo.h"
#include "MyGUI_Guid.h"

namespace MyGUI
{

	class MYGUI_EXPORT StaticImage :
		public Widget
	{
		MYGUI_RTTI_DERIVED( StaticImage )

	public:
		StaticImage();

		//------------------------------------------------------------------------------//
		// The simple interface
		//------------------------------------------------------------------------------//

		/* Set texture and size of image _tile
			@param _texture file name or texture name
			@param _coord - part of texture where we take tiles
			@param _tile size
		*/
		void setImageInfo(const std::string& _texture, const IntCoord& _coord, const IntSize& _tile);

		/* Set texture
			@param _texture file name or texture name
		*/
		void setImageTexture(const std::string& _value);

		/** Set _rect - part of texture where we take tiles */
		void setImageRect(const IntRect& _value);

		/** Set _coord - part of texture where we take tiles */
		void setImageCoord(const IntCoord& _value);

		/** Set _tile size */
		void setImageTile(const IntSize& _value);

		/** Set current tile index
			@param _index - tile index
			@remarks Tiles in file start numbering from left to right and from top to bottom.
			\n \bExample:\n
			<pre>
				+---+---+---+
				| 0 | 1 | 2 |
				+---+---+---+
				| 3 | 4 | 5 |
				+---+---+---+
			</pre>
		*/
		void setImageIndex(size_t _index);
		/** Get current tile index */
		size_t getImageIndex();

		//------------------------------------------------------------------------------//
		// The expanded interface
		//------------------------------------------------------------------------------//

		//! Get number of items
		size_t getItemCount() const { return mItems.size(); }

		//! Select specified _index
		void setItemSelect(size_t _index);
		//! Get index of selected item (ITEM_NONE if none selected)
		size_t getItemSelect() { return mIndexSelect; }
		//! Reset item selection
		void resetItemSelect() { setItemSelect(ITEM_NONE); }

		//! Insert an item into a list at a specified position
		void insertItem(size_t _index, const IntCoord& _item);
		//! Add an item to the end of a list
		void addItem(const IntCoord& _item) { insertItem(ITEM_NONE, _item); }
		//! Replace an item at a specified position
		void setItem(size_t _index, const IntCoord& _item);

		//! Delete item at a specified position
		void deleteItem(size_t _index);
		//! Delete all items
		void deleteAllItems();

		// работа с фреймами анимированных индексов
		/** Add frame
			@param _index Image item index
			@param _item Frame coordinates at image texture
		*/
		void addItemFrame(size_t _index, const IntCoord& _item);

		/** Insert frame
			@param _index Image item index
			@param _indexFrame Frame index where we insert frame
			@param _item Frame coordinates at image texture
		*/
		void insertItemFrame(size_t _index, size_t _indexFrame, const IntCoord& _item);

		/** Add copy of frame (similar to StaticImage::addItemFrame but we copy frame coordinates)
			@param _index Image item index
			@param _indexSourceFrame Frame index of frame that we copying
		*/
		void addItemFrameDublicate(size_t _index, size_t _indexSourceFrame);

		/** Insert copy of frame (similar to StaticImage::insertItemFrame but we copy frame coordinates)
			@param _index Image item index
			@param _indexFrame Frame index where we insert frame
			@param _indexSourceFrame Frame index of frame that we copying
		*/
		void insertItemFrameDublicate(size_t _index, size_t _indexFrame, size_t _indexSourceFrame);

		/** Change frame
			@param _index Image item index
			@param _indexFrame Frame index to change
			@param _item Frame coordinates at image texture
		*/
		void setItemFrame(size_t _index, size_t _indexFrame, const IntCoord& _item);

		/** Delete frame
			@param _index Image item index
			@param _indexFrame Frame index that we delete
		*/
		void deleteItemFrame(size_t _index, size_t _indexFrame);

		/** Delete all frames
			@param _index Image item index
		*/
		void deleteAllItemFrames(size_t _index);

		/** Set item frame rate
			@param _index Image item index
			@param _rate Duration of one frame in seconds
		*/
		void setItemFrameRate(size_t _index, float _rate);

		/** Get item frame rate
			@param _index Image item index
		*/
		float getItemFrameRate(size_t _index);

		//------------------------------------------------------------------------------//
		// The interface with support of resources
		//------------------------------------------------------------------------------//

		/** Select current items resource used in StaticImage
			@param _id Resource guid
			@return false if resource with such guid not exist
		*/
		bool setItemResource(const Guid& _id);

		/** Select current items resource used in StaticImage
			@param _id Resource name
			@return false if resource with such name not exist
		*/
		bool setItemResource(const std::string& _name);

		/** Select current item group */
		void setItemGroup(const std::string& _value);
		/** Select current item mane */
		void setItemName(const std::string& _value);

		/** Select current items resource used in StaticImage
			@param _resource Resource pointer
		*/
		void setItemResourcePtr(ResourceImageSetPtr _resource);
		/** Set current item */
		void setItemResourceInfo(const ImageIndexInfo& _info);

		/** Get current items resource used in StaticImage */
		ResourceImageSetPtr getItemResource() { return mResource; }
		/** Select current item resource, group and name */
		void setItemResourceInfo(ResourceImageSetPtr _resource, const std::string& _group, const std::string& _name);

		/** @copydoc Widget::setProperty(const std::string& _key, const std::string& _value) */
		virtual void setProperty(const std::string& _key, const std::string& _value);

	/*internal:*/
		virtual void _initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name);

	protected:
		virtual ~StaticImage();

		void baseChangeWidgetSkin(ResourceSkin* _info);

	private:
		void initialiseWidgetSkin(ResourceSkin* _info);
		void shutdownWidgetSkin();

		void frameEntered(float _frame);

		void recalcIndexes();
		void updateSelectIndex(size_t _index);

		void frameAdvise(bool _advise);

	private:
		// кусок в текстуре наших картинок
		IntRect mRectImage;
		// размер одной картинки
		IntSize mSizeTile;
		// размер текстуры
		IntSize mSizeTexture;
		// текущая картинка
		size_t mIndexSelect;

		VectorImages mItems;

		bool mFrameAdvise;
		float mCurrentTime;
		size_t mCurrentFrame;

		ResourceImageSetPtr mResource;
		std::string mItemName;
		std::string mItemGroup;
		std::string mCurrentTextureName;

	};

} // namespace MyGUI

#endif // __MYGUI_STATIC_IMAGE_H__
