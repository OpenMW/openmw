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
#ifndef __MYGUI_TPONT_H__
#define __MYGUI_TPONT_H__

#include "MyGUI_Prerequest.h"

namespace MyGUI
{
	namespace types
	{
		template< typename T > struct TPoint
		{
			T left, top;

			TPoint() : left( 0 ), top( 0 ) { }
			TPoint( T const& _left, T const& _top) : left( _left ), top( _top ) { }
			TPoint( TPoint const& o ) : left( o.left ), top( o.top ) { }

			TPoint& operator-=( TPoint const& o )
			{
				left -= o.left;
				top -= o.top;
				return *this;
			}

			TPoint& operator+=( TPoint const& o )
			{
				left += o.left;
				top += o.top;
				return *this;
			}

			TPoint operator-( TPoint const& o ) const
			{
				return TPoint(left - o.left, top - o.top);
			}

			TPoint operator+( TPoint const& o ) const
			{
				return TPoint(left + o.left, top + o.top);
			}

			TPoint& operator=( TPoint const& o )
			{
				left = o.left;
				top = o.top;
				return *this;
			}

			template< typename U >
			TPoint& operator=( TPoint<U> const& o )
			{
				left = o.left;
				top = o.top;
				return *this;
			}

			bool operator==( TPoint const& o ) const
			{
				return ((left == o.left) && (top == o.top));
			}

			bool operator!=( TPoint const& o ) const
			{
				return ! ((left == o.left) && (top == o.top));
			}

			void clear()
			{
				left = top = 0;
			}

			void set( T const& _left, T const& _top)
			{
				left = _left;
				top = _top;
			}

			void swap(TPoint& _value)
			{
				TPoint tmp = _value;
				_value = *this;
				*this = tmp;
			}

			bool empty() const
			{
				return ((left == 0) && (top == 0));
			}

			std::string print() const
			{
				std::ostringstream stream;
				stream << *this;
				return stream.str();
			}

			static TPoint<T> parse(const std::string& _value)
			{
				TPoint<T> result;
				std::istringstream stream(_value);
				stream >> result.left >> result.top;
				if (stream.fail()) return TPoint<T>();
				else
				{
					int item = stream.get();
					while (item != -1)
					{
						if (item != ' ' && item != '\t') return TPoint<T>();
						item = stream.get();
					}
				}
				return result;
			}

			friend std::ostream& operator << ( std::ostream& _stream, const TPoint<T>&  _value )
			{
				_stream << _value.left << " " << _value.top;
				return _stream;
			}

			friend std::istream& operator >> ( std::istream& _stream, TPoint<T>&  _value )
			{
				_stream >> _value.left >> _value.top;
				if (_stream.fail()) _value.clear();
				return _stream;
			}

		};

	} // namespace types
} // namespace MyGUI

#endif // __MYGUI_TPONT_H__
