/*!
	@file
	@author		Albert Semenov
	@date		12/2007
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
#ifndef __MyGUI_TCOORD_H__
#define __MyGUI_TCOORD_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_TPoint.h"
#include "MyGUI_TSize.h"

namespace MyGUI
{
	namespace types
	{

		template< typename T > struct TCoord
		{
			T left, top, width, height;

			TCoord() : left( 0 ), top( 0 ), width( 0 ), height( 0 ) { }
			TCoord( T const& _left, T const& _top, T const& _width, T const& _height ) : left( _left ), top( _top ), width( _width ), height( _height ) { }
			TCoord( TCoord const& _obj ) : left( _obj.left ), top( _obj.top ), width( _obj.width ), height( _obj.height ) { }
			TCoord( TPoint<T> const& _point, TSize<T> const& _size ) : left( _point.left ), top( _point.top ), width( _size.width ), height( _size.height ) { }

			TCoord& operator-=( TCoord const& _obj )
			{
				left -= _obj.left;
				top -= _obj.top;
				width -= _obj.width;
				height -= _obj.height;
				return *this;
			}

			TCoord& operator+=( TCoord const& _obj )
			{
				left += _obj.left;
				top += _obj.top;
				width += _obj.width;
				height += _obj.height;
				return *this;
			}

			TCoord operator-( TCoord const& _obj ) const
			{
				return TCoord(left - _obj.left, top - _obj.top, width - _obj.width, height - _obj.height);
			}

			TCoord operator-( TPoint<T> const& _obj ) const
			{
				return TCoord(left - _obj.left, top - _obj.top, width, height);
			}

			TCoord operator-( TSize<T> const& _obj ) const
			{
				return TCoord(left, top, width - _obj.width, height - _obj.height);
			}

			TCoord operator+( TCoord const& _obj ) const
			{
				return TCoord(left + _obj.left, top + _obj.top, width + _obj.width, height + _obj.height);
			}

			TCoord operator+( TPoint<T> const& _obj ) const
			{
				return TCoord(left + _obj.left, top + _obj.top, width, height);
			}

			TCoord operator+( TSize<T> const& _obj ) const
			{
				return TCoord(left, top, width + _obj.width, height + _obj.height);
			}

			TCoord& operator=( TCoord const& _obj )
			{
				left = _obj.left;
				top = _obj.top;
				width = _obj.width;
				height = _obj.height;
				return *this;
			}

			template< typename U >
			TCoord& operator=( TCoord<U> const& _obj )
			{
				left = _obj.left;
				top = _obj.top;
				width = _obj.width;
				height = _obj.height;
				return *this;
			}

			TCoord& operator=( TPoint<T> const& _obj )
			{
				left = _obj.left;
				top = _obj.top;
				return *this;
			}

			TCoord& operator=( TSize<T> const& _obj )
			{
				width = _obj.width;
				height = _obj.height;
				return *this;
			}


			bool operator==( TCoord const& _obj ) const
			{
				return ((left == _obj.left) && (top == _obj.top) && (width == _obj.width) && (height == _obj.height));
			}

			bool operator!=( TCoord const& _obj ) const
			{
				return ! ((left == _obj.left) && (top == _obj.top) && (width == _obj.width) && (height == _obj.height));
			}

			T right() const
			{
				return left + width;
			}

			T bottom() const
			{
				return top + height;
			}

			void clear()
			{
				left = top = width = height = 0;
			}

			void set( T const& _left, T const& _top, T const& _width, T const& _height )
			{
				left = _left;
				top = _top;
				width = _width;
				height = _height;
			}

			void swap(TCoord& _value)
			{
				TCoord tmp = _value;
				_value = *this;
				*this = tmp;
			}

			bool empty() const
			{
				return ((left == 0) && (top == 0) && (width == 0) && (height == 0));
			}

			TPoint<T> point() const
			{
				return TPoint<T>(left, top);
			}

			TSize<T> size() const
			{
				return TSize<T>(width, height);
			}

			bool inside(const TPoint<T>&  _value) const
			{
				return ( (_value.left >= left) && (_value.left <= right()) && (_value.top >= top) && (_value.top <= bottom()) );
			}

			std::string print() const
			{
				std::ostringstream stream;
				stream << *this;
				return stream.str();
			}

			static TCoord<T> parse(const std::string& _value)
			{
				TCoord<T> result;
				std::istringstream stream(_value);
				stream >> result.left >> result.top >> result.width >> result.height;
				if (stream.fail()) return TCoord<T>();
				else
				{
					int item = stream.get();
					while (item != -1)
					{
						if (item != ' ' && item != '\t') return TCoord<T>();
						item = stream.get();
					}
				}
				return result;
			}

			friend std::ostream& operator << ( std::ostream& _stream, const TCoord<T>&  _value )
			{
				_stream << _value.left << " " << _value.top << " " << _value.width << " " << _value.height;
				return _stream;
			}

			friend std::istream& operator >> ( std::istream& _stream, TCoord<T>&  _value )
			{
				_stream >> _value.left >> _value.top >> _value.width >> _value.height;
				if (_stream.fail()) _value.clear();
				return _stream;
			}

		};

	} // namespace types
} // namespace MyGUI

#endif // __MyGUI_TCOORD_H__
