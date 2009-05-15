/*
  Copyright (c) Jacob Essex 2009

  This file is part of MWLand.

  MWLand is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  MWLand is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with MWLand.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
* @brief utility class for holding two values
*
* This is mainly used for querying the position of the quad.
* Each quad has a center position, which we can use as a unique identifier
*/
template<class T>
struct Point2 {
    T x, y; //held values.

    inline Point2() {}
    inline Point2(T ix, T iy) {
        x = ix;
        y = iy;
    }
    inline Point2(const Point2<T>& i) {
        x = i.x;
        y = i.y;
    }

    /**
    * @brief comparison operator. Although not used directly, this
    * class is used in std::map a lot, which used the < operator
    */

    inline bool operator<(const Point2<T>& rhs) const{
        return ( x < rhs.x || !( rhs.x < x) && y < rhs.y );
    }

    inline Point2 operator + (const Point2<T>& rhs) {
        return Point2(x + rhs.x, y + rhs.y);
    }
};
