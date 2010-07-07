/*!
	@file
	@author		Albert Semenov
	@date		08/2009
	@module
*/

#ifndef __MYGUI_OGRE_DATA_STREAM_H__
#define __MYGUI_OGRE_DATA_STREAM_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_IDataStream.h"

#include <OgreDataStream.h>

#include "MyGUI_LastHeader.h"

namespace MyGUI
{

	class OgreDataStream : public IDataStream
	{
	public:
		OgreDataStream(Ogre::DataStreamPtr _stream);
		~OgreDataStream();

		virtual bool eof();
		virtual size_t size();
		virtual void readline(std::string& _source, Char _delim);
		virtual size_t read(void* _buf, size_t _count);

	private:
		Ogre::DataStreamPtr mStream;
	};

} // namespace MyGUI

#endif // __MYGUI_OGRE_DATA_STREAM_H__
