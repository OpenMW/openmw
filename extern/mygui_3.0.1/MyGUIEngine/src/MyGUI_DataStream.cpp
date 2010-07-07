/*!
	@file
	@author		Albert Semenov
	@date		08/2009
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
#include "MyGUI_DataStream.h"

namespace MyGUI
{

	DataStream::DataStream() :
		mStream(nullptr),
		mSize((size_t)-1)
	{
	}

	DataStream::DataStream(std::istream* _stream) :
		mStream(_stream),
		mSize((size_t)-1)
	{
	}

	DataStream::~DataStream()
	{
	}

	size_t DataStream::size()
	{
		if (mStream == nullptr) return 0;
		if (mSize == (size_t)-1)
		{
			mStream->seekg (0, std::ios::end);
			mSize = mStream->tellg();
			mStream->seekg (0, std::ios::beg);
		}
		return mSize;
	}

	bool DataStream::eof()
	{
		return mStream == nullptr ? true : mStream->eof();
	}

	void DataStream::readline(std::string& _source, Char _delim)
	{
		if (mStream == nullptr) return;
		std::getline(*mStream, _source, (char)_delim);
	}

	size_t DataStream::read(void* _buf, size_t _count)
	{
		if (mStream == nullptr) return 0;
		size_t count = std::min(size(), _count);
		mStream->read((char*)_buf, count);
		return count;
	}

} // namespace MyGUI
