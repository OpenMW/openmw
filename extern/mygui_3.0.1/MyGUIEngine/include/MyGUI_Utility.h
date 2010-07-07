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
#ifndef __MYGUI_UTILITY_H__
#define __MYGUI_UTILITY_H__

#include "MyGUI_Prerequest.h"
#include <vector>
#include <sstream>

namespace MyGUI
{
	namespace utility
	{

		inline void trim(std::string& _str, bool _left = true, bool _right = true)
		{
			if (_right) _str.erase(_str.find_last_not_of(" \t\r")+1);
			if (_left) _str.erase(0, _str.find_first_not_of(" \t\r"));
		}

		// конвертирование в строку
		template<typename T >
		inline std::string toString (T p)
		{
			std::ostringstream stream;
			stream << p;
			return stream.str();
		}

		inline const std::string& toString (const std::string& _value)
		{
			return _value;
		}

		template<typename T1,  typename T2 >
		inline std::string toString (T1 p1, T2 p2)
		{
			std::ostringstream stream;
			stream << p1 << p2;
			return stream.str();
		}

		template<typename T1,  typename T2,  typename T3 >
		inline std::string toString (T1 p1, T2 p2, T3 p3)
		{
			std::ostringstream stream;
			stream << p1 << p2 << p3;
			return stream.str();
		}

		template<typename T1,  typename T2,  typename T3, typename T4 >
		inline std::string toString (T1 p1, T2 p2, T3 p3, T4 p4)
		{
			std::ostringstream stream;
			stream << p1 << p2 << p3 << p4;
			return stream.str();
		}

		template<typename T1,  typename T2,  typename T3, typename T4, typename T5 >
		inline std::string toString (T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
		{
			std::ostringstream stream;
			stream << p1 << p2 << p3 << p4 << p5;
			return stream.str();
		}

		template<typename T1,  typename T2,  typename T3, typename T4, typename T5, typename T6 >
		inline std::string toString (T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
		{
			std::ostringstream stream;
			stream << p1 << p2 << p3 << p4 << p5 << p6;
			return stream.str();
		}

		template<typename T1,  typename T2,  typename T3, typename T4, typename T5, typename T6, typename T7 >
		inline std::string toString (T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
		{
			std::ostringstream stream;
			stream << p1 << p2 << p3 << p4 << p5 << p6 << p7;
			return stream.str();
		}

		template<typename T1,  typename T2,  typename T3, typename T4, typename T5, typename T6, typename T7, typename T8 >
		inline std::string toString (T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
		{
			std::ostringstream stream;
			stream << p1 << p2 << p3 << p4 << p5 << p6 << p7 << p8;
			return stream.str();
		}

		template<typename T1,  typename T2,  typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9 >
		inline std::string toString (T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9)
		{
			std::ostringstream stream;
			stream << p1 << p2 << p3 << p4 << p5 << p6 << p7 << p8 << p9;
			return stream.str();
		}

		template< >
		inline std::string toString<bool> (bool _value)
		{
			return _value ? "true" : "false";
		}


		// утилиты для парсинга
		template<typename T >
		inline T parseValue( const std::string& _value )
		{
			std::istringstream stream(_value);
			T result;
			stream >> result;
			if (stream.fail()) return T();
			else
			{
				int item = stream.get();
				while (item != -1)
				{
					if (item != ' ' && item != '\t') return T();
					item = stream.get();
				}
			}
			return result;
		}

		// отдельная имплементация под bool
		template<>
		inline bool parseValue( const std::string& _value )
		{
			if (_value == "true" || _value == "1") return true;
			return false;
		}

		// отдельная имплементация под char
		template<>
		inline char parseValue( const std::string& _value ) { return (char)parseValue<short>(_value); }

		// отдельная имплементация под unsigned char
		template<>
		inline unsigned char parseValue( const std::string& _value ) { return (unsigned char)parseValue<unsigned short>(_value); }


		inline short parseShort(const std::string& _value) { return parseValue<short>(_value); }
		inline unsigned short parseUShort(const std::string& _value) { return parseValue<unsigned short>(_value); }
		inline int parseInt(const std::string& _value) { return parseValue<int>(_value); }
		inline unsigned int parseUInt(const std::string& _value) { return parseValue<unsigned int>(_value); }
		inline size_t parseSizeT(const std::string& _value) { return parseValue<size_t>(_value); }
		inline float parseFloat(const std::string& _value) { return parseValue<float>(_value); }
		inline double parseDouble(const std::string& _value) { return parseValue<double>(_value); }

		inline bool parseBool(const std::string& _value) { return parseValue<bool>(_value); }
		inline char parseChar(const std::string& _value) { return parseValue<char>(_value); }
		inline unsigned char parseUChar(const std::string& _value) { return parseValue<unsigned char>(_value); }

		// для парсинга сложных типов, состоящих из простых
		template<typename T1, typename T2 >
		inline T1 parseValueEx2(const std::string& _value)
		{
			T2 p1, p2;
			std::istringstream stream(_value);
			stream >> p1 >> p2;
			if (stream.fail()) return T1();
			else
			{
				int item = stream.get();
				while (item != -1)
				{
					if (item != ' ' && item != '\t') return T1();
					item = stream.get();
				}
			}
			return T1(p1, p2);
		}

		template<typename T1, typename T2 >
		inline T1 parseValueEx3(const std::string& _value)
		{
			T2 p1, p2, p3;
			std::istringstream stream(_value);
			stream >> p1 >> p2 >> p3;
			if (stream.fail()) return T1();
			else
			{
				int item = stream.get();
				while (item != -1)
				{
					if (item != ' ' && item != '\t') return T1();
					item = stream.get();
				}
			}
			return T1(p1, p2, p3);
		}

		template<typename T1, typename T2 >
		inline T1 parseValueEx4(const std::string& _value)
		{
			T2 p1, p2, p3, p4;
			std::istringstream stream(_value);
			stream >> p1 >> p2 >> p3 >> p4;
			if (stream.fail()) return T1();
			else
			{
				int item = stream.get();
				while (item != -1)
				{
					if (item != ' ' && item != '\t') return T1();
					item = stream.get();
				}
			}
			return T1(p1, p2, p3, p4);
		}

		namespace templates
		{
			template<typename T>
			inline void split(std::vector<std::string>& _ret, const std::string& _source, const std::string& _delims)
			{
				size_t start = _source.find_first_not_of(_delims);
				while (start != _source.npos)
				{
					size_t end = _source.find_first_of(_delims, start);
					if (end != _source.npos) _ret.push_back(_source.substr(start, end-start));
					else
					{
						_ret.push_back(_source.substr(start));
						break;
					}
					start = _source.find_first_not_of(_delims, end + 1);
				}
			}
		} // namespace templates

		inline std::vector<std::string> split(const std::string& _source, const std::string& _delims = "\t\n ")
		{
			std::vector<std::string> result;
			templates::split<void>(result, _source, _delims);
			return result;
		}

		template<typename T1, typename T2, typename T3, typename T4>
		inline bool parseComplex(const std::string& _value, T1& _p1, T2& _p2, T3& _p3, T4& _p4)
		{
			std::istringstream stream(_value);

			stream >> _p1 >> _p2 >> _p3 >> _p4;

			if (stream.fail()) return false;
			int item = stream.get();
			while (item != -1)
			{
				if (item != ' ' && item != '\t') return false;
				item = stream.get();
			}

			return true;
		}

		template<typename T1, typename T2, typename T3>
		inline bool parseComplex(const std::string& _value, T1& _p1, T2& _p2, T3& _p3)
		{
			std::istringstream stream(_value);

			stream >> _p1 >> _p2 >> _p3;

			if (stream.fail()) return false;
			int item = stream.get();
			while (item != -1)
			{
				if (item != ' ' && item != '\t') return false;
				item = stream.get();
			}

			return true;
		}

		template<typename T1, typename T2>
		inline bool parseComplex(const std::string& _value, T1& _p1, T2& _p2)
		{
			std::istringstream stream(_value);

			stream >> _p1 >> _p2;

			if (stream.fail()) return false;
			int item = stream.get();
			while (item != -1)
			{
				if (item != ' ' && item != '\t') return false;
				item = stream.get();
			}

			return true;
		}

		template<typename T1>
		inline bool parseComplex(const std::string& _value, T1& _p1)
		{
			std::istringstream stream(_value);

			stream >> _p1;

			if (stream.fail()) return false;
			int item = stream.get();
			while (item != -1)
			{
				if (item != ' ' && item != '\t') return false;
				item = stream.get();
			}

			return true;
		}

		template<>
		inline bool parseComplex<bool>(const std::string& _value, bool& _p1)
		{
			std::string value(_value);
			trim(value);
			if ((value == "true") || (value == "1"))
			{
				_p1 = true;
				return true;
			}
			else if ((value == "false") || (value == "0"))
			{
				_p1 = false;
				return true;
			}

			return false;
		}

	} // namespace utility

} // namespace MyGUI

#endif // __MYGUI_UTILITY_H__
