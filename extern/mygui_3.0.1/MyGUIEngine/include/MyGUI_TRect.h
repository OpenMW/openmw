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
#ifndef __MYGUI_TRECT_H__
#define __MYGUI_TRECT_H__

#include "MyGUI_Prerequest.h"

namespace MyGUI
{
	namespace types
	{

		template< typename T > struct TRect
		{
			T left, top, right, bottom;

			TRect() : left( 0 ), top( 0 ), right( 0 ), bottom( 0 ) { }
			TRect( T const& _left, T const& _top, T const& _right, T const& _bottom ) : left( _left ), top( _top ), right( _right ), bottom( _bottom ) { }
			TRect( TRect const& o ) : left( o.left ), top( o.top ), right( o.right ), bottom( o.bottom ) { }

			TRect& operator-=( TRect const& o )
			{
				left -= o.left;
				top -= o.top;
				right -= o.right;
				bottom -= o.bottom;
				return *this;
			}

			TRect& operator+=( TRect const& o )
			{
				left += o.left;
				top += o.top;
				right += o.right;
				bottom += o.bottom;
				return *this;
			}

			TRect operator-( TRect const& o ) const
			{
				return TRect(left - o.left, top - o.top, right - o.right, bottom - o.bottom);
			}

			TRect operator+( TRect const& o ) const
			{
				return TRect(left + o.left, top + o.top, right + o.right, bottom + o.bottom);
			}

			TRect& operator=( TRect const& o )
			{
				left = o.left;
				top = o.top;
				right = o.right;
				bottom = o.bottom;
				return *this;
			}

			template< typename U >
			TRect& operator=( TRect<U> const& o )
			{
				left = o.left;
				top = o.top;
				right = o.right;
				bottom = o.bottom;
				return *this;
			}

			bool operator==( TRect const& o ) const
			{
				return ((left == o.left) && (top == o.top) && (right == o.right) && (bottom == o.bottom));
			}

			bool operator!=( TRect const& o ) const
			{
				return ! ((left == o.left) && (top == o.top) && (right == o.right) && (bottom == o.bottom));
			}

			T width() const
			{
				return right - left;
			}

			T height() const
			{
				return bottom - top;
			}

			void clear()
			{
				left = top = right = bottom = 0;
			}

			void set( T const& _left, T const& _top, T const& _right, T const& _bottom )
			{
				left = _left;
				top = _top;
				right = _right;
				bottom = _bottom;
			}

			void swap(TRect& _value)
			{
				TRect tmp = _value;
				_value = *this;
				*this = tmp;
			}

			bool empty() const
			{
				return ((left == 0) && (top == 0) && (right == 0) && (bottom == 0));
			}

			bool inside(const TRect<T>&  _value) const
			{
				return ( (_value.left >= left) && (_value.right <= right) && (_value.top >= top) && (_value.bottom <= bottom) );
			}

			bool intersect(const TRect<T>&  _value) const
			{
				return ( (_value.left <= right) && (_value.right >= left) && (_value.top <= bottom) && (_value.bottom >= top) );
			}

			bool inside(const TPoint<T>&  _value) const
			{
				return ( (_value.left >= left) && (_value.left <= right) && (_value.top >= top) && (_value.top <= bottom) );
			}

			std::string print() const
			{
				std::ostringstream stream;
				stream << *this;
				return stream.str();
			}

			static TRect<T> parse(const std::string& _value)
			{
				TRect<T> result;
				std::istringstream stream(_value);
				stream >> result.left >> result.top >> result.right >> result.bottom;
				if (stream.fail()) return TRect<T>();
				else
				{
					int item = stream.get();
					while (item != -1)
					{
						if (item != ' ' && item != '\t') return TRect<T>();
						item = stream.get();
					}
				}
				return result;
			}

			friend std::ostream& operator << ( std::ostream& _stream, const TRect<T>&  _value )
			{
				_stream << _value.left << " " << _value.top << " " << _value.right << " " << _value.bottom;
				return _stream;
			}

			friend std::istream& operator >> ( std::istream& _stream, TRect<T>&  _value )
			{
				_stream >> _value.left >> _value.top >> _value.right >> _value.bottom;
				if (_stream.fail()) _value.clear();
				return _stream;
			}

		};

	} // namespace types
} // namespace MyGUI

#endif // __MYGUI_TRECT_H__
