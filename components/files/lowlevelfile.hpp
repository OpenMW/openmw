#ifndef COMPONENTS_FILES_LOWLEVELFILE_HPP
#define COMPONENTS_FILES_LOWLEVELFILE_HPP

#include <OgrePlatform.h>

#include <cstdlib>

#define FILE_API_STDIO	0
#define FILE_API_POSIX	1
#define FILE_API_WIN32	2

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#define FILE_API	FILE_API_POSIX
#elif OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define FILE_API	FILE_API_WIN32
#else
#define FILE_API	FILE_API_STDIO
#endif

#if FILE_API == FILE_API_STDIO
#include <cstdio>
#elif FILE_API == FILE_API_POSIX
#elif FILE_API == FILE_API_WIN32
#include <windows.h>
#else
#error Unsupported File API
#endif

class LowLevelFile
{
public:

	LowLevelFile ();
	~LowLevelFile ();

	void open (char const * filename);
	void close ();

	size_t size ();

	void seek (size_t Position);
	size_t tell ();

	size_t read (void * data, size_t size);

private:
#if FILE_API == FILE_API_STDIO
	FILE* mHandle;
#elif FILE_API == FILE_API_POSIX
	int mHandle;
#elif FILE_API == FILE_API_WIN32
	HANDLE mHandle;
#endif
};

#endif
